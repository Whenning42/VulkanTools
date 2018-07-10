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

enum class VkVizCommandType {
    BASIC,
    ACCESS,
    INDEX_BUFFER_BIND,
    VERTEX_BUFFER_BIND,
    PIPELINE_BARRIER,
    BIND_DESCRIPTOR_SETS
};

struct BasicCommand {
    static const VkVizCommandType VkVizType() { return VkVizCommandType::BASIC; };
    CMD_TYPE type_ = CMD_NONE;

    BasicCommand() : type_(CMD_NONE) {};
    BasicCommand(CMD_TYPE type) : type_(type){};
    std::string CmdTypeString() const { return cmdToString(type_);}
};
SERIALIZE(BasicCommand, CMD_TYPE, type_);

struct Access : BasicCommand {
    static const VkVizCommandType VkVizType() { return VkVizCommandType::ACCESS; };
    std::vector<MemoryAccess> accesses_;

    Access() = default;
    Access(CMD_TYPE type, MemoryAccess access) : BasicCommand(type) { accesses_.push_back(std::move(access)); };
    Access(CMD_TYPE type, std::vector<MemoryAccess> accesses) : BasicCommand(type), accesses_(std::move(accesses)) { };

    std::string CmdTypeString() const { return cmdToString(type_) + " + causes one or more acesses.";}
};
SERIALIZE2(Access, CMD_TYPE, type_, std::vector<MemoryAccess>, accesses_);

struct IndexBufferBind : BasicCommand {
    static const VkVizCommandType VkVizType() { return VkVizCommandType::INDEX_BUFFER_BIND; };
    VkBuffer index_buffer_ = nullptr;

    IndexBufferBind() = default;
    IndexBufferBind(CMD_TYPE type, VkBuffer index_buffer) : BasicCommand(type), index_buffer_(index_buffer){};
};
SERIALIZE2(IndexBufferBind, CMD_TYPE, type_, VkBuffer, index_buffer_);

struct VertexBufferBind : BasicCommand {
    static const VkVizCommandType VkVizType() { return VkVizCommandType::VERTEX_BUFFER_BIND; };
    std::vector<VkBuffer> vertex_buffers_;

    VertexBufferBind() = default;
    VertexBufferBind(CMD_TYPE type, std::vector<VkBuffer> vertex_buffers) : BasicCommand(type), vertex_buffers_(vertex_buffers){};
};
SERIALIZE2(VertexBufferBind, CMD_TYPE, type_, std::vector<VkBuffer>, vertex_buffers_);

struct PipelineBarrierCommand : BasicCommand {
    static const VkVizCommandType VkVizType() { return VkVizCommandType::PIPELINE_BARRIER; }
    VkVizPipelineBarrier barrier_;

    PipelineBarrierCommand() = default;
    PipelineBarrierCommand(CMD_TYPE type, VkVizPipelineBarrier barrier) : BasicCommand(type), barrier_(std::move(barrier)){};
};
SERIALIZE2(PipelineBarrierCommand, CMD_TYPE, type_, VkVizPipelineBarrier, barrier_);

struct BindDescriptorSets : BasicCommand {
    static const VkVizCommandType VkVizType() { return VkVizCommandType::BIND_DESCRIPTOR_SETS; }

    // Currently stores all bound descriptor sets as opposed to just the changed ones. This allows deserialization to be stateless.
    std::vector<VkVizDescriptorSet> bound_descriptor_sets_;

    enum GraphicsOrCompute {GRAPHICS, COMPUTE};
    GraphicsOrCompute bind_point;

    BindDescriptorSets() =  default;
    BindDescriptorSets(CMD_TYPE type, std::vector<VkVizDescriptorSet> sets): BasicCommand(type) {
        for(const auto& set : bound_descriptor_sets_) {
            //bound_descriptor_sets_.push_back(set.Clone());
        }
    }
};
SERIALIZE2(BindDescriptorSets, CMD_TYPE, type_, std::vector<VkVizDescriptorSet>, bound_descriptor_sets_);

class Command {
  protected:
    struct concept {
        virtual ~concept() {}
        virtual std::string CmdTypeString() const = 0;
        virtual VkVizCommandType VkVizType() const = 0;
        virtual std::unique_ptr<concept> Clone() const = 0;
        //virtual void to_json(json& j, const concept& obj) const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Cannot create a Command from a const instance");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        std::string CmdTypeString() const override { return data_.CmdTypeString(); }
        std::unique_ptr<concept> Clone() const override {
            return std::unique_ptr<model>(new model(*this));
        }

        void to_json(json& j, const T& obj) const {
            to_json(j, obj);
            j["kVkVizType"] = T::VkVizType();
        }

        VkVizCommandType VkVizType() const override { return T::VkVizType(); }

        T data_;
    };

   public:
    Command(): impl_(nullptr) {};
    Command(const Command& other) : impl_(other.impl_->Clone()) {}
    Command(Command&&) = default;

    Command& operator=(Command&&) = default;
    Command& operator=(const Command& c) {
        impl_ = c.impl_->Clone();
        return *this;
    }

    template <typename T>
    Command(T&& impl): impl_(new model<std::decay_t<T>>(std::forward<T>(impl))) {
        //static_assert(std::is_base_of<BasicCommand, T>::value, "Commands need to be instances of the BasicCommand class");
    }

    template <typename T>
    Command& operator=(T&& impl) {
        impl_.reset(new model<std::decay_t<T>>(std::forward<T>(impl)));
        //static_assert(std::is_base_of<BasicCommand, T>::value, "Commands need to be instances of the BasicCommand class");
        return *this;
    }

    std::string CmdTypeString() const { return impl_->CmdTypeString(); }
    std::string Name() const { return CmdTypeString(); }

   protected:
    std::unique_ptr<concept> impl_;
};
inline void to_json(json& j, const Command& obj) {
    return to_json(j, obj);
}
inline void from_json(const json& j, Command& obj) {
    VkVizCommandType type = j["kVkVizType"].get<VkVizCommandType>();
    switch(type) {
        case VkVizCommandType::BASIC:
            obj = j.get<BasicCommand>();
        case VkVizCommandType::ACCESS:
            obj = j.get<Access>();
        case VkVizCommandType::INDEX_BUFFER_BIND:
            obj = j.get<IndexBufferBind>();
        case VkVizCommandType::VERTEX_BUFFER_BIND:
            obj = j.get<VertexBufferBind>();
        case VkVizCommandType::PIPELINE_BARRIER:
            obj = j.get<PipelineBarrierCommand>();
        case VkVizCommandType::BIND_DESCRIPTOR_SETS:
            obj = j.get<BindDescriptorSets>();
        /*
        case VkVizCommandType::BASIC:
            return BasicCommand::from_json(j);
        case VkVizCommandType::ACCESS:
            return Access::from_json(j);
        case VkVizCommandType::INDEX_BUFFER_BIND:
            return IndexBufferBind::from_json(j);
        case VkVizCommandType::VERTEX_BUFFER_BIND:
            return VertexBufferBind::from_json(j);
        case VkVizCommandType::PIPELINE_BARRIER:
            return PipelineBarrierCommand::from_json(j);
        case VkVizCommandType::BIND_DESCRIPTOR_SETS:
            return BindDescriptorSets::from_json(j);
            */
    }
}

#endif  // COMMAND_H
