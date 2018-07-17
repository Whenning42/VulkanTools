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

#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "command.h"
#include "memory_barrier.h"
#include "memory_access.h"

#include <cassert>
#include <vector>
#include <vulkan_core.h>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <memory>

/* This is the beginning of an implementation of tracking which images attachments reference. This is necessary to track which
 * images shaders are writing to and to track which images VkCmdClearColorImage commands affect.

class VkVizAttachment {
    VkVizImageView image_view;

   public:
    VkVizImageView& ImageView(){return image_view;}
};

class VkVizSubPass {
    std::vector<VkVizAttachment> color_attachments;

    bool has_depth_attachment;
    VkVizAttachment depth_attachment;

    bool has_stencil_attachment;
    VkVizAttachment stencil_attachment;

   public:
    std::vector<VkVizAttachment>& ColorAttachments() { return color_attachments; }

    bool HasDepthAttachment() { return has_depth_attachment; }
    VkVizAttachment& DepthAttachment() { return depth_attachment; }

    bool HasStencilAttachment() { return has_stencil_attachment; }
    VkVizAttachment& StencilAttachment() { return stencil_attachment; }
};

class VkVizRenderPass {
    VkRenderPass render_pass;
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkSubpassDescription> subpasses;
    std::vector<VkSubpassDependency> dependencies;
    // static std::unordered_map<VkRenderPass, VkVizRenderPass*> render_passes_;

   public:

    // Just here for testing
    VkVizRenderPass() {};

    // static VkVizRenderPass& Get(VkRenderPass render_pass) { return *render_passes_[render_pass];};
    VkVizRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkRenderPass* pRenderPass) {
        for (int i = 0; i < pCreateInfo->attachmentCount; ++i) {
            attachments.push_back(pCreateInfo->pAttachments[i]);
        }
        for (int i = 0; i < pCreateInfo->subpassCount; ++i) {
            subpasses.push_back(pCreateInfo->pSubpasses[i]);
        }
        for (int i = 0; i < pCreateInfo->dependencyCount; ++i) {
            dependencies.push_back(pCreateInfo->pDependencies[i]);
        }

    // render_passes_[*pRenderPass] = this;
    }

    VkSubpassDescription Subpass(size_t index) const {return subpasses[index];}
};

class VkVizFramebuffer {
    VkFramebuffer framebuffer_;
//    static std::unordered_map<VkFramebuffer, VkVizFramebuffer> framebuffer_map_;

 public:
//    static VkVizFramebuffer& Get(VkFramebuffer framebuffer) {return framebuffer_map_[framebuffer];}
      VkVizFramebuffer() : framebuffer_(nullptr) {};
};

class VkVizRenderPassInstance {
    const VkVizRenderPass& render_pass;
    int current_subpass_index = 0;
    const VkVizFramebuffer& framebuffer;

   public:
    VkVizRenderPassInstance(const VkRenderPassBeginInfo* pRenderPassBegin)
        : render_pass(VkVizRenderPass()), framebuffer(VkVizFramebuffer()){};
//        : render_pass(VkVizRenderPass::Get(pRenderPassBegin->renderPass)),
framebuffer(VkVizFramebuffer::Get(pRenderPassBegin->framebuffer)){};

    void NextSubpass(VkSubpassContents) { ++current_subpass_index; }

    void EndRenderPass() { current_subpass_index = -1; }

    VkSubpassDescription CurrentSubpass() { return render_pass.Subpass(current_subpass_index); }
};

*/
class VkVizCommandBuffer {
    VkVizDevice* device_;

    VkPipeline bound_graphics_pipeline_ = nullptr;
    VkPipeline bound_compute_pipeline_ = nullptr;
    std::vector<VkVizDescriptorSet> graphics_descriptor_sets_;
    std::vector<VkVizDescriptorSet> compute_descriptor_sets_;

    VkCommandBufferLevel level_;

    // std::vector<VkVizRenderPassInstance> render_pass_instances_;

    // Looks like spirv can't give which descriptor set binding indices shaders use. Therefore, we only track which sets and
    // bindings are used, and mark every buffer or image in the given set and binding as being used.
    std::vector<VkVizDescriptor> DescriptorsFromUse(const DescriptorUse& use, PipelineType pipeline_type) {
        std::vector<VkVizDescriptor> out;
        if(pipeline_type == GRAPHICS) {
            return graphics_descriptor_sets_[use.set].set_[use.binding];
        } else {
            return compute_descriptor_sets_[use.set].set_[use.binding];
        }
    }

