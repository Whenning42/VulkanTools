/* Copyright (c) 2015-2018 The Khronos Group Inc.
 * Copyright (c) 2015-2018 Valve Corporation
 * Copyright (c) 2015-2018 LunarG, Inc.
 * Copyright (C) 2015-2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Vulkan-ValidationLayers's shader_validation.cpp authors
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Dave Houlton <daveh@lunarg.com>
 *
 * VkViz modifications author
 * Author: William Henning <whenning@google.com>
 */

// This file is an adapted version of Vulkan-ValidationLayers's shader_validation.cpp used by VkViz to determine which resources a
// shader reads and writes. Changes include removing all the validation code and adding a GetShaderDescriptorUses function that
// calls a modified version of the CollectInterfaceByDescriptorSlot function.

#include <cassert>
#include <cinttypes>
#include <map>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "spirv.hpp"
#include "vk_loader_platform.h"
#include "vk_enum_string_helper.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "shader.h"
#include "xxhash.h"

// A forward iterator over spirv instructions. Provides easy access to len, opcode, and content words
// without the caller needing to care too much about the physical SPIRV module layout.
struct spirv_inst_iter {
    std::vector<uint32_t>::const_iterator zero;
    std::vector<uint32_t>::const_iterator it;

    // Length is stored in the upper 16 bits of word 0
    uint32_t len() {
        auto result = *it >> 16;
        assert(result > 0);
        return result;
    }

    // Op code is stored in the lower 16 bits of word 0
    uint32_t opcode() { return *it & 0x0ffffu; }

    uint32_t const &word(unsigned n) {
        assert(n < len());
        return it[n];
    }

    uint32_t offset() { return (uint32_t)(it - zero); }

    spirv_inst_iter() {}

    spirv_inst_iter(std::vector<uint32_t>::const_iterator zero, std::vector<uint32_t>::const_iterator it) : zero(zero), it(it) {}

    bool operator==(spirv_inst_iter const &other) { return it == other.it; }

    bool operator!=(spirv_inst_iter const &other) { return it != other.it; }

    spirv_inst_iter operator++(int) {  // x++
        spirv_inst_iter ii = *this;
        it += len();
        return ii;
    }

    spirv_inst_iter operator++() {  // ++x;
        it += len();
        return *this;
    }

    // The iterator and the value are the same thing.
    spirv_inst_iter &operator*() { return *this; }
    spirv_inst_iter const &operator*() const { return *this; }
};

struct interface_var {
    uint32_t id;
    uint32_t type_id;
    uint32_t offset;

    uint32_t storage_class;

    bool is_patch;
    bool is_block_member;
    bool is_relaxed_precision;
    // TODO: collect the name, too? Isn't required to be present.
};

struct descriptor_slot_t {
    uint32_t set;
    uint32_t binding;
};

struct shader_module {
    // The spirv image itself
    std::vector<uint32_t> words;
    // A mapping of <id> to the first word of its def. this is useful because walking type
    // trees, constant expressions, etc requires jumping all over the instruction stream.
    std::unordered_map<unsigned, unsigned> def_index;
    bool has_valid_spirv;
    VkShaderModule vk_shader_module;

    shader_module(VkShaderModuleCreateInfo const *pCreateInfo, VkShaderModule shaderModule)
        : words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)),
          def_index(),
          has_valid_spirv(true),
          vk_shader_module(shaderModule) {
        BuildDefIndex();
    }

    shader_module() : has_valid_spirv(false), vk_shader_module(VK_NULL_HANDLE) {}

    // Expose begin() / end() to enable range-based for
    spirv_inst_iter begin() const { return spirv_inst_iter(words.begin(), words.begin() + 5); }  // First insn
    spirv_inst_iter end() const { return spirv_inst_iter(words.begin(), words.end()); }          // Just past last insn
    // Given an offset into the module, produce an iterator there.
    spirv_inst_iter at(unsigned offset) const { return spirv_inst_iter(words.begin(), words.begin() + offset); }

    // Gets an iterator to the definition of an id
    spirv_inst_iter get_def(unsigned id) const {
        auto it = def_index.find(id);
        if (it == def_index.end()) {
            return end();
        }
        return at(it->second);
    }

    void BuildDefIndex();
};

