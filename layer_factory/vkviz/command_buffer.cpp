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

#include "command_buffer.h"

std::string kMemoryTypeNames[] = {"image memory", "buffer memory"};
std::string memoryTypeString(MEMORY_TYPE type) {
    return kMemoryTypeNames[type];
}

std::string kReadWriteStrings[] = {"read", "write"};
std::string readWriteString(READ_WRITE rw) {
    return kReadWriteStrings[rw];
}

VkResult VkVizCommandBuffer::Begin() { commands_.emplace_back(BasicCommand(CMD_BEGINCOMMANDBUFFER)); }

VkResult VkVizCommandBuffer::End() { commands_.emplace_back(BasicCommand(CMD_ENDCOMMANDBUFFER)); }

VkResult VkVizCommandBuffer::Reset() {
    commands_.clear();
    in_render_pass_instance_ = false;
}

void VkVizCommandBuffer::BeginDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo) {
    commands_.emplace_back(BasicCommand(CMD_BEGINDEBUGUTILSLABELEXT));
}

void VkVizCommandBuffer::BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    commands_.emplace_back(BasicCommand(CMD_BEGINQUERY));
}

void VkVizCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    commands_.emplace_back(BasicCommand(CMD_BEGINRENDERPASS));
    in_render_pass_instance_ = true;
}

void VkVizCommandBuffer::BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
                                            uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets,
                                            uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) {
    std::vector<VkVizDescriptorSet>* bound_descriptor_sets = &graphics_descriptor_sets_;
    if(pipelineBindPoint == VK_PIPELINE_BIND_POINT_COMPUTE) {
        bound_descriptor_sets = &compute_descriptor_sets_;
    }

    if(firstSet + descriptorSetCount > bound_descriptor_sets->size()) {
        bound_descriptor_sets->resize(firstSet + descriptorSetCount);
    }

    for(uint32_t i=0, set_number = firstSet; i<descriptorSetCount; ++i, ++set_number) {
        (*bound_descriptor_sets)[set_number] = device_->GetVkVizDescriptorSet(pDescriptorSets[i]);
    }

    commands_.emplace_back(BindDescriptorSetsCommand(CMD_BINDDESCRIPTORSETS, *bound_descriptor_sets));
}

void VkVizCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    commands_.emplace_back(IndexBufferBind(CMD_BINDINDEXBUFFER, buffer));
}

void VkVizCommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    if(pipelineBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
        bound_graphics_pipeline_ = pipeline;
    } else {
        bound_compute_pipeline_ = pipeline;
    }
    commands_.emplace_back(BasicCommand(CMD_BINDPIPELINE));
}

void VkVizCommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers,
                                           const VkDeviceSize* pOffsets) {
    std::vector<VkBuffer> vertex_buffers;
    for(int i=0; i<bindingCount; ++i) {
        vertex_buffers.emplace_back(pBuffers[i]);
    }

    commands_.emplace_back(VertexBufferBind(CMD_BINDVERTEXBUFFERS, vertex_buffers));
}

void VkVizCommandBuffer::BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) {
    commands_.emplace_back(Access(CMD_BLITIMAGE, {ImageRead(srcImage, regionCount, pRegions), ImageWrite(dstImage, regionCount, pRegions)}));
}

/* Unfinished code for handling VkCmdClearColorImage
std::vector<MemoryAccess> ClearAttachments(VkVizSubPass sub_pass, uint32_t attachment_count, const VkClearAttachment* p_attachments,
                                           uint32_t rect_count, const VkClearRect* p_rects) {
    std::vector<MemoryAccess> accesses;

    for (uint32_t i = 0; i < attachment_count; ++i) {
        const VkClearAttachment& clear = p_attachments[i];

        if (clear.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT && clear.colorAttachment != VK_ATTACHMENT_UNUSED) {
            VkVizImageView& cleared_view = sub_pass.ColorAttachments()[clear.colorAttachment].ImageView();
            accesses.emplace_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }

        if (clear.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT && sub_pass.HasDepthAttachment()) {
            VkVizImageView& cleared_view = sub_pass.DepthAttachment().ImageView();
            accesses.emplace_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }

        if (clear.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT && sub_pass.HasStencilAttachment()) {
            VkVizImageView& cleared_view = sub_pass.StencilAttachment().ImageView();
            accesses.emplace_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }
    }

    return accesses;
}*/

void VkVizCommandBuffer::ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount,
                                          const VkClearRect* pRects) {
    // commands_.emplace_back(BasicCommand(CMD_CLEARATTACHMENTS, AttachmentsClear(CurrentSubpass(), attachmentCount, pAttachments,
    // rectCount, pRects)));
    commands_.emplace_back(BasicCommand(CMD_CLEARATTACHMENTS));
}