    std::vector<std::pair<VkShaderStageFlagBits, std::vector<MemoryAccess>>> GraphicsPipelineAccesses();

   public:
    VkCommandBuffer handle_;
    std::vector<CommandWrapper> commands_;

    VkVizCommandBuffer() = default;
    VkVizCommandBuffer(VkCommandBuffer handle, std::vector<CommandWrapper> commands)
        : handle_(handle), commands_(std::move(commands)){};
    VkVizCommandBuffer(const VkCommandBuffer commandBuffer, VkCommandBufferLevel level, VkVizDevice* device) : handle_(commandBuffer), level_(level), device_(device) {}

    const std::vector<CommandWrapper>& Commands() const { return commands_; }
    VkCommandBuffer Handle() const { return handle_; }

    // These three functions aren't of the form vkCmd*.
    VkResult Begin();
    VkResult End();
    VkResult Reset();

    void BeginDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo);
    void BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
    void BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents);
    void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
                            uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                            const uint32_t* pDynamicOffsets);
    void BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
    void BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
    void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
    void BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter);
    // More unifinished code for handling VkCmdClearColorImage.
    // std::vector<MemoryAccess> ClearAttachments(VkVizRenderPass render_pass, uint32_t attachment_count,
    //                                           const VkClearAttachment* p_attachments, uint32_t rect_count,
    //                                           const VkClearRect* p_rects);
    void ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount,
                          const VkClearRect* pRects);
    void ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount,
                         const VkImageSubresourceRange* pRanges);
    void ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil,
                                uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
    void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                           const VkBufferImageCopy* pRegions);
    void CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageCopy* pRegions);
    void CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount,
                           const VkBufferImageCopy* pRegions);
    void CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer,
                              VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags);
    void DebugMarkerBeginEXT(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
    void DebugMarkerEndEXT();
    void DebugMarkerInsertEXT(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void DispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY,
                      uint32_t groupCountZ);
    void DispatchBaseKHR(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY,
                         uint32_t groupCountZ);
    void DispatchIndirect(VkBuffer buffer, VkDeviceSize offset);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                     uint32_t firstInstance);
    void DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    void DrawIndexedIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset,
                                     uint32_t maxDrawCount, uint32_t stride);
    void DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
    void DrawIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset,
                              uint32_t maxDrawCount, uint32_t stride);
    void EndDebugUtilsLabelEXT();
    void EndQuery(VkQueryPool queryPool, uint32_t query);
    void EndRenderPass();
    void ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandWrapperBuffers);
    void FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
    void InsertDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo);
    void NextSubpass(VkSubpassContents contents);
    void PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                         uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                         const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                         const VkImageMemoryBarrier* pImageMemoryBarriers);
    void ProcessCommandsNVX(const VkCmdProcessCommandsInfoNVX* pProcessCommandWrappersInfo);
    void PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
    void PushDescriptorSetKHR(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set,
                              uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites);
    void PushDescriptorSetWithTemplateKHR(VkDescriptorUpdateTemplate descriptorUpdateTemplate, VkPipelineLayout layout,
                                          uint32_t set, const void* pData);
    void ReserveSpaceForCommandsNVX(const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo);
    void ResetEvent(VkEvent event, VkPipelineStageFlags stageMask);
    void ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
    void ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                      uint32_t regionCount, const VkImageResolve* pRegions);
    void SetBlendConstants(const float blendConstants[4]);
    void SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
    void SetDepthBounds(float minDepthBounds, float maxDepthBounds);
    void SetDeviceMask(uint32_t deviceMask);
    void SetDeviceMaskKHR(uint32_t deviceMask);
    void SetDiscardRectangleEXT(uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles);
    void SetEvent(VkEvent event, VkPipelineStageFlags stageMask);
    void SetLineWidth(float lineWidth);
    void SetSampleLocationsEXT(const VkSampleLocationsInfoEXT* pSampleLocationsInfo);
    void SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
    void SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
    void SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
    void SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
    void SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
    void SetViewportWScalingNV(uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings);
    void UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
    void WaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask,
                    VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                    uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                    uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers);
    void WriteBufferMarkerAMD(VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker);
    void WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query);
    VkResult QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence);
};
SERIALIZE2(VkVizCommandBuffer, handle_, commands_);
/* CodeGen? */

#endif  // COMMAND_BUFFER_H