enum FORMAT_TYPE {
    FORMAT_TYPE_FLOAT = 1,  // UNORM, SNORM, FLOAT, USCALED, SSCALED, SRGB -- anything we consider float in the shader
    FORMAT_TYPE_SINT = 2,
    FORMAT_TYPE_UINT = 4,
};

typedef std::pair<unsigned, unsigned> location_t;

struct shader_stage_attributes {
    char const *const name;
    bool arrayed_input;
    bool arrayed_output;
};

static shader_stage_attributes shader_stage_attribs[] = {
    {"vertex shader", false, false},  {"tessellation control shader", true, true}, {"tessellation evaluation shader", true, false},
    {"geometry shader", true, false}, {"fragment shader", false, false},
};

// SPIRV utility functions
void shader_module::BuildDefIndex() {
    for (auto insn : *this) {
        switch (insn.opcode()) {
            // Types
            case spv::OpTypeVoid:
            case spv::OpTypeBool:
            case spv::OpTypeInt:
            case spv::OpTypeFloat:
            case spv::OpTypeVector:
            case spv::OpTypeMatrix:
            case spv::OpTypeImage:
            case spv::OpTypeSampler:
            case spv::OpTypeSampledImage:
            case spv::OpTypeArray:
            case spv::OpTypeRuntimeArray:
            case spv::OpTypeStruct:
            case spv::OpTypeOpaque:
            case spv::OpTypePointer:
            case spv::OpTypeFunction:
            case spv::OpTypeEvent:
            case spv::OpTypeDeviceEvent:
            case spv::OpTypeReserveId:
            case spv::OpTypeQueue:
            case spv::OpTypePipe:
                def_index[insn.word(1)] = insn.offset();
                break;

                // Fixed constants
            case spv::OpConstantTrue:
            case spv::OpConstantFalse:
            case spv::OpConstant:
            case spv::OpConstantComposite:
            case spv::OpConstantSampler:
            case spv::OpConstantNull:
                def_index[insn.word(2)] = insn.offset();
                break;

                // Specialization constants
            case spv::OpSpecConstantTrue:
            case spv::OpSpecConstantFalse:
            case spv::OpSpecConstant:
            case spv::OpSpecConstantComposite:
            case spv::OpSpecConstantOp:
                def_index[insn.word(2)] = insn.offset();
                break;

                // Variables
            case spv::OpVariable:
                def_index[insn.word(2)] = insn.offset();
                break;

                // Functions
            case spv::OpFunction:
                def_index[insn.word(2)] = insn.offset();
                break;

            default:
                // We don't care about any other defs for now.
                break;
        }
    }
}

static spirv_inst_iter FindEntrypoint(shader_module const *src, char const *name, VkShaderStageFlagBits stageBits) {
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpEntryPoint) {
            auto entrypointName = (char const *)&insn.word(3);
            auto entrypointStageBits = 1u << insn.word(1);

            if (!strcmp(entrypointName, name) && (entrypointStageBits & stageBits)) {
                return insn;
            }
        }
    }

    return src->end();
}

static char const *StorageClassName(unsigned sc) {
    switch (sc) {
        case spv::StorageClassInput:
            return "input";
        case spv::StorageClassOutput:
            return "output";
        case spv::StorageClassUniformConstant:
            return "const uniform";
        case spv::StorageClassUniform:
            return "uniform";
        case spv::StorageClassWorkgroup:
            return "workgroup local";
        case spv::StorageClassCrossWorkgroup:
            return "workgroup global";
        case spv::StorageClassPrivate:
            return "private global";
        case spv::StorageClassFunction:
            return "function";
        case spv::StorageClassGeneric:
            return "generic";
        case spv::StorageClassAtomicCounter:
            return "atomic counter";
        case spv::StorageClassImage:
            return "image";
        case spv::StorageClassPushConstant:
            return "push constant";
        case spv::StorageClassStorageBuffer:
            return "storage buffer";
        default:
            return "unknown";
    }
}

// Get the value of an integral constant
unsigned GetConstantValue(shader_module const *src, unsigned id) {
    auto value = src->get_def(id);
    assert(value != src->end());

    if (value.opcode() != spv::OpConstant) {
        // TODO: Either ensure that the specialization transform is already performed on a module we're
        //       considering here, OR -- specialize on the fly now.
        return 1;
    }

    return value.word(3);
}

