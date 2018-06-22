#ifndef COMMAND_H
#define COMMAND_H

#include <vulkan.h>
#include <type_traits>
#include <fstream>
#include <memory>
#include <vector>

#include "command_enums.h"
#include "memory_barrier.h"
#include "memory_access.h"

class VkVizPipelineBarrier;

// Testing
typedef std::ofstream Logger;

class Command {
    struct concept {
        virtual ~concept() {}
        virtual void CallLog(Logger & logger) const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Takes non-const Commands only");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        void CallLog(Logger& logger) const override { Log(data_, logger); }

        T data_;
    };

   public:
    Command(const Command&) = delete;
    Command(Command&&) = default;

    template <typename T>
    Command(T&& impl) : impl_(new model<std::decay_t<T>>(std::forward<T>(impl))) {}

    Command& operator=(const Command&) = delete;
    Command& operator=(Command&&) = default;

    template <typename T>
    Command& operator=(T&& impl) {
        impl_.reset(new model<std::decay_t<T>>(std::forward<T>(impl)));
        return *this;
    }

    friend inline void LogCommand(const Command& cmd, Logger& logger) { cmd.impl_->CallLog(logger); }

   private:
    std::unique_ptr<concept> impl_;
};

struct BasicCommand {
    CMD_TYPE type_ = CMD_NONE;

    BasicCommand(CMD_TYPE type) : type_(type){};
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

inline void Log(const BasicCommand& cmd, Logger& logger) { logger << "Command ran: " << cmdToString(cmd.type_) << std::endl; }
inline void LogB(const BasicCommand& cmd, Logger& logger) { Log(cmd, logger); }

inline void Log(const Access& access, Logger& logger) {
    LogB(access, logger);
    for (const auto& access : access.accesses_) {
        access.Log(logger);
    }
}

inline void Log(const IndexBufferBind& bind_cmd, Logger& logger) {
    LogB(bind_cmd, logger);
    logger << "  Bound index buffer: " << bind_cmd.index_buffer_ << std::endl;
}

inline void Log(const VertexBufferBind& bind_cmd, Logger& logger) {
    LogB(bind_cmd, logger);
    logger << "  Bound vertex buffers:" << std::endl;
    for (const auto& buffer : bind_cmd.vertex_buffers_) {
        logger << "    " << buffer << std::endl;
    }
}

struct PipelineBarrierCommand : BasicCommand {
    VkVizPipelineBarrier barrier_;

    PipelineBarrierCommand(CMD_TYPE type, VkVizPipelineBarrier barrier) : BasicCommand(type), barrier_(std::move(barrier)){};
};

inline void Log(const PipelineBarrierCommand& barrier_cmd, Logger& logger) {
    LogB(barrier_cmd, logger);
    auto& barrier_ = barrier_cmd.barrier_;
    logger << "  Pipeline barrier" << std::endl;
    logger << "  Source stage mask: " << std::hex << std::showbase << barrier_.src_stage_mask << std::endl;
    logger << "  Destination stage mask: " << std::hex << std::showbase << barrier_.dst_stage_mask << std::endl;
    for (const auto& memory_barrier : barrier_.memory_barriers) {
        memory_barrier.Log(logger);
    }
}

#endif  // COMMAND_H
