/* Copyright (C) 2018 Google Inc.
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
 * Author: William Henning <whenning@google.com>
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <vulkan_core.h>
#include <type_traits>
#include <fstream>
#include <memory>
#include <vector>
#include <cstdint>

#include "command_enums.h"
#include "serialize.h"
#include "memory_barrier.h"
#include "memory_access.h"

class VkVizPipelineBarrier;

enum VkVizCommandType { BASIC, ACCESS, DRAW, INDEX_BUFFER_BIND, VERTEX_BUFFER_BIND, PIPELINE_BARRIER, BIND_DESCRIPTOR_SETS };

struct BasicCommand {
    virtual const VkVizCommandType VkVizType() { return VkVizCommandType::BASIC; };
    CMD_TYPE type = CMD_NONE;

    virtual ~BasicCommand(){};
    BasicCommand() : type(CMD_NONE){};
    BasicCommand(CMD_TYPE type) : type(type){};

    // Virtual serialization methods to be overwritten by derived classes with the C_SERIALIZE macros
    virtual void to_json(json& j) const { j = {{"type", type}}; }

    virtual void from_json(const json& j) { this->type = j["type"].get<CMD_TYPE>(); }
};

struct Access : BasicCommand {
    const VkVizCommandType VkVizType() override { return VkVizCommandType::ACCESS; };
    std::vector<MemoryAccess> accesses;

    Access() = default;
    Access(CMD_TYPE type, MemoryAccess access) : BasicCommand(type) { accesses.push_back(std::move(access)); };
    Access(CMD_TYPE type, std::vector<MemoryAccess> accesses) : BasicCommand(type), accesses(std::move(accesses)){};

    C_SERIALIZE2(BasicCommand, Access, type, accesses);
};

struct DrawCommand : BasicCommand {
    const VkVizCommandType VkVizType() override { return VkVizCommandType::DRAW; };
    std::vector<std::pair<VkShaderStageFlagBits, std::vector<MemoryAccess>>> stage_accesses;

    DrawCommand() = default;
    DrawCommand(CMD_TYPE type, std::vector<std::pair<VkShaderStageFlagBits, std::vector<MemoryAccess>>> stage_accesses)
        : BasicCommand(type), stage_accesses(std::move(stage_accesses)) {}

    C_SERIALIZE2(BasicCommand, DrawCommand, type, stage_accesses);
};

struct IndexBufferBind : BasicCommand {
    const VkVizCommandType VkVizType() override { return VkVizCommandType::INDEX_BUFFER_BIND; };
    VkBuffer index_buffer = nullptr;

    IndexBufferBind() = default;
    IndexBufferBind(CMD_TYPE type, VkBuffer index_buffer) : BasicCommand(type), index_buffer(index_buffer){};

    C_SERIALIZE2(BasicCommand, IndexBufferBind, type, index_buffer);
};

struct VertexBufferBind : BasicCommand {
    const VkVizCommandType VkVizType() override { return VkVizCommandType::VERTEX_BUFFER_BIND; };
    std::vector<VkBuffer> vertex_buffers;

    VertexBufferBind() = default;
    VertexBufferBind(CMD_TYPE type, std::vector<VkBuffer> vertex_buffers) : BasicCommand(type), vertex_buffers(vertex_buffers){};

    C_SERIALIZE2(BasicCommand, VertexBufferBind, type, vertex_buffers);
};

struct PipelineBarrierCommand : BasicCommand {
    const VkVizCommandType VkVizType() override { return VkVizCommandType::PIPELINE_BARRIER; }
    VkVizPipelineBarrier barrier;

    PipelineBarrierCommand() = default;
    PipelineBarrierCommand(CMD_TYPE type, VkVizPipelineBarrier barrier) : BasicCommand(type), barrier(std::move(barrier)){};

    C_SERIALIZE2(BasicCommand, PipelineBarrierCommand, type, barrier);
};

// This struct isn't currently used since VkVizCommandBuffers store all the descriptor set use info for draw calls in the
// DrawCommand struct.
struct BindDescriptorSetsCommand : BasicCommand {
    /*
    const VkVizCommandType VkVizType() override { return VkVizCommandType::BIND_DESCRIPTOR_SETS; }

    std::vector<VkVizDescriptorSet> bound_descriptor_sets;

    enum GraphicsOrCompute {GRAPHICS, COMPUTE};
    GraphicsOrCompute bind_point;


    C_SERIALIZE3(BasicCommand, BindDescriptorSetsCommand, type, bound_descriptor_sets, bind_point);*/

    BindDescriptorSetsCommand() = default;
    BindDescriptorSetsCommand(CMD_TYPE type, std::vector<VkVizDescriptorSet> bind_sets) : BasicCommand(type) {}
};