static void DescribeTypeInner(std::ostringstream &ss, shader_module const *src, unsigned type) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypeBool:
            ss << "bool";
            break;
        case spv::OpTypeInt:
            ss << (insn.word(3) ? 's' : 'u') << "int" << insn.word(2);
            break;
        case spv::OpTypeFloat:
            ss << "float" << insn.word(2);
            break;
        case spv::OpTypeVector:
            ss << "vec" << insn.word(3) << " of ";
            DescribeTypeInner(ss, src, insn.word(2));
            break;
        case spv::OpTypeMatrix:
            ss << "mat" << insn.word(3) << " of ";
            DescribeTypeInner(ss, src, insn.word(2));
            break;
        case spv::OpTypeArray:
            ss << "arr[" << GetConstantValue(src, insn.word(3)) << "] of ";
            DescribeTypeInner(ss, src, insn.word(2));
            break;
        case spv::OpTypePointer:
            ss << "ptr to " << StorageClassName(insn.word(2)) << " ";
            DescribeTypeInner(ss, src, insn.word(3));
            break;
        case spv::OpTypeStruct: {
            ss << "struct of (";
            for (unsigned i = 2; i < insn.len(); i++) {
                DescribeTypeInner(ss, src, insn.word(i));
                if (i == insn.len() - 1) {
                    ss << ")";
                } else {
                    ss << ", ";
                }
            }
            break;
        }
        case spv::OpTypeSampler:
            ss << "sampler";
            break;
        case spv::OpTypeSampledImage:
            ss << "sampler+";
            DescribeTypeInner(ss, src, insn.word(2));
            break;
        case spv::OpTypeImage:
            ss << "image(dim=" << insn.word(3) << ", sampled=" << insn.word(7) << ")";
            break;
        default:
            ss << "oddtype";
            break;
    }
}

static std::string DescribeType(shader_module const *src, unsigned type) {
    std::ostringstream ss;
    DescribeTypeInner(ss, src, type);
    return ss.str();
}

static bool IsNarrowNumericType(spirv_inst_iter type) {
    if (type.opcode() != spv::OpTypeInt && type.opcode() != spv::OpTypeFloat) return false;
    return type.word(2) < 64;
}

static bool TypesMatch(shader_module const *a, shader_module const *b, unsigned a_type, unsigned b_type, bool a_arrayed,
                       bool b_arrayed, bool relaxed) {
    // Walk two type trees together, and complain about differences
    auto a_insn = a->get_def(a_type);
    auto b_insn = b->get_def(b_type);
    assert(a_insn != a->end());
    assert(b_insn != b->end());

    if (a_arrayed && a_insn.opcode() == spv::OpTypeArray) {
        return TypesMatch(a, b, a_insn.word(2), b_type, false, b_arrayed, relaxed);
    }

    if (b_arrayed && b_insn.opcode() == spv::OpTypeArray) {
        // We probably just found the extra level of arrayness in b_type: compare the type inside it to a_type
        return TypesMatch(a, b, a_type, b_insn.word(2), a_arrayed, false, relaxed);
    }

    if (a_insn.opcode() == spv::OpTypeVector && relaxed && IsNarrowNumericType(b_insn)) {
        return TypesMatch(a, b, a_insn.word(2), b_type, a_arrayed, b_arrayed, false);
    }

    if (a_insn.opcode() != b_insn.opcode()) {
        return false;
    }

    if (a_insn.opcode() == spv::OpTypePointer) {
        // Match on pointee type. storage class is expected to differ
        return TypesMatch(a, b, a_insn.word(3), b_insn.word(3), a_arrayed, b_arrayed, relaxed);
    }

    if (a_arrayed || b_arrayed) {
        // If we havent resolved array-of-verts by here, we're not going to.
        return false;
    }

    switch (a_insn.opcode()) {
        case spv::OpTypeBool:
            return true;
        case spv::OpTypeInt:
            // Match on width, signedness
            return a_insn.word(2) == b_insn.word(2) && a_insn.word(3) == b_insn.word(3);
        case spv::OpTypeFloat:
            // Match on width
            return a_insn.word(2) == b_insn.word(2);
        case spv::OpTypeVector:
            // Match on element type, count.
            if (!TypesMatch(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false)) return false;
            if (relaxed && IsNarrowNumericType(a->get_def(a_insn.word(2)))) {
                return a_insn.word(3) >= b_insn.word(3);
            } else {
                return a_insn.word(3) == b_insn.word(3);
            }
        case spv::OpTypeMatrix:
            // Match on element type, count.
            return TypesMatch(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false) &&
                   a_insn.word(3) == b_insn.word(3);
        case spv::OpTypeArray:
            // Match on element type, count. these all have the same layout. we don't get here if b_arrayed. This differs from
            // vector & matrix types in that the array size is the id of a constant instruction, * not a literal within OpTypeArray
            return TypesMatch(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false) &&
                   GetConstantValue(a, a_insn.word(3)) == GetConstantValue(b, b_insn.word(3));
        case spv::OpTypeStruct:
            // Match on all element types
            {
                if (a_insn.len() != b_insn.len()) {
                    return false;  // Structs cannot match if member counts differ
                }

                for (unsigned i = 2; i < a_insn.len(); i++) {
                    if (!TypesMatch(a, b, a_insn.word(i), b_insn.word(i), a_arrayed, b_arrayed, false)) {
                        return false;
                    }
                }

                return true;
            }
        default:
            // Remaining types are CLisms, or may not appear in the interfaces we are interested in. Just claim no match.
            return false;
    }
}

