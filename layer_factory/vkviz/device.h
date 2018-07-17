#include "serialize.h"
#include "descriptor_set.h"
#include "shader.h"

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <vulkan_core.h>

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

enum PipelineType {GRAPHICS, COMPUTE};

struct PipelineStage {
    PipelineType type;
    VkShaderStageFlagBits stage_flag;
    std::vector<DescriptorUse> descriptor_uses;
};
SERIALIZE3(PipelineStage, type, stage_flag, descriptor_uses);

class VkVizDevice {
    VkDevice device_;
    //std::unordered_map<VkImage, VkVizMemoryRegion> image_bindings_;
    //std::unordered_map<VkBuffer, VkVizMemoryRegion> buffer_bindings_;
    std::unordered_map<VkImageView, VkImage> images_for_views_;
    std::unordered_map<VkBufferView, VkBuffer> buffers_for_views_;
    std::unordered_map<VkShaderModule, VkShaderModuleCreateInfo> shader_create_info_;
    std::unordered_map<VkDescriptorSetLayout, VkVizDescriptorSetLayoutCreateInfo> set_layout_info_;
    std::unordered_map<VkDescriptorSet, VkVizDescriptorSet> descriptor_sets_;
    std::unordered_map<VkPipeline, std::vector<PipelineStage>> pipelines_;

    std::vector<const uint32_t*> shader_sources_;
    std::vector<Operation> operations_;

   public:
    VkVizDevice(VkDevice device) : device_(device) {}

    VkResult BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        // Could probably cache requirements from an intercepted GetReqs call
        //VkMemoryRequirements requirements;
        //vkGetImageMemoryRequirements(device_, image, &requirements);
        //assert(image_bindings_.find(image) == image_bindings_.end());
        //image_bindings_[image] = {memoryOffset, requirements.size};
    }

    void UnbindImageMemory(VkImage image) { /* assert(image_bindings_.erase(image)); */ }

    VkResult BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        // Could probably cache requirements from an intercepted GetReqs call
        //VkMemoryRequirements requirements;
        //vkGetBufferMemoryRequirements(device_, buffer, &requirements);
        //assert(buffer_bindings_.find(buffer) == buffer_bindings_.end());
        //buffer_bindings_[buffer] = {memoryOffset, requirements.size};
    }

    void UnbindBufferMemory(VkBuffer buffer) { /* assert(buffer_bindings_.erase(buffer)); */ }

    // We don't track the state of images, just their uses.
    void CreateImage(const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) {}

    // We don't yet track the state of buffers, just their uses.
    void CreateBuffer(const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) {}

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
        VkShaderModuleCreateInfo shader_info;
        shader_info.flags = pCreateInfo->flags;
        shader_info.codeSize = pCreateInfo->codeSize;

        uint32_t* copied_source = new uint32_t[pCreateInfo->codeSize/4];
        std::memcpy(copied_source, pCreateInfo->pCode, pCreateInfo->codeSize);
        shader_info.pCode = copied_source;

        shader_create_info_[*pShaderModule] = shader_info;
        shader_sources_.push_back(copied_source);
    }

    VkResult CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
        for(uint32_t i=0; i<createInfoCount; ++i) {
            const VkGraphicsPipelineCreateInfo& pipeline_create_info = pCreateInfos[i];
            std::vector<PipelineStage> stages;

            for(uint32_t j=0; j<pipeline_create_info.stageCount; ++j) {
                const auto& stage_create_info = pipeline_create_info.pStages[j];
                PipelineStage stage;
                stage.type = GRAPHICS;
                stage.stage_flag = stage_create_info.stage;
                stage.descriptor_uses = GetShaderDescriptorUses(shader_create_info_[stage_create_info.module], stage_create_info);

                stages.push_back(stage);
            }

            pipelines_.emplace(std::make_pair(pPipelines[i], std::move(stages)));
        }
    }

    VkResult CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
        for(uint32_t i=0; i<createInfoCount; ++i) {
            const VkComputePipelineCreateInfo& pipeline_create_info = pCreateInfos[i];
            const auto& stage_create_info = pipeline_create_info.stage;

            PipelineStage stage;
            stage.type = COMPUTE;
            stage.stage_flag = stage_create_info.stage;
            stage.descriptor_uses = GetShaderDescriptorUses(shader_create_info_[stage_create_info.module], stage_create_info);

            std::vector<PipelineStage> stages = {stage};
            pipelines_.emplace(std::make_pair(pPipelines[i], std::move(stages)));
        }
    }

    VkResult CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks*, VkDescriptorSetLayout* pSetLayout) {
        set_layout_info_.emplace(std::make_pair(*pSetLayout, VkVizDescriptorSetLayoutCreateInfo(*pCreateInfo)));
    }

    VkResult AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) {
        const VkDescriptorSetAllocateInfo& allocate_info = *pAllocateInfo;
        for(uint32_t i=0; i<allocate_info.descriptorSetCount; ++i) {
            const VkDescriptorSet handle = pDescriptorSets[i];
            const VkVizDescriptorSetLayoutCreateInfo& layout_create_info = set_layout_info_.at(allocate_info.pSetLayouts[i]);
            descriptor_sets_.emplace(std::make_pair(handle, VkVizDescriptorSet(layout_create_info, handle)));
        }
    }

    void UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
        // Fills all array elements passed stepping through bindings?
        for(uint32_t i=0; i<descriptorWriteCount; ++i) {
            const VkWriteDescriptorSet& write = pDescriptorWrites[i];

            DescriptorIterator it = descriptor_sets_.at(write.dstSet).GetIt(write.dstBinding, write.dstArrayElement);
            for(uint32_t j=0; j<write.descriptorCount; ++j, ++it) {
                if(VkVizDescriptorSet::IsImage(write.descriptorType)) {
                    *it = VkVizDescriptor(write.pImageInfo[j].imageView);
                } else if (VkVizDescriptorSet::IsBuffer(write.descriptorType)) {
                    *it = VkVizDescriptor(write.pBufferInfo[j].buffer);
                } else {
                    assert(VkVizDescriptorSet::IsBufferView(write.descriptorType));
                    *it = VkVizDescriptor(write.pTexelBufferView[j]);
                }
            }
        }

        for(uint32_t i=0; i<descriptorCopyCount; ++i) {
            const VkCopyDescriptorSet& copy = pDescriptorCopies[i];

            DescriptorIterator src_it = descriptor_sets_.at(copy.srcSet).GetIt(copy.srcBinding, copy.srcArrayElement);
            DescriptorIterator dst_it = descriptor_sets_.at(copy.dstSet).GetIt(copy.dstBinding, copy.dstArrayElement);
            for(uint32_t j=0; j<copy.descriptorCount; ++j, ++src_it, ++dst_it) {
                *dst_it = *src_it;
            }
        }
    }

    void CreateImageView(const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) {
        images_for_views_.emplace(std::make_pair(*pView, pCreateInfo->image));
    }

    VkImage ImageFromView(VkImageView view) {
        return images_for_views_.at(view);
    }

    void CreateBufferView(const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) {
        buffers_for_views_.emplace(std::make_pair(*pView, pCreateInfo->buffer));
    }

    VkBuffer BufferFromView(VkBufferView view) {
        return buffers_for_views_.at(view);
    }

    const VkVizDescriptorSet& GetVkVizDescriptorSet(VkDescriptorSet set) const {
        return descriptor_sets_.at(set);
    }

    const std::vector<PipelineStage> GetPipelineStages(VkPipeline pipeline) const {
        return pipelines_.at(pipeline);
    }
};
