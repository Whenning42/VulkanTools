#include "device.h"

VkResult VkVizDevice::CreateShaderModule(const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                         VkShaderModule* pShaderModule) {
    VkShaderModuleCreateInfo shader_info;
    shader_info.flags = pCreateInfo->flags;
    shader_info.codeSize = pCreateInfo->codeSize;

    uint32_t* copied_source = new uint32_t[pCreateInfo->codeSize / 4];
    std::memcpy(copied_source, pCreateInfo->pCode, pCreateInfo->codeSize);
    shader_info.pCode = copied_source;

    shader_create_info_[*pShaderModule] = shader_info;
    shader_sources_.push_back(copied_source);
}

VkResult VkVizDevice::CreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                              const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                              const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    for (uint32_t i = 0; i < createInfoCount; ++i) {
        const VkGraphicsPipelineCreateInfo& pipeline_create_info = pCreateInfos[i];
        std::vector<PipelineStage> stages;

        for (uint32_t j = 0; j < pipeline_create_info.stageCount; ++j) {
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

VkResult VkVizDevice::CreateComputePipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                             const VkComputePipelineCreateInfo* pCreateInfos,
                                             const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
    for (uint32_t i = 0; i < createInfoCount; ++i) {
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

VkResult VkVizDevice::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks*,
                                                VkDescriptorSetLayout* pSetLayout) {
    set_layout_info_.emplace(std::make_pair(*pSetLayout, VkVizDescriptorSetLayoutCreateInfo(*pCreateInfo)));
}

VkResult VkVizDevice::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) {
    const VkDescriptorSetAllocateInfo& allocate_info = *pAllocateInfo;
    for (uint32_t i = 0; i < allocate_info.descriptorSetCount; ++i) {
        const VkDescriptorSet handle = pDescriptorSets[i];
        const VkVizDescriptorSetLayoutCreateInfo& layout_create_info = set_layout_info_.at(allocate_info.pSetLayouts[i]);
        descriptor_sets_.emplace(std::make_pair(handle, VkVizDescriptorSet(layout_create_info, handle)));
    }
}

void VkVizDevice::UpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites,
                                       uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
    for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
        const VkWriteDescriptorSet& write = pDescriptorWrites[i];

        DescriptorIterator it = descriptor_sets_.at(write.dstSet).GetIt(write.dstBinding, write.dstArrayElement);
        for (uint32_t j = 0; j < write.descriptorCount; ++j, ++it) {
            if (VkVizDescriptorSet::IsImage(write.descriptorType)) {
                *it = VkVizDescriptor(write.pImageInfo[j].imageView);
            } else if (VkVizDescriptorSet::IsBuffer(write.descriptorType)) {
                *it = VkVizDescriptor(write.pBufferInfo[j].buffer);
            } else {
                assert(VkVizDescriptorSet::IsBufferView(write.descriptorType));
                *it = VkVizDescriptor(write.pTexelBufferView[j]);
            }
        }
    }

    for (uint32_t i = 0; i < descriptorCopyCount; ++i) {
        const VkCopyDescriptorSet& copy = pDescriptorCopies[i];

        DescriptorIterator src_it = descriptor_sets_.at(copy.srcSet).GetIt(copy.srcBinding, copy.srcArrayElement);
        DescriptorIterator dst_it = descriptor_sets_.at(copy.dstSet).GetIt(copy.dstBinding, copy.dstArrayElement);
        for (uint32_t j = 0; j < copy.descriptorCount; ++j, ++src_it, ++dst_it) {
            *dst_it = *src_it;
        }
    }
}