static unsigned ValueOrDefault(std::unordered_map<unsigned, unsigned> const &map, unsigned id, unsigned def) {
    auto it = map.find(id);
    if (it == map.end())
        return def;
    else
        return it->second;
}

static unsigned GetLocationsConsumedByType(shader_module const *src, unsigned type, bool strip_array_level) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypePointer:
            // See through the ptr -- this is only ever at the toplevel for graphics shaders we're never actually passing
            // pointers around.
            return GetLocationsConsumedByType(src, insn.word(3), strip_array_level);
        case spv::OpTypeArray:
            if (strip_array_level) {
                return GetLocationsConsumedByType(src, insn.word(2), false);
            } else {
                return GetConstantValue(src, insn.word(3)) * GetLocationsConsumedByType(src, insn.word(2), false);
            }
        case spv::OpTypeMatrix:
            // Num locations is the dimension * element size
            return insn.word(3) * GetLocationsConsumedByType(src, insn.word(2), false);
        case spv::OpTypeVector: {
            auto scalar_type = src->get_def(insn.word(2));
            auto bit_width =
                (scalar_type.opcode() == spv::OpTypeInt || scalar_type.opcode() == spv::OpTypeFloat) ? scalar_type.word(2) : 32;

            // Locations are 128-bit wide; 3- and 4-component vectors of 64 bit types require two.
            return (bit_width * insn.word(3) + 127) / 128;
        }
        default:
            // Everything else is just 1.
            return 1;

            // TODO: extend to handle 64bit scalar types, whose vectors may need multiple locations.
    }
}

static unsigned GetLocationsConsumedByFormat(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R64G64B64A64_SFLOAT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_UINT:
            return 2;
        default:
            return 1;
    }
}

static unsigned GetFormatType(VkFormat fmt) {
    if (FormatIsSInt(fmt)) return FORMAT_TYPE_SINT;
    if (FormatIsUInt(fmt)) return FORMAT_TYPE_UINT;
    if (FormatIsDepthAndStencil(fmt)) return FORMAT_TYPE_FLOAT | FORMAT_TYPE_UINT;
    if (fmt == VK_FORMAT_UNDEFINED) return 0;
    // everything else -- UNORM/SNORM/FLOAT/USCALED/SSCALED is all float in the shader.
    return FORMAT_TYPE_FLOAT;
}

// characterizes a SPIR-V type appearing in an interface to a FF stage, for comparison to a VkFormat's characterization above.
static unsigned GetFundamentalType(shader_module const *src, unsigned type) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypeInt:
            return insn.word(3) ? FORMAT_TYPE_SINT : FORMAT_TYPE_UINT;
        case spv::OpTypeFloat:
            return FORMAT_TYPE_FLOAT;
        case spv::OpTypeVector:
            return GetFundamentalType(src, insn.word(2));
        case spv::OpTypeMatrix:
            return GetFundamentalType(src, insn.word(2));
        case spv::OpTypeArray:
            return GetFundamentalType(src, insn.word(2));
        case spv::OpTypePointer:
            return GetFundamentalType(src, insn.word(3));
        case spv::OpTypeImage:
            return GetFundamentalType(src, insn.word(2));

        default:
            return 0;
    }
}