void VkVizCommandBuffer::ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor,
                                         uint32_t rangeCount, const VkImageSubresourceRange* pRanges) {
    commands_.emplace_back(Access(CMD_CLEARCOLORIMAGE, ImageWrite(image, rangeCount, pRanges)));
}

void VkVizCommandBuffer::ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout,
                                                const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
                                                const VkImageSubresourceRange* pRanges) {
    commands_.emplace_back(Access(CMD_CLEARDEPTHSTENCILIMAGE, ImageWrite(image, rangeCount, pRanges)));
}

void VkVizCommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) {
    commands_.emplace_back(Access(CMD_COPYBUFFER, {BufferRead(srcBuffer, regionCount, pRegions), BufferWrite(dstBuffer, regionCount, pRegions)}));
}

void VkVizCommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                           const VkBufferImageCopy* pRegions) {
    commands_.emplace_back(
        Access(CMD_COPYBUFFERTOIMAGE, {BufferRead(srcBuffer, regionCount, pRegions), ImageWrite(dstImage, regionCount, pRegions)}));
}

void VkVizCommandBuffer::CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                                   uint32_t regionCount, const VkImageCopy* pRegions) {
    commands_.emplace_back(
        Access(CMD_COPYIMAGE, {ImageRead(srcImage, regionCount, pRegions), ImageWrite(dstImage, regionCount, pRegions)}));
}

void VkVizCommandBuffer::CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount,
                                           const VkBufferImageCopy* pRegions) {
    commands_.emplace_back(
        Access(CMD_COPYIMAGETOBUFFER, {ImageRead(srcImage, regionCount, pRegions), BufferWrite(dstBuffer, regionCount, pRegions)}));
}

void VkVizCommandBuffer::CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer,
                                              VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    commands_.emplace_back(BasicCommand(CMD_COPYQUERYPOOLRESULTS));

    // Read queryPool range

    // We don't really know the write size unless we keep track of
    // GetQueryPoolResults I think
    // Write dstBuffer range
}

void VkVizCommandBuffer::DebugMarkerBeginEXT(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    commands_.emplace_back(BasicCommand(CMD_DEBUGMARKERBEGINEXT));
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}

void VkVizCommandBuffer::DebugMarkerEndEXT() {
    commands_.emplace_back(BasicCommand(CMD_DEBUGMARKERENDEXT));
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}

void VkVizCommandBuffer::DebugMarkerInsertEXT(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    commands_.emplace_back(BasicCommand(CMD_DEBUGMARKERINSERTEXT));
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}

void VkVizCommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCH));
}

void VkVizCommandBuffer::DispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX,
                                      uint32_t groupCountY, uint32_t groupCountZ) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCHBASE));
}

void VkVizCommandBuffer::DispatchBaseKHR(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX,
                                         uint32_t groupCountY, uint32_t groupCountZ) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCHBASEKHR));
}

void VkVizCommandBuffer::DispatchIndirect(VkBuffer buffer, VkDeviceSize offset) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCHINDIRECT));
}

std::vector<std::pair<VkShaderStageFlagBits, std::vector<MemoryAccess>>> VkVizCommandBuffer::GraphicsPipelineAccesses() {
    std::vector<std::pair<VkShaderStageFlagBits, std::vector<MemoryAccess>>> pipeline_accesses;

    const std::vector<PipelineStage> pipeline = device_->GetPipelineStages(bound_graphics_pipeline_);
    for(const auto& stage : pipeline) {
        std::vector<MemoryAccess> stage_accesses;

        for(const DescriptorUse& descriptor_use : stage.descriptor_uses) {
            const std::vector<VkVizDescriptor> descriptors = DescriptorsFromUse(descriptor_use, GRAPHICS);

            READ_WRITE read_or_write;
            if (descriptor_use.is_readonly) {
                read_or_write = READ;
            } else {
                read_or_write = WRITE;
            }

            for(const auto& descriptor : descriptors) {
                if(descriptor.descriptor_type == IMAGE_DESCRIPTOR) {
                    VkImageView image_view = descriptor.ImageView();
                    VkImage image = device_->ImageFromView(image_view);
                    VkImageBlit dummy_blit;
                    stage_accesses.emplace_back(MemoryAccess::Image(read_or_write, image, 0, &dummy_blit));
                } else if(descriptor.descriptor_type == BUFFER_DESCRIPTOR) {
                    VkBuffer buffer = descriptor.Buffer();
                    stage_accesses.emplace_back(MemoryAccess::Buffer(read_or_write, buffer, 0, 0));
                } else {
                    VkBufferView buffer_view = descriptor.BufferView();
                    VkBuffer buffer = device_->BufferFromView(buffer_view);
                    stage_accesses.emplace_back(MemoryAccess::Buffer(read_or_write, buffer, 0, 0));
                }
            }
        }

        pipeline_accesses.emplace_back(std::make_pair(stage.stage_flag, std::move(stage_accesses)));
    }

    return pipeline_accesses;
}

void VkVizCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    // Reads vertex buffers according to PipelineVertexInputState and bind commands
    // Runs graphics pipeline shader stages

    commands_.emplace_back(DrawCommand(CMD_DRAW, GraphicsPipelineAccesses()));
}

void VkVizCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                     uint32_t firstInstance) {
    // Read index buffer (bound by CmdBindIndexBuffer)
    // Reads vertex buffers according to PipelineVertexInputState and bind commands
    // Runs graphics pipeline shader stages

    commands_.emplace_back(DrawCommand(CMD_DRAWINDEXED, GraphicsPipelineAccesses()));
}

void VkVizCommandBuffer::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDEXEDINDIRECT));
}

void VkVizCommandBuffer::DrawIndexedIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                                     VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDEXEDINDIRECTCOUNTAMD));
}

void VkVizCommandBuffer::DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDIRECT));
}

void VkVizCommandBuffer::DrawIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                              VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDIRECTCOUNTAMD));
}

void VkVizCommandBuffer::EndDebugUtilsLabelEXT() {
    commands_.emplace_back(BasicCommand(CMD_ENDDEBUGUTILSLABELEXT));
    // TODO(whenning): unsure
}

void VkVizCommandBuffer::EndQuery(VkQueryPool queryPool, uint32_t query) {
    commands_.emplace_back(BasicCommand(CMD_ENDQUERY));
    // TODO(whenning): unsure
}

void VkVizCommandBuffer::EndRenderPass() {
    commands_.emplace_back(BasicCommand(CMD_ENDRENDERPASS));
    in_render_pass_instance_ = false;
}

void VkVizCommandBuffer::ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    commands_.emplace_back(BasicCommand(CMD_EXECUTECOMMANDS));

    // Secondary command buffers
}

void VkVizCommandBuffer::FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) {
    commands_.emplace_back(Access(CMD_FILLBUFFER, BufferWrite(dstBuffer, dstOffset, size)));
}

void VkVizCommandBuffer::InsertDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo) {
    commands_.emplace_back(BasicCommand(CMD_INSERTDEBUGUTILSLABELEXT));
}

void VkVizCommandBuffer::NextSubpass(VkSubpassContents contents) { commands_.emplace_back(BasicCommand(CMD_NEXTSUBPASS)); }

void VkVizCommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                         VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
                                         const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                         const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                         const VkImageMemoryBarrier* pImageMemoryBarriers) {
    if (in_render_pass_instance_) {
        // We aren't trying to handle synchronization inside of render passes yet.
        commands_.emplace_back(PipelineBarrierCommand(CMD_PIPELINEBARRIER, VkVizPipelineBarrier()));
        return;
    }

    // Add a pipeline barrier

    // If outside render pass, first sync scope is all earlier submitted commands and second scope is all later commands
    // If inside a render pass, the same applies but the scope is limited to the current subpass
    // In both cases, the sync scopes are limited to the given stage masks in the pipeline

    // If inside a render pass, there can't be buffer memory barriers, and all images in image memory barriers must be attachments
    // that the framebuffer was created with, and the image must be an attachment in the VkSubpassDescription of the current
    // subpass.

    // Add global, buffer, and image memory barriers
    // Queue family and image format transitions can be ignored?

    std::vector<MemoryBarrier> global_barriers;
    for (int i = 0; i < memoryBarrierCount; ++i) {
        global_barriers.emplace_back(pMemoryBarriers[i]);
    }

    std::vector<BufferBarrier> buffer_barriers;
    for (int i = 0; i < bufferMemoryBarrierCount; ++i) {
        buffer_barriers.emplace_back(pBufferMemoryBarriers[i]);
    }

    std::vector<ImageBarrier> image_barriers;
    for (int i = 0; i < imageMemoryBarrierCount; ++i) {
        image_barriers.emplace_back(pImageMemoryBarriers[i]);
    }

    commands_.emplace_back(PipelineBarrierCommand(
        CMD_PIPELINEBARRIER, VkVizPipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, std::move(global_barriers),
                                                  std::move(buffer_barriers), std::move(image_barriers))));
}

void VkVizCommandBuffer::ProcessCommandsNVX(const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) {
    commands_.emplace_back(BasicCommand(CMD_PROCESSCOMMANDSNVX));
}

void VkVizCommandBuffer::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                                       const void* pValues) {
    commands_.emplace_back(BasicCommand(CMD_PUSHCONSTANTS));
}

void VkVizCommandBuffer::PushDescriptorSetKHR(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set,
                                              uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) {
    commands_.emplace_back(BasicCommand(CMD_PUSHDESCRIPTORSETKHR));
}

