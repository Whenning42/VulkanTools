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

using json = nlohmann::json;

class VkVizPipelineBarrier;

struct BasicCommand {
    CMD_TYPE type_ = CMD_NONE;

    BasicCommand(CMD_TYPE type) : type_(type){};
    std::string TypeString() const { return cmdToString(type_);}

    json to_json() const {
        return {{"type", type_}};
    }
    static BasicCommand from_json(const json& j) {
        return BasicCommand(j["type"].get<CMD_TYPE>());
    }
};

class Command {
  protected:
    struct concept {
        virtual ~concept() {}
        virtual std::string TypeString() const = 0;
        virtual std::unique_ptr<concept> Clone() const = 0;
        virtual json to_json() const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Cannot create a serilizable object from a const one");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        std::string TypeString() const override { return data_.TypeString(); }
        std::unique_ptr<concept> Clone() const override {
            return std::unique_ptr<model>(new model(*this));
        }

        json to_json() const override { return data_.to_json(); }
        static model from_json(const json& j) { return T::from_json(j); }

        T data_;
    };

   public:
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

    std::string TypeString() const { return impl_->TypeString(); }

    json to_json() const { return impl_->to_json(); }

    template <typename T>
    static Command from_json(const json& j) { return T::from_json(j); }

   protected:
    std::unique_ptr<concept> impl_;
};

struct Access : public BasicCommand {
    std::vector<MemoryAccess> accesses_;


    Access(CMD_TYPE type, MemoryAccess access) : BasicCommand(type) { accesses_.push_back(std::move(access)); };
    Access(CMD_TYPE type, std::vector<MemoryAccess> accesses) : BasicCommand(type), accesses_(std::move(accesses)) { };

    std::string TypeString() const { return cmdToString(type_) + " + causes one or more acesses.";}

    json to_json() const {
        json serialized = BasicCommand::to_json();
        for (const auto& access : accesses_) {
            serialized["accesses"].push_back(access.to_json());
        }
        return serialized;
    }
    static Access from_json(const json& j) {
        std::vector<MemoryAccess> accesses;
        for(int i=0; i<j["accesses"].size(); ++i) {
            accesses.push_back(MemoryAccess::from_json(j["accesses"][i]));
        }
        return Access(j["type"].get<CMD_TYPE>(), std::move(accesses));
    }
};

struct IndexBufferBind : BasicCommand {
    VkBuffer index_buffer_ = nullptr;

    IndexBufferBind(CMD_TYPE type, VkBuffer index_buffer) : BasicCommand(type), index_buffer_(index_buffer){};

    json to_json() const {
        json serialized = BasicCommand::to_json();
        serialized["index_buffer"] = reinterpret_cast<std::uintptr_t>(index_buffer_);
        return serialized;
    }
    static IndexBufferBind from_json(const json& j) {
        return IndexBufferBind(j["type"].get<CMD_TYPE>(), reinterpret_cast<VkBuffer>(j["index_buffer"].get<std::uintptr_t>()));
    }
};

struct VertexBufferBind : BasicCommand {
    std::vector<VkBuffer> vertex_buffers_;

    VertexBufferBind(CMD_TYPE type, std::vector<VkBuffer> vertex_buffers) : BasicCommand(type), vertex_buffers_(vertex_buffers){};

    json to_json() const {
        json serialized = BasicCommand::to_json();
        for (const auto& buffer : vertex_buffers_) {
            serialized["Vertex buffers"] = reinterpret_cast<uintptr_t>(buffer);
        }
        return serialized;
    }
    static VertexBufferBind from_json(const json& j) {
        std::vector<VkBuffer> vertex_buffers;
        for(int i=0; i<j["vertex_buffers_"].size(); ++i) {
            vertex_buffers.push_back(reinterpret_cast<VkBuffer>(j["vertex_buffers_"][0].get<std::uintptr_t>()));
        }
        return VertexBufferBind(j["type"].get<CMD_TYPE>(),  std::move(vertex_buffers));
    }
};

struct PipelineBarrierCommand : BasicCommand {
    VkVizPipelineBarrier barrier_;

    PipelineBarrierCommand(CMD_TYPE type, VkVizPipelineBarrier barrier) : BasicCommand(type), barrier_(std::move(barrier)){};

    json to_json() const {
        json serialized = BasicCommand::to_json();
        serialized["barrier"] = barrier_.to_json();
        return serialized;
    }
    static PipelineBarrierCommand from_json(const json& j) {
        return PipelineBarrierCommand(j["type"].get<CMD_TYPE>(), VkVizPipelineBarrier::from_json(j["barrier"]));
    }
};


#endif  // COMMAND_H