static uint32_t GetShaderStageId(VkShaderStageFlagBits stage) {
    uint32_t bit_pos = uint32_t(u_ffs(stage));
    return bit_pos - 1;
}

static spirv_inst_iter GetStructType(shader_module const *src, spirv_inst_iter def, bool is_array_of_verts) {
    while (true) {
        if (def.opcode() == spv::OpTypePointer) {
            def = src->get_def(def.word(3));
        } else if (def.opcode() == spv::OpTypeArray && is_array_of_verts) {
            def = src->get_def(def.word(2));
            is_array_of_verts = false;
        } else if (def.opcode() == spv::OpTypeStruct) {
            return def;
        } else {
            return src->end();
        }
    }
}

static bool CollectInterfaceBlockMembers(shader_module const *src, std::map<location_t, interface_var> *out,
                                         std::unordered_map<unsigned, unsigned> const &blocks, bool is_array_of_verts, uint32_t id,
                                         uint32_t type_id, bool is_patch, int /*first_location*/) {
    // Walk down the type_id presented, trying to determine whether it's actually an interface block.
    auto type = GetStructType(src, src->get_def(type_id), is_array_of_verts && !is_patch);
    if (type == src->end() || blocks.find(type.word(1)) == blocks.end()) {
        // This isn't an interface block.
        return false;
    }

    std::unordered_map<unsigned, unsigned> member_components;
    std::unordered_map<unsigned, unsigned> member_relaxed_precision;
    std::unordered_map<unsigned, unsigned> member_patch;

    // Walk all the OpMemberDecorate for type's result id -- first pass, collect components.
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);

            if (insn.word(3) == spv::DecorationComponent) {
                unsigned component = insn.word(4);
                member_components[member_index] = component;
            }

            if (insn.word(3) == spv::DecorationRelaxedPrecision) {
                member_relaxed_precision[member_index] = 1;
            }

            if (insn.word(3) == spv::DecorationPatch) {
                member_patch[member_index] = 1;
            }
        }
    }

    // TODO: correctly handle location assignment from outside

    // Second pass -- produce the output, from Location decorations
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);
            unsigned member_type_id = type.word(2 + member_index);

            if (insn.word(3) == spv::DecorationLocation) {
                unsigned location = insn.word(4);
                unsigned num_locations = GetLocationsConsumedByType(src, member_type_id, false);
                auto component_it = member_components.find(member_index);
                unsigned component = component_it == member_components.end() ? 0 : component_it->second;
                bool is_relaxed_precision = member_relaxed_precision.find(member_index) != member_relaxed_precision.end();
                bool member_is_patch = is_patch || member_patch.count(member_index) > 0;

                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v = {};
                    v.id = id;
                    // TODO: member index in interface_var too?
                    v.type_id = member_type_id;
                    v.offset = offset;
                    v.is_patch = member_is_patch;
                    v.is_block_member = true;
                    v.is_relaxed_precision = is_relaxed_precision;
                    (*out)[std::make_pair(location + offset, component)] = v;
                }
            }
        }
    }

    return true;
}

