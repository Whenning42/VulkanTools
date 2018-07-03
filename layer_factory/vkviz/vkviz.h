/* Copyright (C) 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Tobin Ehlis <tobine@google.com>
 */

#ifndef VkViz_H
#define VkViz_H

#include <unordered_map>
#include <iostream>
#include <fstream>
#include <thread>

#include <vulkan_core.h>
#include "layer_factory.h"
#include "render_pass.h"
#include "command_buffer.h"


// vkCmd tracking -- complete as of header 1.0.68
// please keep in "none, then sorted" order
// Note: grepping vulkan.h for VKAPI_CALL.*vkCmd will return all functions except vkEndCommandBuffer, vkBeginBuffer, and vkResetBuffer


class VkViz : public layer_factory {

   public:
    // Constructor for interceptor
    VkViz() : layer_factory(this), out_file_("vkviz_frame_start") {};

    // These functions are all implemented in vkviz.cpp.

    VkResult PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo);
    VkResult PostCallEndCommandBuffer(VkCommandBuffer commandBuffer);
    VkResult PostCallResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags);

    // Processes the submitted command buffers and updates any necessary output info.
    VkResult PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);

    // Used to track the start and end of frames.
    VkResult PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);


    VkResult PostCallCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
        device_map_.emplace(*pDevice, VkVizDevice(*pDevice));
    }

    // Create calls here
    VkResult PostCallCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                  VkBuffer* pBuffer) {
        buffer_map_.emplace(*pBuffer, VkVizBuffer(*pBuffer, device));
    }
    void PostCallDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) {
        device_map_.at(device).UnbindBufferMemory(buffer);
        assert(buffer_map_.erase(buffer));
    }

    VkResult PostCallCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                 VkImage* pImage) {
        image_map_.emplace(*pImage, VkVizImage(*pImage, device));
    }
    void PostCallDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) {
        device_map_.at(device).UnbindImageMemory(image);
        assert(image_map_.erase(image));
    }

    VkResult PostCallMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
                               VkMemoryMapFlags flags, void** ppData) {
        device_map_.at(device).MapMemory(memory, offset, size, flags, ppData);
    }

    VkResult PostCallFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) {
        device_map_.at(device).FlushMappedMemoryRanges(memoryRangeCount, pMemoryRanges);
    }

    VkResult PostCallInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                  const VkMappedMemoryRange* pMemoryRanges) {
        device_map_.at(device).InvalidateMappedMemoryRanges(memoryRangeCount, pMemoryRanges);
    }

    void PostCallUnmapMemory(VkDevice device, VkDeviceMemory memory) { device_map_.at(device).UnmapMemory(memory); }

    VkResult PostCallCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
    void PostCallDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);

    VkResult PostCallAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer*);
    void PostCallFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers);

    // end create calls


    // Tracks memory bindings.
    VkResult PostCallBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        device_map_.at(device).BindBufferMemory(buffer, memory, memoryOffset);
    }

    // Tracks memory bindings.
    VkResult PostCallBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        device_map_.at(device).BindImageMemory(image, memory, memoryOffset);
    }

    // These functions are implemented in vkviz_command_intercepts.cpp and just call the corresponding function on the
    // VkCommandBuffers corresponding VkVizCommandBuffer object.
    void PostCallCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo);
    void PostCallCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
    void PostCallCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
                                    VkSubpassContents contents);
    void PostCallCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                       const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                       const uint32_t* pDynamicOffsets);
    void PostCallCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
    void PostCallCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
    void PostCallCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                      const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
    void PostCallCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                              VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
    void PostCallCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments,
                                     uint32_t rectCount, const VkClearRect* pRects);
    void PostCallCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                    const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
    void PostCallCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                           const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
                                           const VkImageSubresourceRange* pRanges);
    void PostCallCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
                               const VkBufferCopy* pRegions);
    void PostCallCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                      VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
    void PostCallCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                              VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
    void PostCallCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                      VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
    void PostCallCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                         uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
                                         VkQueryResultFlags flags);
    void PostCallCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
    void PostCallCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer);
    void PostCallCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
    void PostCallCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void PostCallCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                 uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void PostCallCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void PostCallCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset);
    void PostCallCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                         uint32_t firstInstance);
    void PostCallCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                int32_t vertexOffset, uint32_t firstInstance);
    void PostCallCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                        uint32_t stride);
    void PostCallCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                                uint32_t stride);
    void PostCallCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                 uint32_t stride);
    void PostCallCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                         VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride);
    void PostCallCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer);
    void PostCallCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query);
    void PostCallCmdEndRenderPass(VkCommandBuffer commandBuffer);
    void PostCallCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers);
    void PostCallCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size,
                               uint32_t data);
    void PostCallCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo);
    void PostCallCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents);
    void PostCallCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
    void PostCallCmdProcessCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo);
    void PostCallCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                  uint32_t offset, uint32_t size, const void* pValues);
    void PostCallCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                         VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
                                         const VkWriteDescriptorSet* pDescriptorWrites);
    void PostCallCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer,
                                                     VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout,
                                                     uint32_t set, const void* pData);
    void PostCallCmdReserveSpaceForCommandsNVX(VkCommandBuffer commandBuffer,
                                               const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo);
    void PostCallCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);
    void PostCallCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
    void PostCallCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
    void PostCallCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]);
    void PostCallCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                 float depthBiasSlopeFactor);
    void PostCallCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds);
    void PostCallCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask);
    void PostCallCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask);
    void PostCallCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle,
                                           uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles);
    void PostCallCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask);
    void PostCallCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth);
    void PostCallCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT* pSampleLocationsInfo);
    void PostCallCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                               const VkRect2D* pScissors);
    void PostCallCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask);
    void PostCallCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference);
    void PostCallCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask);
    void PostCallCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                const VkViewport* pViewports);
    void PostCallCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                          const VkViewportWScalingNV* pViewportWScalings);
    void PostCallCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize,
                                 const void* pData);
    void PostCallCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
                               VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount,
                               const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                               const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                               const VkImageMemoryBarrier* pImageMemoryBarriers);
    void PostCallCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer,
                                         VkDeviceSize dstOffset, uint32_t marker);
    void PostCallCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
                                   uint32_t query);

   private:
    void AddRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkRenderPass* pRenderPass) {
        render_pass_map_.emplace(*pRenderPass, VkVizRenderPass(device, pCreateInfo, pRenderPass));
    }

    void RemoveRenderPass(VkDevice device, VkRenderPass render_pass) {
        assert(render_pass_map_.erase(render_pass));
    }

    VkVizRenderPass& GetRenderPass(VkRenderPass render_pass) {
        return render_pass_map_.at(render_pass);
    }

    void AddCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) {
        for (int i = 0; i < pAllocateInfo->commandBufferCount; ++i) {
            command_buffer_map_.emplace(pCommandBuffers[i], VkVizCommandBuffer(pCommandBuffers[i], pAllocateInfo->level));
        }
    }

    void RemoveCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
        for (int i = 0; i < commandBufferCount; ++i) {
            const VkCommandBuffer& command_buffer = pCommandBuffers[i];
            assert(command_buffer_map_.erase(command_buffer));
        }
    }

    VkVizCommandBuffer& GetCommandBuffer(VkCommandBuffer command_buffer) {
        return command_buffer_map_.at(command_buffer);
    }

    std::unordered_map<VkCommandBuffer, VkVizCommandBuffer> command_buffer_map_;
    std::unordered_map<VkRenderPass, VkVizRenderPass> render_pass_map_;
    std::unordered_map<VkDevice, VkVizDevice> device_map_;
    std::unordered_map<VkImage, VkVizImage> image_map_;
    std::unordered_map<VkBuffer, VkVizBuffer> buffer_map_;

    std::ofstream out_file_;
    int current_frame_ = 0;
};

#endif  // VkViz_H

/*

Might have to intercept some or all of these


VkResult DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator);
VkResult DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator);
VkResult DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator);
VkResult DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator);


VkResult CreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence;
VkResult CreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore;
VkResult CreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent;
VkResult CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool;
VkResult CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule;
VkResult CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines;
VkResult CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines;
VkResult CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout;
VkResult CreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler;
VkResult CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout;
VkResult CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool;
VkResult CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer;
VkResult CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass;
VkResult CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool;

VkResult DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator);
VkResult DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator);
VkResult DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator);
VkResult DestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator);
VkResult DestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator);
VkResult DestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator);
VkResult DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator);
VkResult DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator);
VkResult DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
VkResult DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator);
VkResult DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
VkResult DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator);
VkResult DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator);
VkResult DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
VkResult DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
*/