void VkVizCommandBuffer::PushDescriptorSetWithTemplateKHR(VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                          VkPipelineLayout layout, uint32_t set, const void* pData) {
    commands_.emplace_back(BasicCommand(CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR));
}

void VkVizCommandBuffer::ReserveSpaceForCommandsNVX(const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) {
    commands_.emplace_back(BasicCommand(CMD_RESERVESPACEFORCOMMANDSNVX));
}

void VkVizCommandBuffer::ResetEvent(VkEvent event, VkPipelineStageFlags stageMask) {
    commands_.emplace_back(BasicCommand(CMD_RESETEVENT));
}

void VkVizCommandBuffer::ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    commands_.emplace_back(BasicCommand(CMD_RESETQUERYPOOL));
}

void VkVizCommandBuffer::ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                      VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) {
    commands_.emplace_back(BasicCommand(CMD_RESOLVEIMAGE));
    // Read srcImage regions
    // Write dstImage regions
}

void VkVizCommandBuffer::SetBlendConstants(const float blendConstants[4]) {
    commands_.emplace_back(BasicCommand(CMD_SETBLENDCONSTANTS));
}

void VkVizCommandBuffer::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) {
    commands_.emplace_back(BasicCommand(CMD_SETDEPTHBIAS));
}

void VkVizCommandBuffer::SetDepthBounds(float minDepthBounds, float maxDepthBounds) {
    commands_.emplace_back(BasicCommand(CMD_SETDEPTHBOUNDS));
}

void VkVizCommandBuffer::SetDeviceMask(uint32_t deviceMask) {
    commands_.emplace_back(BasicCommand(CMD_SETDEVICEMASK));
}

void VkVizCommandBuffer::SetDeviceMaskKHR(uint32_t deviceMask) {
    commands_.emplace_back(BasicCommand(CMD_SETDEVICEMASKKHR));
}

void VkVizCommandBuffer::SetDiscardRectangleEXT(uint32_t firstDiscardRectangle, uint32_t discardRectangleCount,
                                                const VkRect2D* pDiscardRectangles) {
    commands_.emplace_back(BasicCommand(CMD_SETDISCARDRECTANGLEEXT));
}

void VkVizCommandBuffer::SetEvent(VkEvent event, VkPipelineStageFlags stageMask) {
    commands_.emplace_back(BasicCommand(CMD_SETEVENT));
}

void VkVizCommandBuffer::SetLineWidth(float lineWidth) {
    commands_.emplace_back(BasicCommand(CMD_SETLINEWIDTH));
}

void VkVizCommandBuffer::SetSampleLocationsEXT(const VkSampleLocationsInfoEXT* pSampleLocationsInfo) {
    commands_.emplace_back(BasicCommand(CMD_SETSAMPLELOCATIONSEXT));
}

void VkVizCommandBuffer::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) {
    commands_.emplace_back(BasicCommand(CMD_SETSCISSOR));
}

void VkVizCommandBuffer::SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask) {
    commands_.emplace_back(BasicCommand(CMD_SETSTENCILCOMPAREMASK));
}

void VkVizCommandBuffer::SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference) {
    commands_.emplace_back(BasicCommand(CMD_SETSTENCILREFERENCE));
}

void VkVizCommandBuffer::SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask) {
    commands_.emplace_back(BasicCommand(CMD_SETSTENCILWRITEMASK));
}

void VkVizCommandBuffer::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) {
    commands_.emplace_back(BasicCommand(CMD_SETVIEWPORT));
}

void VkVizCommandBuffer::SetViewportWScalingNV(uint32_t firstViewport, uint32_t viewportCount,
                                               const VkViewportWScalingNV* pViewportWScalings) {
    commands_.emplace_back(BasicCommand(CMD_SETVIEWPORTWSCALINGNV));
}

void VkVizCommandBuffer::UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) {
    commands_.emplace_back(BasicCommand(CMD_UPDATEBUFFER));
    // Write dstBuffer range
    // Treated as a "transfer" operation for purpose of synchronization
}

void VkVizCommandBuffer::WaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount,
                                    const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                    const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                    const VkImageMemoryBarrier* pImageMemoryBarriers) {
    commands_.emplace_back(BasicCommand(CMD_WAITEVENTS));
    // Synchronization
}

void VkVizCommandBuffer::WriteBufferMarkerAMD(VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                              uint32_t marker) {
    commands_.emplace_back(BasicCommand(CMD_WRITEBUFFERMARKERAMD));
    // Write to dstBuffer after pipelineStage
    // Maybe not implement?
}

void VkVizCommandBuffer::WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) {
    commands_.emplace_back(BasicCommand(CMD_WRITETIMESTAMP));
    // Write to queryPool after pipelineStage
    // maybe not implement?
}