static std::map<location_t, interface_var> CollectInterfaceByLocation(shader_module const *src, spirv_inst_iter entrypoint,
                                                                      spv::StorageClass sinterface, bool is_array_of_verts) {
    std::unordered_map<unsigned, unsigned> var_locations;
    std::unordered_map<unsigned, unsigned> var_builtins;
    std::unordered_map<unsigned, unsigned> var_components;
    std::unordered_map<unsigned, unsigned> blocks;
    std::unordered_map<unsigned, unsigned> var_patch;
    std::unordered_map<unsigned, unsigned> var_relaxed_precision;

    for (auto insn : *src) {
        // We consider two interface models: SSO rendezvous-by-location, and builtins. Complain about anything that
        // fits neither model.
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationLocation) {
                var_locations[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBuiltIn) {
                var_builtins[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationComponent) {
                var_components[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBlock) {
                blocks[insn.word(1)] = 1;
            }

            if (insn.word(2) == spv::DecorationPatch) {
                var_patch[insn.word(1)] = 1;
            }

            if (insn.word(2) == spv::DecorationRelaxedPrecision) {
                var_relaxed_precision[insn.word(1)] = 1;
            }
        }
    }

    // TODO: handle grouped decorations
    // TODO: handle index=1 dual source outputs from FS -- two vars will have the same location, and we DON'T want to clobber.

    // Find the end of the entrypoint's name string. additional zero bytes follow the actual null terminator, to fill out the
    // rest of the word - so we only need to look at the last byte in the word to determine which word contains the terminator.
    uint32_t word = 3;
    while (entrypoint.word(word) & 0xff000000u) {
        ++word;
    }
    ++word;

    std::map<location_t, interface_var> out;

    for (; word < entrypoint.len(); word++) {
        auto insn = src->get_def(entrypoint.word(word));
        assert(insn != src->end());
        assert(insn.opcode() == spv::OpVariable);

        if (insn.word(3) == static_cast<uint32_t>(sinterface)) {
            unsigned id = insn.word(2);
            unsigned type = insn.word(1);

            int location = ValueOrDefault(var_locations, id, static_cast<unsigned>(-1));
            int builtin = ValueOrDefault(var_builtins, id, static_cast<unsigned>(-1));
            unsigned component = ValueOrDefault(var_components, id, 0);  // Unspecified is OK, is 0
            bool is_patch = var_patch.find(id) != var_patch.end();
            bool is_relaxed_precision = var_relaxed_precision.find(id) != var_relaxed_precision.end();

            if (builtin != -1)
                continue;
            else if (!CollectInterfaceBlockMembers(src, &out, blocks, is_array_of_verts, id, type, is_patch, location)) {
                // A user-defined interface variable, with a location. Where a variable occupied multiple locations, emit
                // one result for each.
                unsigned num_locations = GetLocationsConsumedByType(src, type, is_array_of_verts && !is_patch);
                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v = {};
                    v.id = id;
                    v.type_id = type;
                    v.offset = offset;
                    v.is_patch = is_patch;
                    v.is_relaxed_precision = is_relaxed_precision;
                    out[std::make_pair(location + offset, component)] = v;
                }
            }
        }
    }

    return out;
}

static std::vector<std::pair<uint32_t, interface_var>> CollectInterfaceByInputAttachmentIndex(
    shader_module const *src, std::unordered_set<uint32_t> const &accessible_ids) {
    std::vector<std::pair<uint32_t, interface_var>> out;

    for (auto insn : *src) {
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationInputAttachmentIndex) {
                auto attachment_index = insn.word(3);
                auto id = insn.word(1);

                if (accessible_ids.count(id)) {
                    auto def = src->get_def(id);
                    assert(def != src->end());

                    if (def.opcode() == spv::OpVariable && insn.word(3) == spv::StorageClassUniformConstant) {
                        auto num_locations = GetLocationsConsumedByType(src, def.word(1), false);
                        for (unsigned int offset = 0; offset < num_locations; offset++) {
                            interface_var v = {};
                            v.id = id;
                            v.type_id = def.word(1);
                            v.offset = offset;
                            out.emplace_back(attachment_index + offset, v);
                        }
                    }
                }
            }
        }
    }

    return out;
}

static bool IsWritableDescriptorType(shader_module const *module, uint32_t type_id) {
    auto type = module->get_def(type_id);

    // Strip off any array or ptrs. Where we remove array levels, adjust the  descriptor count for each dimension.
    while (type.opcode() == spv::OpTypeArray || type.opcode() == spv::OpTypePointer) {
        if (type.opcode() == spv::OpTypeArray) {
            type = module->get_def(type.word(2));
        } else {
            if (type.word(2) == spv::StorageClassStorageBuffer) {
                return true;
            }
            type = module->get_def(type.word(3));
        }
    }

    switch (type.opcode()) {
        case spv::OpTypeImage: {
            auto dim = type.word(3);
            auto sampled = type.word(7);
            return sampled == 2 && dim != spv::DimSubpassData;
        }

        case spv::OpTypeStruct:
            for (auto insn : *module) {
                if (insn.opcode() == spv::OpDecorate && insn.word(1) == type.word(1)) {
                    if (insn.word(2) == spv::DecorationBufferBlock) {
                        return true;
                    }
                }
            }
    }

    return false;
}

