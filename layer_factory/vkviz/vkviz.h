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
 * Author: William Henning <whenning@google.com>
 */

#ifndef VKVIZ_H
#define VKVIZ_H

#include <unordered_map>
#include <iostream>
#include <fstream>
#include <thread>

#include <vulkan_core.h>

#include "command_buffer.h"
#include "frame_capture.h"
#include "layer_factory.h"

class VkVizDevice;

// Note that a lot of the create resource calls
class VkViz : public layer_factory {
    // The layer has a map of all VkViz objects that are dispatchable.
    std::unordered_map<VkCommandBuffer, VkVizCommandBuffer> command_buffer_map_;
    std::unordered_map<VkDevice, VkVizDevice> device_map_;

    FrameCapturer capturer_;
    int current_frame_ = 0;

    void AddCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers,
                           VkVizDevice* device) {
        for (int i = 0; i < pAllocateInfo->commandBufferCount; ++i) {
            command_buffer_map_.emplace(pCommandBuffers[i], VkVizCommandBuffer(pCommandBuffers[i], pAllocateInfo->level, device));
        }
    }

    void RemoveCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
        for (int i = 0; i < commandBufferCount; ++i) {
            const VkCommandBuffer& command_buffer = pCommandBuffers[i];
            assert(command_buffer_map_.erase(command_buffer));
        }
    }

    VkVizCommandBuffer& GetCommandBuffer(VkCommandBuffer command_buffer) { return command_buffer_map_.at(command_buffer); }

   public:
    // Constructor for interceptor
    VkViz() : layer_factory(this) {capturer_.Begin("vkviz_frame_start");};

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

    VkResult PostCallDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo) {
        device_map_.at(device).DebugUtilsObjectNameEXT(pNameInfo);
    }

    // Device create calls here
    VkResult PostCallCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                  VkBuffer* pBuffer) {
        device_map_.at(device).CreateBuffer(pCreateInfo, pAllocator, pBuffer);
    }
    // void PostCallDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) {}

    VkResult PostCallCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkBufferView* pView) {
        device_map_.at(device).CreateBufferView(pCreateInfo, pAllocator, pView);
    }

    VkResult PostCallCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                 VkImage* pImage) {
        device_map_.at(device).CreateImage(pCreateInfo, pAllocator, pImage);
    }
    // void PostCallDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) {}

    VkResult PostCallCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) {
        device_map_.at(device).CreateImageView(pCreateInfo, pAllocator, pView);
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

    //VkResult PostCallCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
    //                                  const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass);
    //void PostCallDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);

    VkResult PostCallAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer*);
    void PostCallFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers);

    VkResult PostCallCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo,
                                  const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
        device_map_.at(device).CreateShaderModule(pCreateInfo, pAllocator, pShaderModule);
    }

    // void PostCallDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator);

    VkResult PostCallCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
        device_map_.at(device).CreateGraphicsPipelines(pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }

    VkResult PostCallCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
        device_map_.at(device).CreateComputePipelines(pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }

    //void PostCallDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator);
    // end create calls

    void vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
        device_map_.at(device).UpdateDescriptorSets(descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }

    // Tracks memory bindings.
    VkResult PostCallBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        device_map_.at(device).BindBufferMemory(buffer, memory, memoryOffset);
    }

    // Tracks memory bindings.
    VkResult PostCallBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        device_map_.at(device).BindImageMemory(image, memory, memoryOffset);
    }

    VkResult PostCallCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) {
        device_map_.at(device).CreateDescriptorSetLayout(pCreateInfo, pAllocator, pSetLayout);
    }

    VkResult PostCallAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) {
        device_map_.at(device).AllocateDescriptorSets(pAllocateInfo, pDescriptorSets);
    }

    void PostCallUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) {
        device_map_.at(device).UpdateDescriptorSets(descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }

    // vkCmd tracking -- complete as of header 1.0.68
    // please keep in "none, then sorted" order
    // Note: grepping vulkan.h for VKAPI_CALL.*vkCmd will return all functions except vkEndCommandBuffer, vkBeginBuffer, and
    // vkResetBuffer

    // All of these intercepts just call the corresponding VkViz command buffer functions.
    void PostCallCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
        GetCommandBuffer(commandBuffer).BeginDebugUtilsLabelEXT(pLabelInfo);
    }
    void PostCallCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
        GetCommandBuffer(commandBuffer).BeginQuery(queryPool, query, flags);
    }
    void PostCallCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
                                    VkSubpassContents contents) {
        GetCommandBuffer(commandBuffer).BeginRenderPass(pRenderPassBegin, contents);
    }
    void PostCallCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                       VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                       const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                       const uint32_t* pDynamicOffsets) {
        GetCommandBuffer(commandBuffer)
            .BindDescriptorSets(pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount,
                                pDynamicOffsets);
    }
    void PostCallCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
        GetCommandBuffer(commandBuffer).BindIndexBuffer(buffer, offset, indexType);
    }
    void PostCallCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
        GetCommandBuffer(commandBuffer).BindPipeline(pipelineBindPoint, pipeline);
    }
    void PostCallCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                      const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) {
        GetCommandBuffer(commandBuffer).BindVertexBuffers(firstBinding, bindingCount, pBuffers, pOffsets);
    }
    void PostCallCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                              VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) {
        GetCommandBuffer(commandBuffer)
            .BlitImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
    }
    void PostCallCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments,
                                     uint32_t rectCount, const VkClearRect* pRects) {
        GetCommandBuffer(commandBuffer).ClearAttachments(attachmentCount, pAttachments, rectCount, pRects);
    }
    void PostCallCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                    const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) {
        GetCommandBuffer(commandBuffer).ClearColorImage(image, imageLayout, pColor, rangeCount, pRanges);
    }
    void PostCallCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                           const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
                                           const VkImageSubresourceRange* pRanges) {
        GetCommandBuffer(commandBuffer).ClearDepthStencilImage(image, imageLayout, pDepthStencil, rangeCount, pRanges);
    }
    void PostCallCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
                               const VkBufferCopy* pRegions) {
        GetCommandBuffer(commandBuffer).CopyBuffer(srcBuffer, dstBuffer, regionCount, pRegions);
    }
    void PostCallCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                      VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) {
        GetCommandBuffer(commandBuffer).CopyBufferToImage(srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }
    void PostCallCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                              VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) {
        GetCommandBuffer(commandBuffer).CopyImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }
    void PostCallCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                      VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions) {
        GetCommandBuffer(commandBuffer).CopyImageToBuffer(srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
    }
    void PostCallCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                         uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
                                         VkQueryResultFlags flags) {
        GetCommandBuffer(commandBuffer)
            .CopyQueryPoolResults(queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
    }
    void PostCallCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
        GetCommandBuffer(commandBuffer).DebugMarkerBeginEXT(pMarkerInfo);
    }
    void PostCallCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).DebugMarkerEndEXT(); }
    void PostCallCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
        GetCommandBuffer(commandBuffer).DebugMarkerInsertEXT(pMarkerInfo);
    }
    void PostCallCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        GetCommandBuffer(commandBuffer).Dispatch(groupCountX, groupCountY, groupCountZ);
    }
    void PostCallCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                 uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        GetCommandBuffer(commandBuffer).DispatchBase(baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
    }
    void PostCallCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
        GetCommandBuffer(commandBuffer).DispatchBaseKHR(baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
    }
    void PostCallCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
        GetCommandBuffer(commandBuffer).DispatchIndirect(buffer, offset);
    }
    void PostCallCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                         uint32_t firstInstance) {
        GetCommandBuffer(commandBuffer).Draw(vertexCount, instanceCount, firstVertex, firstInstance);
    }
    void PostCallCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                int32_t vertexOffset, uint32_t firstInstance) {
        GetCommandBuffer(commandBuffer).DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    void PostCallCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                        uint32_t stride) {
        GetCommandBuffer(commandBuffer).DrawIndexedIndirect(buffer, offset, drawCount, stride);
    }
    void PostCallCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                                uint32_t stride) {
        GetCommandBuffer(commandBuffer)
            .DrawIndexedIndirectCountAMD(buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
    }
    void PostCallCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                 uint32_t stride) {
        GetCommandBuffer(commandBuffer).DrawIndirect(buffer, offset, drawCount, stride);
    }
    void PostCallCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                         VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
        GetCommandBuffer(commandBuffer).DrawIndirectCountAMD(buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
    }
    void PostCallCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) {
        GetCommandBuffer(commandBuffer).EndDebugUtilsLabelEXT();
    }
    void PostCallCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) {
        GetCommandBuffer(commandBuffer).EndQuery(queryPool, query);
    }
    void PostCallCmdEndRenderPass(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).EndRenderPass(); }
    void PostCallCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                                    const VkCommandBuffer* pCommandBuffers) {
        GetCommandBuffer(commandBuffer).ExecuteCommands(commandBufferCount, pCommandBuffers);
    }
    void PostCallCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size,
                               uint32_t data) {
        GetCommandBuffer(commandBuffer).FillBuffer(dstBuffer, dstOffset, size, data);
    }
    void PostCallCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
        GetCommandBuffer(commandBuffer).InsertDebugUtilsLabelEXT(pLabelInfo);
    }
    void PostCallCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
        GetCommandBuffer(commandBuffer).NextSubpass(contents);
    }
    void PostCallCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
        GetCommandBuffer(commandBuffer)
            .PipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    void PostCallCmdProcessCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) {
        GetCommandBuffer(commandBuffer).ProcessCommandsNVX(pProcessCommandsInfo);
    }
    void PostCallCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                  uint32_t offset, uint32_t size, const void* pValues) {
        GetCommandBuffer(commandBuffer).PushConstants(layout, stageFlags, offset, size, pValues);
    }
    void PostCallCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                         VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
                                         const VkWriteDescriptorSet* pDescriptorWrites) {
        GetCommandBuffer(commandBuffer)
            .PushDescriptorSetKHR(pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
    }
    void PostCallCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer,
                                                     VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout,
                                                     uint32_t set, const void* pData) {
        GetCommandBuffer(commandBuffer).PushDescriptorSetWithTemplateKHR(descriptorUpdateTemplate, layout, set, pData);
    }
    void PostCallCmdReserveSpaceForCommandsNVX(VkCommandBuffer commandBuffer,
                                               const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) {
        GetCommandBuffer(commandBuffer).ReserveSpaceForCommandsNVX(pReserveSpaceInfo);
    }
    void PostCallCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
        GetCommandBuffer(commandBuffer).ResetEvent(event, stageMask);
    }
    void PostCallCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
        GetCommandBuffer(commandBuffer).ResetQueryPool(queryPool, firstQuery, queryCount);
    }
    void PostCallCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) {
        GetCommandBuffer(commandBuffer).ResolveImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
    }
    void PostCallCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
        GetCommandBuffer(commandBuffer).SetBlendConstants(blendConstants);
    }
    void PostCallCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                 float depthBiasSlopeFactor) {
        GetCommandBuffer(commandBuffer).SetDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    }
    void PostCallCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
        GetCommandBuffer(commandBuffer).SetDepthBounds(minDepthBounds, maxDepthBounds);
    }
    void PostCallCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
        GetCommandBuffer(commandBuffer).SetDeviceMask(deviceMask);
    }
    void PostCallCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
        GetCommandBuffer(commandBuffer).SetDeviceMaskKHR(deviceMask);
    }
    void PostCallCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle,
                                           uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) {
        GetCommandBuffer(commandBuffer).SetDiscardRectangleEXT(firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
    }
    void PostCallCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
        GetCommandBuffer(commandBuffer).SetEvent(event, stageMask);
    }
    void PostCallCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
        GetCommandBuffer(commandBuffer).SetLineWidth(lineWidth);
    }
    void PostCallCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT* pSampleLocationsInfo) {
        GetCommandBuffer(commandBuffer).SetSampleLocationsEXT(pSampleLocationsInfo);
    }
    void PostCallCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                               const VkRect2D* pScissors) {
        GetCommandBuffer(commandBuffer).SetScissor(firstScissor, scissorCount, pScissors);
    }
    void PostCallCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) {
        GetCommandBuffer(commandBuffer).SetStencilCompareMask(faceMask, compareMask);
    }
    void PostCallCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
        GetCommandBuffer(commandBuffer).SetStencilReference(faceMask, reference);
    }
    void PostCallCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
        GetCommandBuffer(commandBuffer).SetStencilWriteMask(faceMask, writeMask);
    }
    void PostCallCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                const VkViewport* pViewports) {
        GetCommandBuffer(commandBuffer).SetViewport(firstViewport, viewportCount, pViewports);
    }
    void PostCallCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                          const VkViewportWScalingNV* pViewportWScalings) {
        GetCommandBuffer(commandBuffer).SetViewportWScalingNV(firstViewport, viewportCount, pViewportWScalings);
    }
    void PostCallCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize,
                                 const void* pData) {
        GetCommandBuffer(commandBuffer).UpdateBuffer(dstBuffer, dstOffset, dataSize, pData);
    }
    void PostCallCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
                               VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount,
                               const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                               const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                               const VkImageMemoryBarrier* pImageMemoryBarriers) {
        GetCommandBuffer(commandBuffer)
            .WaitEvents(eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers,
                        bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    void PostCallCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer,
                                         VkDeviceSize dstOffset, uint32_t marker) {
        GetCommandBuffer(commandBuffer).WriteBufferMarkerAMD(pipelineStage, dstBuffer, dstOffset, marker);
    }
    void PostCallCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
                                   uint32_t query) {
        GetCommandBuffer(commandBuffer).WriteTimestamp(pipelineStage, queryPool, query);
    }
};

#endif  // VKVIZ_H

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
VkResult CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout;
VkResult CreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler;
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
VkResult DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
VkResult DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator);
VkResult DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
VkResult DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator);
VkResult DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator);
VkResult DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
VkResult DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
*/
