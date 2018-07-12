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
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: William Henning <whenning@google.com>
 */

// This file is an adapted version of Vulkan-ValidationLayers's shader_validation.h used by VkViz to determine which resources a shader reads and writes.

#ifndef SHADER_H
#define SHADER_H

#include "serialize.h"

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <vulkan_core.h>

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

struct DescriptorUse {
    uint32_t set;
    uint32_t binding;
    uint32_t storage_class;
};
SERIALIZE3(DescriptorUse, uint32_t, set, uint32_t, binding, uint32_t, storage_class);

std::vector<DescriptorUse> GetShaderDescriptorUses(const VkShaderModuleCreateInfo& shader_create_info, const VkPipelineShaderStageCreateInfo& stage_create_info);

#endif  // SHADER_H