static std::vector<DescriptorUse> CollectInterfaceByDescriptorSlot(
    debug_report_data const *report_data, shader_module const *src, std::unordered_set<uint32_t> const &accessible_ids,
    bool *has_writable_descriptor) {
    std::unordered_map<unsigned, unsigned> var_sets;
    std::unordered_map<unsigned, unsigned> var_bindings;
    std::unordered_map<unsigned, unsigned> var_nonwritable;
    std::unordered_map<unsigned, unsigned> uniform_blocks;

    for (auto insn : *src) {
        // All variables in the Uniform or UniformConstant storage classes are required to be decorated with both
        // DecorationDescriptorSet and DecorationBinding.
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationDescriptorSet) {
                var_sets[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBinding) {
                var_bindings[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationNonWritable) {
                var_nonwritable[insn.word(1)] = 1;
            }

            if (insn.word(2) == spv::DecorationBlock) {
                uniform_blocks[insn.word(1)] = 1;
            }
        }
    }

    std::vector<DescriptorUse> out;

    for (auto id : accessible_ids) {
        auto insn = src->get_def(id);
        assert(insn != src->end());

        if (insn.opcode() == spv::OpVariable &&
            (insn.word(3) == spv::StorageClassUniform || insn.word(3) == spv::StorageClassUniformConstant ||
             insn.word(3) == spv::StorageClassStorageBuffer)) {
            unsigned set = ValueOrDefault(var_sets, insn.word(2), 0);
            unsigned binding = ValueOrDefault(var_bindings, insn.word(2), 0);

            interface_var v = {};
            v.offset = insn.offset();
            // Operand word, is the resulting Id of the allocation.
            v.id = insn.word(2);
            // Result word, is the id of an OpTypePointer whose type operand is the type of object in memory
            v.type_id = insn.word(1);

            v.storage_class = insn.word(3);

            bool uniform_is_block = false;
            if (v.storage_class == spv::StorageClassUniform) {
                auto typepointer_definition = src->get_def(v.type_id);
                auto uniform_id = typepointer_definition.word(3);
                uniform_is_block = ValueOrDefault(uniform_blocks, uniform_id, 0);
            }

            bool uniform_is_readonly = uniform_is_block || v.storage_class == spv::StorageClassUniformConstant;

            out.emplace_back(DescriptorUse{set, binding, uniform_is_readonly});

            if (var_nonwritable.find(id) == var_nonwritable.end() && IsWritableDescriptorType(src, insn.word(1))) {
                *has_writable_descriptor = true;
            }
        }
    }

    return out;
}

