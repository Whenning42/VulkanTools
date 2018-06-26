#include "serialize.h"

#include <unordered_map>
#include <cassert>
#include <vulkan.h>

enum OperationType { MAP_MEMORY, FLUSH_MAPPED_MEMORY_RANGES, INVALIDATE_MAPPED_MEMORY_RANGES, UNMAP_MEMORY };
inline std::string OperationName(OperationType type_) {
    std::vector<std::string> operations = {"Map memory", "Flush mapped memory ranges", "Invalidate mapped memory ranges", "Unmap memory"};
}

inline json SerializeRange(VkMappedMemoryRange range) {
//    return {{"memory", range.memory}, {"offset", range.offset}, {"size", range.size}};
    return {};
}

namespace Op {
struct MapMemory {
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    OperationType type = OperationType::MAP_MEMORY;

    std::string TypeName() const { return "Map Memory"; }
    json Serialize() const {
//        return {{"type", type}, {"memory", memory}, {"offset", offset}, {"size", size}};
        return {};
    }
};

struct FlushMappedMemoryRanges {
    std::vector<VkMappedMemoryRange> memory_ranges;
    OperationType type = OperationType::FLUSH_MAPPED_MEMORY_RANGES;

    std::string TypeName() const { return "Flush Mapped Memory Ranges"; }
    json Serialize() const {
        json serialized = {{"type", type}};
        for (const auto& range : memory_ranges) {
            serialized["memory ranges"].push_back(SerializeRange(range));
        }
    }
};

struct InvalidateMappedMemoryRanges {
    std::vector<VkMappedMemoryRange> memory_ranges;
    OperationType type = OperationType::INVALIDATE_MAPPED_MEMORY_RANGES;

    std::string TypeName() const { return "Invalidate Mapped Memory Ranges"; }
    json Serialize() const {
        json serialized = {{"type", type}};
        for (const auto& range : memory_ranges) {
            serialized["memory ranges"].push_back(SerializeRange(range));
        }
    }
};

struct UnmapMemory {
    VkDeviceMemory memory;
    OperationType type = OperationType::UNMAP_MEMORY;

    std::string TypeName() const { return "Unmap Memory"; }
    json Serialize() const {
        //json serialized = {{"type", type}, {"memory", memory}};
        return {};
    }
};
}  // namespace Op

class Operation {
  protected:
    struct concept {
        virtual ~concept() {}
        virtual std::string TypeName() const = 0;
    };

    template <typename T>
    struct model : public concept {
        static_assert(!std::is_const<T>::value, "Cannot create a serilizable object from a const one");
        model() = default;
        model(const T& other) : data_(other) {}
        model(T&& other) : data_(std::move(other)) {}

        std::string TypeName() const override { return data_.TypeName(); }

        T data_;
    };

   public:
    Operation(const Operation&) = delete;
    Operation(Operation&&) = default;

    Operation& operator=(const Operation&) = delete;
    Operation& operator=(Operation&&) = default;

    template <typename T>
    Operation(T&& impl) : impl_(new model<std::decay_t<T>>(std::forward<T>(impl))) {}

    template <typename T>
    Operation& operator=(T&& impl) {
        impl_.reset(new model<std::decay_t<T>>(std::forward<T>(impl)));
        return *this;
    }

    std::string TypeName() const { return impl_->TypeName(); }

   protected:
    std::unique_ptr<concept> impl_;
};

struct VkVizMemoryRegion {
    VkDeviceSize offset;
    VkDeviceSize size;
};

class VkVizDevice {
    VkDevice device_;
    std::unordered_map<VkImage, VkVizMemoryRegion> image_bindings_;
    std::unordered_map<VkBuffer, VkVizMemoryRegion> buffer_bindings_;
    std::vector<Operation> operations_;

   public:
    VkVizDevice(VkDevice device) : device_(device) {}

    VkResult BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        // Could probably cache requirements from an intercepted GetReqs call
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device_, image, &requirements);
        assert(image_bindings_.find(image) == image_bindings_.end());
        image_bindings_[image] = {memoryOffset, requirements.size};
    }

    void UnbindImageMemory(VkImage image) { assert(image_bindings_.erase(image)); }

    VkResult BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        // Could probably cache requirements from an intercepted GetReqs call
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device_, buffer, &requirements);
        assert(buffer_bindings_.find(buffer) == buffer_bindings_.end());
        buffer_bindings_[buffer] = {memoryOffset, requirements.size};
    }

    void UnbindBufferMemory(VkBuffer buffer) { assert(buffer_bindings_.erase(buffer)); }

    VkResult MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
        operations_.push_back(Op::MapMemory{memory, offset, size});
    }

    VkResult FlushMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) {
        operations_.push_back(
            Op::FlushMappedMemoryRanges{std::vector<VkMappedMemoryRange>(pMemoryRanges, pMemoryRanges + memoryRangeCount)});
    }

    VkResult InvalidateMappedMemoryRanges(uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) {
        operations_.push_back(
            Op::InvalidateMappedMemoryRanges{std::vector<VkMappedMemoryRange>(pMemoryRanges, pMemoryRanges + memoryRangeCount)});
    }

    void UnmapMemory(VkDeviceMemory memory) { operations_.push_back(Op::UnmapMemory{memory}); }
};
