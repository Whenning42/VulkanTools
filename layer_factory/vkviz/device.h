#include "serialize.h"
#include "shader_validation.h"

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <vulkan_core.h>

// TODO remove
#include <iostream>

enum OperationType { MAP_MEMORY, FLUSH_MAPPED_MEMORY_RANGES, INVALIDATE_MAPPED_MEMORY_RANGES, UNMAP_MEMORY };
inline std::string OperationName(OperationType type_) {
    std::vector<std::string> operations = {"Map memory", "Flush mapped memory ranges", "Invalidate mapped memory ranges", "Unmap memory"};
    return operations[type_];
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
        return serialized;
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
        return serialized;
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
    std::unordered_map<VkShaderModule, VkShaderModuleCreateInfo> shader_create_infos_;
    std::vector<const uint32_t*> shader_sources_;

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

    VkResult CreateShaderModule(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
        std::cout << "Shader module create!" << std::endl;

        VkShaderModuleCreateInfo shader_info;
        shader_info.flags = pCreateInfo->flags;
        shader_info.codeSize = pCreateInfo->codeSize;

        uint32_t* copied_source = new uint32_t[pCreateInfo->codeSize/4];
        std::memcpy(copied_source, pCreateInfo->pCode, pCreateInfo->codeSize);
        shader_info.pCode = copied_source;

        shader_create_infos_[*pShaderModule] = shader_info;
        shader_sources_.push_back(copied_source);
    }

    void PrintShaderDescriptorUses(const VkShaderModuleCreateInfo& shader_create_info, const VkPipelineShaderStageCreateInfo& stage_create_info) {
        const auto descriptor_uses = shader_module::get_descriptor_uses(shader_create_info, stage_create_info);
        std::cout << "Looking at interface for shader: " << stage_create_info.module << std::endl;
        for(const auto& use : descriptor_uses) {
            descriptor_slot_t slot = use.first;
            interface_var var = use.second;
            std::cout << "Found descriptor" << std::endl;
            std::cout << "Set: " << slot.first << std::endl;
            std::cout << "Binding: " << slot.second << std::endl;
            std::cout << "Storage Class: " << var.storage_class << std::endl;
        }
        std::cout << std::endl << std::endl;
    }

    VkResult CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
        std::cout << "Graphics pipeline create!" << std::endl;
        for(uint32_t i=0; i<createInfoCount; ++i) {
            const VkGraphicsPipelineCreateInfo& pipeline_create_info = pCreateInfos[i];
            for(uint32_t j=0; j<pipeline_create_info.stageCount; ++j) {
                const auto& stage_create_info = pipeline_create_info.pStages[j];
                PrintShaderDescriptorUses(shader_create_infos_[stage_create_info.module], stage_create_info);
            }
        }
    }

    VkResult CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
        std::cout << "Compute pipeline create!" << std::endl;
        for(uint32_t i=0; i<createInfoCount; ++i) {
            const VkComputePipelineCreateInfo& pipeline_create_info = pCreateInfos[i];
            const auto& stage_create_info = pipeline_create_info.stage;
            PrintShaderDescriptorUses(shader_create_infos_[stage_create_info.module], stage_create_info);
        }
    }
};
