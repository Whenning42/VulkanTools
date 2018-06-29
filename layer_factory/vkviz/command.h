#ifndef COMMAND_H
#define COMMAND_H

#include <vulkan_core.h>
#include <type_traits>
#include <fstream>
#include <memory>
#include <vector>

#include "command_enums.h"
#include "serialize.h"
#include "memory_barrier.h"
#include "memory_access.h"

using json = nlohmann::json;

class VkVizPipelineBarrier;

struct BasicCommand {
    CMD_TYPE type_ = CMD_NONE;

    BasicCommand(CMD_TYPE type) : type_(type){};
    std::string TypeString() const { return cmdToString(type_);}
    json Serialize() const {
        return {"type", cmdToString};
    }
};

class Command {
  protected:
    struct concept {
        virtual ~concept() {}
        virtual std::string TypeString() const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Cannot create a serilizable object from a const one");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        std::string TypeString() const override { return data_.TypeString(); }

        T data_;
    };

   public:
    Command(const Command&) = delete;
    Command(Command&&) = default;

    Command& operator=(const Command&) = delete;
    Command& operator=(Command&&) = default;

    template <typename T>
    Command(T&& impl): impl_(new model<std::decay_t<T>>(std::forward<T>(impl))) {
        static_assert(std::is_base_of<BasicCommand, T>::value, "Commands need to be instances of the BasicCommand class");
    }

    template <typename T>
    Command& operator=(T&& impl) {
        impl_.reset(new model<std::decay_t<T>>(std::forward<T>(impl)));
        static_assert(std::is_base_of<BasicCommand, T>::value, "Commands need to be instances of the BasicCommand class");
        return *this;
    }

    std::string TypeString() const { return impl_->TypeString(); }

   protected:
    std::unique_ptr<concept> impl_;
};

struct Access : public BasicCommand {
    std::vector<MemoryAccess> accesses_;

    Access(CMD_TYPE type, MemoryAccess access) : BasicCommand(type), accesses_({access}){};
    Access(CMD_TYPE type, std::vector<MemoryAccess> accesses) : BasicCommand(type), accesses_(accesses){};

    std::string TypeString() const { return cmdToString(type_) + " + causes one or more acesses.";}
    json Serialize() const {
        json serialized = BasicCommand::Serialize();
        for (const auto& access : accesses_) {
            serialized["Memory accesses"].push_back(access.Serialize());
        }
        return serialized;
    }
};

struct IndexBufferBind : BasicCommand {
    VkBuffer index_buffer_ = nullptr;

    IndexBufferBind(CMD_TYPE type, VkBuffer index_buffer) : BasicCommand(type), index_buffer_(index_buffer){};

    json Serialize() const {
        json serialized = BasicCommand::Serialize();
        serialized["Index buffer"] = (uint64_t)(index_buffer_);
        return serialized;
    }
};

struct VertexBufferBind : BasicCommand {
    std::vector<VkBuffer> vertex_buffers_;

    VertexBufferBind(CMD_TYPE type, std::vector<VkBuffer> vertex_buffers) : BasicCommand(type), vertex_buffers_(vertex_buffers){};

    json Serialize() const {
        //json serialized = BasicCommand::Serialize();
        //serialized["Vertex buffers"] = vertex_buffers_;
        //return serialized;
        return {};
    }
};

struct PipelineBarrierCommand : BasicCommand {
    VkVizPipelineBarrier barrier_;

    PipelineBarrierCommand(CMD_TYPE type, VkVizPipelineBarrier barrier) : BasicCommand(type), barrier_(std::move(barrier)){};

    json Serialize() const {
        json serialized = BasicCommand::Serialize();
        serialized["Pipeline barrier"] = barrier_.Serialize();
    }
};


#endif  // COMMAND_H
