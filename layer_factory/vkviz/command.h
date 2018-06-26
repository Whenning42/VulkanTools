#ifndef COMMAND_H
#define COMMAND_H

#include <vulkan.h>
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
    json Serialize() const {
        return {"type", cmdToString};
    }
};

// A Command stores objects inherited from BasicCommand
class Command : public Serializable {
   public:
    template <typename T>
    Command(T&& impl) : Serializable(impl) {
        static_assert(std::is_base_of<BasicCommand, T>::value, "Commands need to be instances of the BasicCommand class");
    }

    template <typename T>
    Command& operator=(T&& impl) {
        *this = Loggable(impl);
        static_assert(std::is_base_of<BasicCommand, T>::value, "Commands need to be instances of the BasicCommand class");
        return *this;
    }
};

struct Access : public BasicCommand {
    std::vector<MemoryAccess> accesses_;

    Access(CMD_TYPE type, MemoryAccess access) : BasicCommand(type), accesses_({access}){};
    Access(CMD_TYPE type, std::vector<MemoryAccess> accesses) : BasicCommand(type), accesses_(accesses){};

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
        json serialized = BasicCommand::Serialize();
        serialized["Vertex buffers"] = vertex_buffers_;
        return serialized;
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
