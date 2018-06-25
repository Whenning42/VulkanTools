#ifndef COMMAND_H
#define COMMAND_H

#include <vulkan.h>
#include <type_traits>
#include <fstream>
#include <memory>
#include <vector>

#include "command_enums.h"
#include "loggable.h"
#include "memory_barrier.h"
#include "memory_access.h"

class VkVizPipelineBarrier;

struct BasicCommand {
    CMD_TYPE type_ = CMD_NONE;

    BasicCommand(CMD_TYPE type) : type_(type){};
};

// A Command stores objects inherited from BasicCommand
class Command : public Loggable {
   public:
    template <typename T>
    Command(T&& impl) : Loggable(impl) {
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
};

struct IndexBufferBind : BasicCommand {
    VkBuffer index_buffer_ = nullptr;

    IndexBufferBind(CMD_TYPE type, VkBuffer index_buffer) : BasicCommand(type), index_buffer_(index_buffer){};
};

struct VertexBufferBind : BasicCommand {
    std::vector<VkBuffer> vertex_buffers_;

    VertexBufferBind(CMD_TYPE type, std::vector<VkBuffer> vertex_buffers) : BasicCommand(type), vertex_buffers_(vertex_buffers){};
};

struct PipelineBarrierCommand : BasicCommand {
    VkVizPipelineBarrier barrier_;

    PipelineBarrierCommand(CMD_TYPE type, VkVizPipelineBarrier barrier) : BasicCommand(type), barrier_(std::move(barrier)){};
};

inline void Log(const BasicCommand& cmd, Logger& logger) { logger << "Command ran: " << cmdToString(cmd.type_) << std::endl; }

inline void LogBasic(const BasicCommand& cmd, Logger& logger) { Log(cmd, logger); }

inline void Log(const Access& access, Logger& logger) {
    LogBasic(access, logger);
    for (const auto& access : access.accesses_) {
        access.Log(logger);
    }
}

inline void Log(const IndexBufferBind& bind_cmd, Logger& logger) {
    LogBasic(bind_cmd, logger);
    logger << "  Bound index buffer: " << bind_cmd.index_buffer_ << std::endl;
}

inline void Log(const VertexBufferBind& bind_cmd, Logger& logger) {
    LogBasic(bind_cmd, logger);
    logger << "  Bound vertex buffers:" << std::endl;
    for (const auto& buffer : bind_cmd.vertex_buffers_) {
        logger << "    " << buffer << std::endl;
    }
}

inline void Log(const PipelineBarrierCommand& barrier_cmd, Logger& logger) {
    LogBasic(barrier_cmd, logger);
    auto& barrier_ = barrier_cmd.barrier_;
    logger << "  Pipeline barrier" << std::endl;
    logger << "  Source stage mask: " << std::hex << std::showbase << barrier_.src_stage_mask << std::endl;
    logger << "  Destination stage mask: " << std::hex << std::showbase << barrier_.dst_stage_mask << std::endl;
    for (const auto& memory_barrier : barrier_.memory_barriers) {
        memory_barrier.Log(logger);
    }
}

#endif  // COMMAND_H
