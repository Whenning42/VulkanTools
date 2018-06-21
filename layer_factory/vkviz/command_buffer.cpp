#include "command_buffer.h"

std::string kMemoryTypeNames[] = {"image memory", "buffer memory"};
std::string memoryTypeString(MEMORY_TYPE type) {
    return kMemoryTypeNames[type];
}

std::string kReadWriteStrings[] = {"read", "write"};
std::string readWriteString(READ_WRITE rw) {
    return kReadWriteStrings[rw];
}


VkResult VkVizCommandBuffer::Begin() { AddCommand(CMD_BEGINCOMMANDBUFFER); }

VkResult VkVizCommandBuffer::End() { AddCommand(CMD_ENDCOMMANDBUFFER); }

VkResult VkVizCommandBuffer::Reset() {
    commands_.clear();
    current_render_pass_ = -1;
    render_pass_instances_.clear();
}

void VkVizCommandBuffer::BeginDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo) {
    AddCommand(CMD_BEGINDEBUGUTILSLABELEXT);
    // TODO
}

void VkVizCommandBuffer::BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    AddCommand(CMD_BEGINQUERY);
}

void VkVizCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    AddCommand(CMD_BEGINRENDERPASS);

    current_render_pass_ = render_pass_instances_.size() - 1;
    render_pass_instances_.push_back(VkVizRenderPassInstance(pRenderPassBegin));
}

void VkVizCommandBuffer::BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
                                            uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets,
                                            uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) {
    AddCommand(CMD_BINDDESCRIPTORSETS);

    // TODO: Looks hard
}

void VkVizCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    //commands_.emplace_back(std::unique_ptr<Command>(new IndexBufferBind(CMD_BINDINDEXBUFFER, buffer)));
    commands_.emplace_back(std::make_unique<IndexBufferBind>(CMD_BINDINDEXBUFFER, buffer));
}

void VkVizCommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) { AddCommand(CMD_BINDPIPELINE); }

void VkVizCommandBuffer::BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers,
                                           const VkDeviceSize* pOffsets) {
    std::vector<VkBuffer> vertex_buffers;
    for(int i=0; i<bindingCount; ++i) {
        vertex_buffers.push_back(pBuffers[i]);
    }

    commands_.emplace_back(std::make_unique<VertexBufferBind>(CMD_BINDVERTEXBUFFERS, vertex_buffers));
}

void VkVizCommandBuffer::BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) {
    AddCommand(CMD_BLITIMAGE, {ImageRead(srcImage, regionCount, pRegions),
                               ImageWrite(dstImage, regionCount, pRegions)});
}

std::vector<MemoryAccess> ClearAttachments(VkVizSubPass sub_pass, uint32_t attachment_count, const VkClearAttachment* p_attachments,
                                           uint32_t rect_count, const VkClearRect* p_rects) {
    std::vector<MemoryAccess> accesses;

    /*
    for (uint32_t i = 0; i < attachment_count; ++i) {
        const VkClearAttachment& clear = p_attachments[i];

        if (clear.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT && clear.colorAttachment != VK_ATTACHMENT_UNUSED) {
            VkVizImageView& cleared_view = sub_pass.ColorAttachments()[clear.colorAttachment].ImageView();
            accesses.push_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }

        if (clear.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT && sub_pass.HasDepthAttachment()) {
            VkVizImageView& cleared_view = sub_pass.DepthAttachment().ImageView();
            accesses.push_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }

        if (clear.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT && sub_pass.HasStencilAttachment()) {
            VkVizImageView& cleared_view = sub_pass.StencilAttachment().ImageView();
            accesses.push_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }
    }*/

    return accesses;
}

void VkVizCommandBuffer::ClearAttachments(uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount,
                                          const VkClearRect* pRects) {
    //AddCommand(CMD_CLEARATTACHMENTS, AttachmentsClear(CurrentSubpass(), attachmentCount, pAttachments, rectCount, pRects));
    AddCommand(CMD_CLEARATTACHMENTS);
}

void VkVizCommandBuffer::ClearColorImage(VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor,
                                         uint32_t rangeCount, const VkImageSubresourceRange* pRanges) {
    AddCommand(CMD_CLEARCOLORIMAGE, ImageWrite(image, rangeCount, pRanges));
}

void VkVizCommandBuffer::ClearDepthStencilImage(VkImage image, VkImageLayout imageLayout,
                                                const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
                                                const VkImageSubresourceRange* pRanges) {
    AddCommand(CMD_CLEARDEPTHSTENCILIMAGE, ImageWrite(image, rangeCount, pRanges));
}

void VkVizCommandBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) {
    AddCommand(CMD_COPYBUFFER, {BufferRead(srcBuffer, regionCount, pRegions), BufferWrite(dstBuffer, regionCount, pRegions)});
}

void VkVizCommandBuffer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                           const VkBufferImageCopy* pRegions) {
    AddCommand(CMD_COPYBUFFERTOIMAGE, {BufferRead(srcBuffer, regionCount, pRegions), ImageWrite(dstImage, regionCount, pRegions)});
}

void VkVizCommandBuffer::CopyImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout,
                                   uint32_t regionCount, const VkImageCopy* pRegions) {
    AddCommand(CMD_COPYIMAGE, {ImageRead(srcImage, regionCount, pRegions), ImageWrite(dstImage, regionCount, pRegions)});
}

void VkVizCommandBuffer::CopyImageToBuffer(VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount,
                                           const VkBufferImageCopy* pRegions) {
    AddCommand(CMD_COPYIMAGETOBUFFER, {ImageRead(srcImage, regionCount, pRegions), BufferWrite(dstBuffer, regionCount, pRegions)});
}

void VkVizCommandBuffer::CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer,
                                              VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) {
    AddCommand(CMD_COPYQUERYPOOLRESULTS);
    // Implement later?
    // Read queryPool range

    // We don't really know the write size unless we keep track of
    // GetQueryPoolResults I think
    // Write dstBuffer range
}

void VkVizCommandBuffer::DebugMarkerBeginEXT(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    AddCommand(CMD_DEBUGMARKERBEGINEXT);
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}

void VkVizCommandBuffer::DebugMarkerEndEXT() {
    AddCommand(CMD_DEBUGMARKERENDEXT);
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}

void VkVizCommandBuffer::DebugMarkerInsertEXT(const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    AddCommand(CMD_DEBUGMARKERINSERTEXT);
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}

void VkVizCommandBuffer::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    AddCommand(CMD_DISPATCH);
    // Compute pipeline synchronization
}

void VkVizCommandBuffer::DispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX,
                                      uint32_t groupCountY, uint32_t groupCountZ) {
    AddCommand(CMD_DISPATCHBASE);
    // Compute pipeline synchronization
}

void VkVizCommandBuffer::DispatchBaseKHR(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX,
                                         uint32_t groupCountY, uint32_t groupCountZ) {
    AddCommand(CMD_DISPATCHBASEKHR);
    // Compute pipeline synchronization
}

void VkVizCommandBuffer::DispatchIndirect(VkBuffer buffer, VkDeviceSize offset) {
    AddCommand(CMD_DISPATCHINDIRECT);
    // Compute pipeline synchronization
    // Read buffer
}

std::vector<MemoryAccess> DrawAccesses() {
    std::vector<MemoryAccess> accesses;

    /*
    // Add shader output writes presumably to entire render target
    // Add descriptor reads and writes for all stages and regions possible, possibly anotate as possible reads/writes
    std::vector<MemoryAccess> output_writes = OutputAttachmentWrites(pipeline?);
    std::vector<MemoryAccess> descriptor_accesses = PossibleDescriptorAccesses(pipeline?);
    accesses.insert(accesses.end(), output_writes.begin(), output_writes.end());
    accesses.insert(accesses.end(), descritpro_accesses.begin(), descriptor_accesses.end());
    */
    return accesses;
}

std::vector<MemoryAccess> DrawAccess(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    std::vector<MemoryAccess> accesses;

    /*
    MemoryAccess buffer_read = VertexBufferRead(pipeline?);
    return buffer_read + DrawAccesses();
    */
    return accesses;
}

std::vector<MemoryAccess> DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                      uint32_t firstInstance) {
    std::vector<MemoryAccess> accesses;

    /*
    MemoryAccess vertex_buffer_read = VertexBufferRead(pipeline?);
    MemoryAccess index_buffer_read = IndexBufferRead(pipleine?);
    return buffer_read + DrawAccesses();
    */
    return accesses;
}

std::vector<MemoryAccess> DrawIndexedIndirect() {
    std::vector<MemoryAccess> accesses;

    /*
    acccesses;

    MemAccess vertex_buffer = vbread(pipeline);
    MemAccess index_buffer = ibread(pipeline);
    return vertex_buffer + index_buffer + DrawAccess();
    */
    return accesses;
}

void VkVizCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    AddCommand(CMD_DRAW, DrawAccess(vertexCount, instanceCount, firstVertex, firstInstance));

    // Graphics pipeline synchronization
}

void VkVizCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                     uint32_t firstInstance) {
    AddCommand(CMD_DRAWINDEXED);
    // Graphics pipeline synchronization
    // Read index buffer (bound by CmdBindIndexBuffer, not an arg)
}

void VkVizCommandBuffer::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    AddCommand(CMD_DRAWINDEXEDINDIRECT);
    // Graphics pipeline synchronization
    // Read buffer
}

void VkVizCommandBuffer::DrawIndexedIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                                     VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    AddCommand(CMD_DRAWINDEXEDINDIRECTCOUNTAMD);
    // Graphics pipeline synchronization
    // Read buffer
}

void VkVizCommandBuffer::DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    AddCommand(CMD_DRAWINDIRECT);
    // Graphics pipeline synchronization
    // Read buffer
}

void VkVizCommandBuffer::DrawIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                              VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    AddCommand(CMD_DRAWINDIRECTCOUNTAMD);
    // Graphics pipeline synchronization
    // Read buffer
    // Read countBuffer
}

void VkVizCommandBuffer::EndDebugUtilsLabelEXT() {
    AddCommand(CMD_ENDDEBUGUTILSLABELEXT);
    // TODO(whenning): unsure
}

void VkVizCommandBuffer::EndQuery(VkQueryPool queryPool, uint32_t query) {
    AddCommand(CMD_ENDQUERY);
    // TODO(whenning): unsure
}

void VkVizCommandBuffer::EndRenderPass() {
    AddCommand(CMD_ENDRENDERPASS);

    // Mark that we are no longer in a renderpass
    current_render_pass_ = -1;

    // Generate intermediate access info?
}

void VkVizCommandBuffer::ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) {
    AddCommand(CMD_EXECUTECOMMANDS);

    // Secondary command buffers
}

void VkVizCommandBuffer::FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) {
    AddCommand(CMD_FILLBUFFER, BufferWrite(dstBuffer, dstOffset, size));
}

void VkVizCommandBuffer::InsertDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo) {
    AddCommand(CMD_INSERTDEBUGUTILSLABELEXT);
}

void VkVizCommandBuffer::NextSubpass(VkSubpassContents contents) { AddCommand(CMD_NEXTSUBPASS); }

void VkVizCommandBuffer::PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                         VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
                                         const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                         const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                         const VkImageMemoryBarrier* pImageMemoryBarriers) {
    // Add a pipeline barrier

    // If outside render pass instance, first sync scope is all earlier submitted commands and second scope is all later commands
    // If inside a render pass instance, the same applies but the scope is limited to the current subpass
    // In both cases, the sync scopes are limited to the given stage masks in the pipeline

    // Add global, buffer, and image memory barriers
    // Queue family and image format transitions can be ignored?

    std::vector<std::unique_ptr<MemoryBarrier>> memory_barriers;
    for (int i = 0; i < memoryBarrierCount; ++i) {
        const VkMemoryBarrier& barrier = pMemoryBarriers[i];
        memory_barriers.emplace_back(std::make_unique<GlobalMemoryBarrier>(barrier.srcAccessMask, barrier.dstAccessMask));
    }

    for (int i = 0; i < bufferMemoryBarrierCount; ++i) {
        const VkBufferMemoryBarrier& barrier = pBufferMemoryBarriers[i];
        memory_barriers.emplace_back(
            std::make_unique<BufferMemoryBarrier>(barrier.srcAccessMask, barrier.dstAccessMask, barrier.buffer, barrier.offset, barrier.size));
    }

    for (int i = 0; i < imageMemoryBarrierCount; ++i) {
        const VkImageMemoryBarrier& barrier = pImageMemoryBarriers[i];
        memory_barriers.emplace_back(
            std::make_unique<ImageMemoryBarrier>(barrier.srcAccessMask, barrier.dstAccessMask, barrier.image, barrier.subresourceRange));
    }

    AddCommand(CMD_PIPELINEBARRIER, VkVizPipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, std::move(memory_barriers)));
}

void VkVizCommandBuffer::ProcessCommandsNVX(const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) {
    AddCommand(CMD_PROCESSCOMMANDSNVX);
    // NVXCommands maybe not?
}

void VkVizCommandBuffer::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                                       const void* pValues) {
    AddCommand(CMD_PUSHCONSTANTS);
    // Write push constants?
}

void VkVizCommandBuffer::PushDescriptorSetKHR(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set,
                                              uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) {
    AddCommand(CMD_PUSHDESCRIPTORSETKHR);
    // Write descriptor sets?
}

