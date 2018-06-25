#include <unordered_map>
#include <cassert>
#include <vulkan.h>

enum OperationType { MAP_MEMORY, FLUSH_MAPPED_MEMORY_RANGES, INVALIDATE_MAPPED_MEMORY_RANGES, UNMAP_MEMORY };

namespace Op {
struct MapMemory {
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
    OperationType type = OperationType::MAP_MEMORY;
};

struct FlushMappedMemoryRanges {
    std::vector<VkMappedMemoryRange> memory_ranges;
    OperationType type = OperationType::FLUSH_MAPPED_MEMORY_RANGES;
};

struct InvalidateMappedMemoryRanges {
    std::vector<VkMappedMemoryRange> memory_ranges;
    OperationType type = OperationType::INVALIDATE_MAPPED_MEMORY_RANGES;
};

struct UnmapMemory {
    VkDeviceMemory memory;
    OperationType type = OperationType::UNMAP_MEMORY;
};
}  // namespace Op

void Log(Op::MapMemory map, Logger& log) {
    log << "  Map memory" << std::endl;
    log << "    Memory: " << map.memory;
    log << "    Offset: " << map.offset << ", Size: " << map.size << std::endl;
}

void Log(Op::FlushMappedMemoryRanges flush, Logger& log) {
    log << "  Flush memory ranges" << std::endl;
    for (const auto& range : flush.memory_ranges) {
        log << "    Memory: " << range.memory;
        log << "    Offset: " << range.offset << ", Size: " << range.size << std::endl;
    }
}

void Log(Op::InvalidateMappedMemoryRanges invalidate, Logger& log) {
    log << "  Invalidate memory ranges" << std::endl;
    for (const auto& range : invalidate.memory_ranges) {
        log << "    Memory: " << range.memory;
        log << "    Offset: " << range.offset << ", Size: " << range.size << std::endl;
    }
}

void Log(Op::UnmapMemory unmap, Logger& log) { log << "  Unmap memory: " << unmap.memory; }

// A Operation stores objects inherited from BasicOperation
class Operation : public Loggable {
   public:
    template <typename T>
    Operation(T&& impl) : Loggable(impl) {
        // static_assert(std::is_base_of<Op::BasicOperation, T>::value, "Operations need to be instances of the BasicOperation
        // class");
    }

    template <typename T>
    Operation& operator=(T&& impl) {
        *this = Loggable(impl);
        // static_assert(std::is_base_of<Op::BasicOperation, T>::value, "Operations need to be instances of the BasicOperation
        // class");
        return *this;
    }
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