// For some analyses, we need to know about all ids referenced by the static call tree of a particular entrypoint. This is
// important for identifying the set of shader resources actually used by an entrypoint, for example.
// Note: we only explore parts of the image which might actually contain ids we care about for the above analyses.
//  - NOT the shader input/output interfaces.
//
// TODO: The set of interesting opcodes here was determined by eyeballing the SPIRV spec. It might be worth
// converting parts of this to be generated from the machine-readable spec instead.
static std::unordered_set<uint32_t> MarkAccessibleIds(shader_module const *src, spirv_inst_iter entrypoint) {
    std::unordered_set<uint32_t> ids;
    std::unordered_set<uint32_t> worklist;
    worklist.insert(entrypoint.word(2));

    while (!worklist.empty()) {
        auto id_iter = worklist.begin();
        auto id = *id_iter;
        worklist.erase(id_iter);

        auto insn = src->get_def(id);
        if (insn == src->end()) {
            // ID is something we didn't collect in BuildDefIndex. that's OK -- we'll stumble across all kinds of things here
            // that we may not care about.
            continue;
        }

        // Try to add to the output set
        if (!ids.insert(id).second) {
            continue;  // If we already saw this id, we don't want to walk it again.
        }

        switch (insn.opcode()) {
            case spv::OpFunction:
                // Scan whole body of the function, enlisting anything interesting
                while (++insn, insn.opcode() != spv::OpFunctionEnd) {
                    switch (insn.opcode()) {
                        case spv::OpLoad:
                        case spv::OpAtomicLoad:
                        case spv::OpAtomicExchange:
                        case spv::OpAtomicCompareExchange:
                        case spv::OpAtomicCompareExchangeWeak:
                        case spv::OpAtomicIIncrement:
                        case spv::OpAtomicIDecrement:
                        case spv::OpAtomicIAdd:
                        case spv::OpAtomicISub:
                        case spv::OpAtomicSMin:
                        case spv::OpAtomicUMin:
                        case spv::OpAtomicSMax:
                        case spv::OpAtomicUMax:
                        case spv::OpAtomicAnd:
                        case spv::OpAtomicOr:
                        case spv::OpAtomicXor:
                            worklist.insert(insn.word(3));  // ptr
                            break;
                        case spv::OpStore:
                        case spv::OpAtomicStore:
                            worklist.insert(insn.word(1));  // ptr
                            break;
                        case spv::OpAccessChain:
                        case spv::OpInBoundsAccessChain:
                            worklist.insert(insn.word(3));  // base ptr
                            break;
                        case spv::OpSampledImage:
                        case spv::OpImageSampleImplicitLod:
                        case spv::OpImageSampleExplicitLod:
                        case spv::OpImageSampleDrefImplicitLod:
                        case spv::OpImageSampleDrefExplicitLod:
                        case spv::OpImageSampleProjImplicitLod:
                        case spv::OpImageSampleProjExplicitLod:
                        case spv::OpImageSampleProjDrefImplicitLod:
                        case spv::OpImageSampleProjDrefExplicitLod:
                        case spv::OpImageFetch:
                        case spv::OpImageGather:
                        case spv::OpImageDrefGather:
                        case spv::OpImageRead:
                        case spv::OpImage:
                        case spv::OpImageQueryFormat:
                        case spv::OpImageQueryOrder:
                        case spv::OpImageQuerySizeLod:
                        case spv::OpImageQuerySize:
                        case spv::OpImageQueryLod:
                        case spv::OpImageQueryLevels:
                        case spv::OpImageQuerySamples:
                        case spv::OpImageSparseSampleImplicitLod:
                        case spv::OpImageSparseSampleExplicitLod:
                        case spv::OpImageSparseSampleDrefImplicitLod:
                        case spv::OpImageSparseSampleDrefExplicitLod:
                        case spv::OpImageSparseSampleProjImplicitLod:
                        case spv::OpImageSparseSampleProjExplicitLod:
                        case spv::OpImageSparseSampleProjDrefImplicitLod:
                        case spv::OpImageSparseSampleProjDrefExplicitLod:
                        case spv::OpImageSparseFetch:
                        case spv::OpImageSparseGather:
                        case spv::OpImageSparseDrefGather:
                        case spv::OpImageTexelPointer:
                            worklist.insert(insn.word(3));  // Image or sampled image
                            break;
                        case spv::OpImageWrite:
                            worklist.insert(insn.word(1));  // Image -- different operand order to above
                            break;
                        case spv::OpFunctionCall:
                            for (uint32_t i = 3; i < insn.len(); i++) {
                                worklist.insert(insn.word(i));  // fn itself, and all args
                            }
                            break;

                        case spv::OpExtInst:
                            for (uint32_t i = 5; i < insn.len(); i++) {
                                worklist.insert(insn.word(i));  // Operands to ext inst
                            }
                            break;
                    }
                }
                break;
        }
    }

    return ids;
}

std::vector<DescriptorUse> GetShaderDescriptorUses(const VkShaderModuleCreateInfo& shader_create_info, const VkPipelineShaderStageCreateInfo& stage_create_info) {
    VkShaderModule vk_shader_module = stage_create_info.module;
    shader_module shader_module(&shader_create_info, vk_shader_module);
    auto entrypoint = FindEntrypoint(&shader_module, stage_create_info.pName, stage_create_info.stage);

    bool has_writable_descriptor = false;
    auto accessible_ids = MarkAccessibleIds(&shader_module, entrypoint);
    return CollectInterfaceByDescriptorSlot(nullptr, &shader_module, accessible_ids, &has_writable_descriptor);
}
