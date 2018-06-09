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

#ifndef Vkviz_Command_Intercepts_H
#define Vkviz_Command_Intercepts_H

#include "vulkan/vulkan.h"
#include "vkviz.h"

// All of these intercepts just call the corresponding VkViz command buffer functions.

void VkViz::PostCallCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
    GetCommandBuffer(commandBuffer).BeginDebugUtilsLabelEXT(pLabelInfo);
}

void VkViz::PostCallCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    GetCommandBuffer(commandBuffer).BeginQuery(queryPool, query, flags);
}

void VkViz::PostCallCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
                                       VkSubpassContents contents) {
    GetCommandBuffer(commandBuffer).BeginRenderPass(pRenderPassBegin, contents);
}

void VkViz::PostCallCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                          VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                          const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                          const uint32_t* pDynamicOffsets) {
    GetCommandBuffer(commandBuffer)
        .BindDescriptorSets(pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount,
                            pDynamicOffsets);
}

void VkViz::PostCallCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    GetCommandBuffer(commandBuffer).BindIndexBuffer(buffer, offset, indexType);
}

void VkViz::PostCallCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    GetCommandBuffer(commandBuffer).BindPipeline(pipelineBindPoint, pipeline);
}

void VkViz::PostCallCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                         const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) {
    GetCommandBuffer(commandBuffer).BindVertexBuffers(firstBinding, bindingCount, pBuffers, pOffsets);
}

void VkViz::PostCallCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) {
    GetCommandBuffer(commandBuffer).BlitImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

void VkViz::PostCallCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                        const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects) {
    GetCommandBuffer(commandBuffer).ClearAttachments(attachmentCount, pAttachments, rectCount, pRects);
}

void VkViz::PostCallCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                       const VkClearColorValue* pColor, uint32_t rangeCount,
                                       const VkImageSubresourceRange* pRanges) {
    GetCommandBuffer(commandBuffer).ClearColorImage(image, imageLayout, pColor, rangeCount, pRanges);
}

void VkViz::PostCallCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                              const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
                                              const VkImageSubresourceRange* pRanges) {
    GetCommandBuffer(commandBuffer).ClearDepthStencilImage(image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

void VkViz::PostCallCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
                                  const VkBufferCopy* pRegions) {
    GetCommandBuffer(commandBuffer).CopyBuffer(srcBuffer, dstBuffer, regionCount, pRegions);
}

void VkViz::PostCallCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                         VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) {
    GetCommandBuffer(commandBuffer).CopyBufferToImage(srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

void VkViz::PostCallCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) {
    GetCommandBuffer(commandBuffer).CopyImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void VkViz::PostCallCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                         VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions) {
    GetCommandBuffer(commandBuffer).CopyImageToBuffer(srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

void VkViz::PostCallCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                            uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
                                            VkQueryResultFlags flags) {
    GetCommandBuffer(commandBuffer).CopyQueryPoolResults(queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

void VkViz::PostCallCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    GetCommandBuffer(commandBuffer).DebugMarkerBeginEXT(pMarkerInfo);
}

void VkViz::PostCallCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).DebugMarkerEndEXT(); }

void VkViz::PostCallCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    GetCommandBuffer(commandBuffer).DebugMarkerInsertEXT(pMarkerInfo);
}

void VkViz::PostCallCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    GetCommandBuffer(commandBuffer).Dispatch(groupCountX, groupCountY, groupCountZ);
}

void VkViz::PostCallCmdDispatchBase(VkCommandBuffer commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY,
                                    groupCountZ) {
    GetCommandBuffer(commandBuffer).DispatchBase(baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void VkViz::PostCallCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                       uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    GetCommandBuffer(commandBuffer).DispatchBaseKHR(baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

void VkViz::PostCallCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    GetCommandBuffer(commandBuffer).DispatchIndirect(buffer, offset);
}

void VkViz::PostCallCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance) {
    GetCommandBuffer(commandBuffer).Draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VkViz::PostCallCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                   int32_t vertexOffset, uint32_t firstInstance) {
    GetCommandBuffer(commandBuffer).DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VkViz::PostCallCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                           uint32_t stride) {
    GetCommandBuffer(commandBuffer).DrawIndexedIndirect(buffer, offset, drawCount, stride);
}

void VkViz::PostCallCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                   VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                                   uint32_t stride) {
    GetCommandBuffer(commandBuffer)
        .DrawIndexedIndirectCountAMD(buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void VkViz::PostCallCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                    uint32_t stride) {
    GetCommandBuffer(commandBuffer).DrawIndirect(buffer, offset, drawCount, stride);
}

void VkViz::PostCallCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                            VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                            uint32_t stride) {
    GetCommandBuffer(commandBuffer).DrawIndirectCountAMD(buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void VkViz::PostCallCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) {
    GetCommandBuffer(commandBuffer).EndDebugUtilsLabelEXT();
}

void VkViz::PostCallCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) {
    GetCommandBuffer(commandBuffer).EndQuery(queryPool, query);
}

void VkViz::PostCallCmdEndRenderPass(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).EndRenderPass(); }

void VkViz::PostCallCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers) {
    GetCommandBuffer(commandBuffer).ExecuteCommands(commandBufferCount, pCommandBuffers);
}

void VkViz::PostCallCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size,
                                  uint32_t data) {
    GetCommandBuffer(commandBuffer).FillBuffer(dstBuffer, dstOffset, size, data);
}