void VkVizCommandBuffer::PushDescriptorSetWithTemplateKHR(VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                          VkPipelineLayout layout, uint32_t set, const void* pData) {
    AddCommand(CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR);
    // Write descriptor sets?
}

void VkVizCommandBuffer::ReserveSpaceForCommandsNVX(const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) {
    AddCommand(CMD_RESERVESPACEFORCOMMANDSNVX);
    // NVXCommands maybe not?
}

void VkVizCommandBuffer::ResetEvent(VkEvent event, VkPipelineStageFlags stageMask) {
    AddCommand(CMD_RESETEVENT);
    // Write event at stage
}

void VkVizCommandBuffer::ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    AddCommand(CMD_RESETQUERYPOOL);
    // Write query pool range
}

void VkVizCommandBuffer::ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                      VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) {
    AddCommand(CMD_RESOLVEIMAGE);
    // Read srcImage regions
    // Write dstImage regions
}

void VkVizCommandBuffer::SetBlendConstants(const float blendConstants[4]) {
    AddCommand(CMD_SETBLENDCONSTANTS);
    // Set blend constants
}

void VkVizCommandBuffer::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) {
    AddCommand(CMD_SETDEPTHBIAS);
    // Set depth bias
}

void VkVizCommandBuffer::SetDepthBounds(float minDepthBounds, float maxDepthBounds) {
    AddCommand(CMD_SETDEPTHBOUNDS);
    // Set depth bounds
}

void VkVizCommandBuffer::SetDeviceMask(uint32_t deviceMask) {
    AddCommand(CMD_SETDEVICEMASK);
    // Set device mask
}

void VkVizCommandBuffer::SetDeviceMaskKHR(uint32_t deviceMask) {
    AddCommand(CMD_SETDEVICEMASKKHR);
    // Set device mask
}

void VkVizCommandBuffer::SetDiscardRectangleEXT(uint32_t firstDiscardRectangle, uint32_t discardRectangleCount,
                                                const VkRect2D* pDiscardRectangles) {
    AddCommand(CMD_SETDISCARDRECTANGLEEXT);
    // Set discard rectangle
}

void VkVizCommandBuffer::SetEvent(VkEvent event, VkPipelineStageFlags stageMask) {
    AddCommand(CMD_SETEVENT);
    // Set event at stage mask
}

void VkVizCommandBuffer::SetLineWidth(float lineWidth) {
    AddCommand(CMD_SETLINEWIDTH);
    // Set line width
}

void VkVizCommandBuffer::SetSampleLocationsEXT(const VkSampleLocationsInfoEXT* pSampleLocationsInfo) {
    AddCommand(CMD_SETSAMPLELOCATIONSEXT);
    // Set sample locations
}

void VkVizCommandBuffer::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) {
    AddCommand(CMD_SETSCISSOR);
    // Set scissor
}

void VkVizCommandBuffer::SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask) {
    AddCommand(CMD_SETSTENCILCOMPAREMASK);
    // Set stencil compare
}

void VkVizCommandBuffer::SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference) {
    AddCommand(CMD_SETSTENCILREFERENCE);
    // Set stencil reference
}

void VkVizCommandBuffer::SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask) {
    AddCommand(CMD_SETSTENCILWRITEMASK);
    // Set stencil write mask
}

void VkVizCommandBuffer::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) {
    AddCommand(CMD_SETVIEWPORT);
    // Set viewport
}

void VkVizCommandBuffer::SetViewportWScalingNV(uint32_t firstViewport, uint32_t viewportCount,
                                               const VkViewportWScalingNV* pViewportWScalings) {
    AddCommand(CMD_SETVIEWPORTWSCALINGNV);
    // Set viewport
}

void VkVizCommandBuffer::UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) {
    AddCommand(CMD_UPDATEBUFFER);
    // Write dstBuffer range
    // Treated as a "transfer" operation for purpose of synchronization
}

void VkVizCommandBuffer::WaitEvents(uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask,
                                    VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount,
                                    const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                    const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                    const VkImageMemoryBarrier* pImageMemoryBarriers) {
    AddCommand(CMD_WAITEVENTS);
    // Synchronization
}

void VkVizCommandBuffer::WriteBufferMarkerAMD(VkPipelineStageFlagBits pipelineStage, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                              uint32_t marker) {
    AddCommand(CMD_WRITEBUFFERMARKERAMD);
    // Write to dstBuffer after pipelineStage
    // Maybe not implement?
}

void VkVizCommandBuffer::WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) {
    AddCommand(CMD_WRITETIMESTAMP);
    // Write to queryPool after pipelineStage
    // maybe not implement?
}
