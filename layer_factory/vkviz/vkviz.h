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
#include "vulkan/vulkan.h"
#include "layer_factory.h"

// vkCmd tracking -- complete as of header 1.0.68
// please keep in "none, then sorted" order
// Note: grepping vulkan.h for VKAPI_CALL.*vkCmd will return all functions except vkEndCommandBuffer

enum CMD_TYPE {
    CMD_NONE,
    CMD_BEGINCOMMANDBUFFER,  // Should be the first command
    CMD_BEGINDEBUGUTILSLABELEXT,
    CMD_BEGINQUERY,
    CMD_BEGINRENDERPASS,
    CMD_BINDDESCRIPTORSETS,
    CMD_BINDINDEXBUFFER,
    CMD_BINDPIPELINE,
    CMD_BINDVERTEXBUFFERS,
    CMD_BLITIMAGE,
    CMD_CLEARATTACHMENTS,
    CMD_CLEARCOLORIMAGE,
    CMD_CLEARDEPTHSTENCILIMAGE,
    CMD_COPYBUFFER,
    CMD_COPYBUFFERTOIMAGE,
    CMD_COPYIMAGE,
    CMD_COPYIMAGETOBUFFER,
    CMD_COPYQUERYPOOLRESULTS,
    CMD_DEBUGMARKERBEGINEXT,
    CMD_DEBUGMARKERENDEXT,
    CMD_DEBUGMARKERINSERTEXT,
    CMD_DISPATCH,
    CMD_DISPATCHBASE,
    CMD_DISPATCHBASEKHR,
    CMD_DISPATCHINDIRECT,
    CMD_DRAW,
    CMD_DRAWINDEXED,
    CMD_DRAWINDEXEDINDIRECT,
    CMD_DRAWINDEXEDINDIRECTCOUNTAMD,
    CMD_DRAWINDIRECT,
    CMD_DRAWINDIRECTCOUNTAMD,
    CMD_ENDCOMMANDBUFFER,  // Should be the last command in any RECORDED cmd buffer
    CMD_ENDDEBUGUTILSLABELEXT,
    CMD_ENDQUERY,
    CMD_ENDRENDERPASS,
    CMD_EXECUTECOMMANDS,
    CMD_FILLBUFFER,
    CMD_INSERTDEBUGUTILSLABELEXT,
    CMD_NEXTSUBPASS,
    CMD_PIPELINEBARRIER,
    CMD_PROCESSCOMMANDSNVX,
    CMD_PUSHCONSTANTS,
    CMD_PUSHDESCRIPTORSETKHR,
    CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR,
    CMD_RESERVESPACEFORCOMMANDSNVX,
    CMD_RESETCOMMANDBUFFER,
    CMD_RESETEVENT,
    CMD_RESETQUERYPOOL,
    CMD_RESOLVEIMAGE,
    CMD_SETBLENDCONSTANTS,
    CMD_SETDEPTHBIAS,
    CMD_SETDEPTHBOUNDS,
    CMD_SETDEVICEMASK,
    CMD_SETDEVICEMASKKHR,
    CMD_SETDISCARDRECTANGLEEXT,
    CMD_SETEVENT,
    CMD_SETLINEWIDTH,
    CMD_SETSAMPLELOCATIONSEXT,
    CMD_SETSCISSOR,
    CMD_SETSTENCILCOMPAREMASK,
    CMD_SETSTENCILREFERENCE,
    CMD_SETSTENCILWRITEMASK,
    CMD_SETVIEWPORT,
    CMD_SETVIEWPORTWSCALINGNV,
    CMD_UPDATEBUFFER,
    CMD_WAITEVENTS,
    CMD_WRITEBUFFERMARKERAMD,
    CMD_WRITETIMESTAMP,
};

class VkViz : public layer_factory {
   public:
    // Constructor for interceptor
    VkViz() : layer_factory(this), outfile_num_(0), outfile_base_name_("vkviz_out"){};

    // This function will be called for every API call
    // void PreCallApiFunction(const char *api_name);

    VkResult PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo);
    VkResult PostCallEndCommandBuffer(VkCommandBuffer commandBuffer);
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
    VkResult PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);

   private:
    //
    std::unordered_map<VkCommandBuffer, std::vector<CMD_TYPE>> cmdbuffer_map_;
    std::ofstream outfile_;
    uint32_t outfile_num_;
    std::string outfile_base_name_;
};

#endif  // VkViz_H