// A wrapper for BasicCommands. Allows one to use classes inherited from BasicCommands without working with pointers. Also handles
// serialization and deserialization of BasicCommands, ensuring that serialized and deserialized BasicCommands retain their type
// information.
class CommandWrapper {
   public:
    CommandWrapper() : command_(nullptr){};
    CommandWrapper(CommandWrapper&&) = default;
    CommandWrapper& operator=(CommandWrapper&&) = default;

    template <typename T>
    CommandWrapper(T&& command) : command_(new T(std::forward<T>(command))) {}

    template <typename T>
    CommandWrapper& operator=(T&& impl) {
        command_.reset(new T(std::forward<T>(impl)));
        return *this;
    }

    VkVizCommandType VkVizType() const { return command_->VkVizType(); }

    // kVkVizType stores what type any serialized children classes of BasicCommands were. This allows us to know which type to
    // deserialize stored commands as.
    void to_json(json& j) const {
        command_->to_json(j);
        j["kVkVizType"] = VkVizType();
    }

    void from_json(const json& j) { command_->from_json(j); }

    template<typename T>
    const T& Unwrap() const { return *dynamic_cast<T*>(command_.get()); }

    template<typename T>
    void AssertHoldsType() const { assert(dynamic_cast<T*>(command_.get())); }

    bool IsPipelineBarrier() const {
        return VkVizType() == VkVizCommandType::PIPELINE_BARRIER;
    }

    bool IsGlobalBarrier() const {
        return VkVizType() == VkVizCommandType::PIPELINE_BARRIER && Unwrap<PipelineBarrierCommand>().barrier.global_barriers.size() != 0;
    }

    std::vector<MemoryAccess> GetAllAccesses() const {
        if(VkVizType() == VkVizCommandType::ACCESS) {
            return Unwrap<Access>().accesses;
        } else if (VkVizType() == VkVizCommandType::DRAW) {
            std::vector<MemoryAccess> accesses;
            for(const auto& stage_access : Unwrap<DrawCommand>().stage_accesses) {
                for(const auto& access : stage_access.second) {
                    accesses.push_back(access);
                }
            }
            return accesses;
        }

        return {};
    }

   protected:
    std::unique_ptr<BasicCommand> command_;
};
inline void to_json(json& j, const CommandWrapper& obj) { obj.to_json(j); }

// Here we instantiate our CommandWrapper with the correct class type using the previously stored VkVizType.
inline void from_json(const json& j, CommandWrapper& obj) {
    VkVizCommandType type = j["kVkVizType"].get<VkVizCommandType>();
    switch(type) {
        case VkVizCommandType::BASIC:
            obj = BasicCommand();
            break;
        case VkVizCommandType::ACCESS:
            obj = Access();
            break;
        case VkVizCommandType::DRAW:
            obj = DrawCommand();
            break;
        case VkVizCommandType::INDEX_BUFFER_BIND:
            obj = IndexBufferBind();
            break;
        case VkVizCommandType::VERTEX_BUFFER_BIND:
            obj = VertexBufferBind();
            break;
        case VkVizCommandType::PIPELINE_BARRIER:
            obj = PipelineBarrierCommand();
            break;
        case VkVizCommandType::BIND_DESCRIPTOR_SETS:
            obj = BindDescriptorSetsCommand();
            break;
    }
    obj.from_json(j);
}

#endif  // COMMAND_H