void VkViz::PostCallCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
    GetCommandBuffer(commandBuffer).InsertDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo);
}

void VkViz::PostCallCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    GetCommandBuffer(commandBuffer).NextSubpass(VkSubpassContents contents);
}

void VkViz::PostCallCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                       uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                       uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                       uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
    GetCommandBuffer(commandBuffer)
        .PipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                         bufferMemoryBarrierCount, VkBufferMemoryBarrier * pBufferMemoryBarriers, imageMemoryBarrierCount,
                         VkImageMemoryBarrier * pImageMemoryBarriers)
}

void VkViz::PostCallCmdProcessCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) {
    GetCommandBuffer(commandBuffer).ProcessCommandsNVX(pProcessCommandsInfo);
}

void VkViz::PostCallCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                     uint32_t offset, uint32_t size, const void* pValues) {
    GetCommandBuffer(commandBuffer).PushConstants(layout, stageFlags, offset, size, pValues);
}

void VkViz::PostCallCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                            VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
                                            const VkWriteDescriptorSet* pDescriptorWrites) {
    GetCommandBuffer(commandBuffer).PushDescriptorSetKHR(pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

void VkViz::PostCallCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer,
                                                        VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                        VkPipelineLayout layout, uint32_t set, const void* pData) {
    GetCommandBuffer(commandBuffer).PushDescriptorSetWithTemplateKHR(commandBuffer, descriptorUpdateTemplate, layout, set, pData);
}

void VkViz::PostCallCmdReserveSpaceForCommandsNVX(VkCommandBuffer commandBuffer,
                                                  const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) {
    GetCommandBuffer(commandBuffer).ReserveSpaceForCommandsNVX(pReserveSpaceInfo);
}

void VkViz::PostCallCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    GetCommandBuffer(commandBuffer).ResetEvent(event, stageMask);
}

void VkViz::PostCallCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                      uint32_t queryCount) {
    GetCommandBuffer(commandBuffer).ResetQueryPool(queryPool, firstQuery, queryCount);
}

void VkViz::PostCallCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                    VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) {
    GetCommandBuffer(commandBuffer)
        .ResolveImage(srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, VkImageResolve * pRegions);
}

void VkViz::PostCallCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    GetCommandBuffer(commandBuffer).SetBlendConstants(blendConstants);
}

void VkViz::PostCallCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                    float depthBiasSlopeFactor) {
    GetCommandBuffer(commandBuffer).SetDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void VkViz::PostCallCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    GetCommandBuffer(commandBuffer).SetDepthBounds(minDepthBounds, maxDepthBounds);
}

void VkViz::PostCallCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    GetCommandBuffer(commandBuffer).SetDeviceMask(deviceMask);
}

void VkViz::PostCallCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    GetCommandBuffer(commandBuffer).SetDeviceMaskKHR(deviceMask);
}

void VkViz::PostCallCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle,
                                              uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) {
    GetCommandBuffer(commandBuffer).SetDiscardRectangleEXT(firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
}

void VkViz::PostCallCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    GetCommandBuffer(commandBuffer).SetEvent(event, stageMask);
}

void VkViz::PostCallCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    GetCommandBuffer(commandBuffer).SetLineWidth(lineWidth);
}

void VkViz::PostCallCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT* pSampleLocationsInfo) {
    GetCommandBuffer(commandBuffer).SetSampleLocationsEXT(pSampleLocationsInfo);
}

void VkViz::PostCallCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                                  const VkRect2D* pScissors) {
    GetCommandBuffer(commandBuffer).SetScissor(firstScissor, scissorCount, VkRect2D * pScissors);
}

void VkViz::PostCallCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) {
    GetCommandBuffer(commandBuffer).SetStencilCompareMask(faceMask, compareMask);
}

void VkViz::PostCallCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    GetCommandBuffer(commandBuffer).SetStencilReference(faceMask, reference);
}

void VkViz::PostCallCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    GetCommandBuffer(commandBuffer).SetStencilWriteMask(faceMask, writeMask);
}

void VkViz::PostCallCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                   const VkViewport* pViewports) {
    GetCommandBuffer(commandBuffer).SetViewport(firstViewport, viewportCount, pViewports);
}

void VkViz::PostCallCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                             const VkViewportWScalingNV* pViewportWScalings) {
    GetCommandBuffer(commandBuffer).SetViewportWScalingNV(firstViewport, viewportCount, pViewportWScalings);
}

void VkViz::PostCallCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                    VkDeviceSize dataSize, const void* pData) {
    GetCommandBuffer(commandBuffer).UpdateBuffer(dstBuffer, dstOffset, dataSize, pData);
}

void VkViz::PostCallCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
                                  VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount,
                                  const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                  const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                  const VkImageMemoryBarrier* pImageMemoryBarriers) {
    GetCommandBuffer(commandBuffer)
        .WaitEvents(eventCount, pEvents, srcStageMask, dstStagemask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                    pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void VkViz::PostCallCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
                                            VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker) {
    GetCommandBuffer(commandBuffer).WriteBufferMarkerAMD(pipelineStage, dstBuffer, dstOffset, marker);
}

void VkViz::PostCallCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
                                      uint32_t query) {
    GetCommandBuffer(commandBuffer).WriteTimestamp(pipelineStage, queryPool, query);
}

VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    GetCommandBuffer(commandBuffer).QueueSubmit(queue, submitCount, pSubmits, fence);
}

#endif  // Vkviz_Command_Intercepts_H
