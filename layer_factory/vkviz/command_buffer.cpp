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
    current_render_pass_ = -1;
    render_pass_instances_.clear();
}

void VkVizCommandBuffer::BeginDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo) {
    commands_.emplace_back(BasicCommand(CMD_BEGINDEBUGUTILSLABELEXT));
}

void VkVizCommandBuffer::BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    commands_.emplace_back(BasicCommand(CMD_BEGINQUERY));
}

void VkVizCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
    commands_.emplace_back(BasicCommand(CMD_BEGINRENDERPASS));

    current_render_pass_ = render_pass_instances_.size() - 1;
    render_pass_instances_.emplace_back(VkVizRenderPassInstance(pRenderPassBegin));
}

std::vector<VkVizDescriptorSet> graphics_descriptor_sets_;
std::vector<VkVizDescriptorSet> compute_descriptor_sets_;

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
        bound_descriptor_sets[set_number] = device_.GetVkVizDescriptorSet(pDescriptorSets[i]);
    }

    commands_.emplace_back(BasicCommand(CMD_BINDDESCRIPTORSETS));
}

void VkVizCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    commands_.emplace_back(IndexBufferBind(CMD_BINDINDEXBUFFER, buffer));
}

void VkVizCommandBuffer::BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
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

std::vector<MemoryAccess> ClearAttachments(VkVizSubPass sub_pass, uint32_t attachment_count, const VkClearAttachment* p_attachments,
                                           uint32_t rect_count, const VkClearRect* p_rects) {
    std::vector<MemoryAccess> accesses;

    /*
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
    }*/

    return accesses;
}

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
    // Implement later?
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
    // Compute pipeline synchronization
}

void VkVizCommandBuffer::DispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX,
                                      uint32_t groupCountY, uint32_t groupCountZ) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCHBASE));
    // Compute pipeline synchronization
}

void VkVizCommandBuffer::DispatchBaseKHR(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX,
                                         uint32_t groupCountY, uint32_t groupCountZ) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCHBASEKHR));
    // Compute pipeline synchronization
}

void VkVizCommandBuffer::DispatchIndirect(VkBuffer buffer, VkDeviceSize offset) {
    commands_.emplace_back(BasicCommand(CMD_DISPATCHINDIRECT));
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
    commands_.emplace_back(Access(CMD_DRAW, DrawAccess(vertexCount, instanceCount, firstVertex, firstInstance)));

    // Graphics pipeline synchronization
}

void VkVizCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                     uint32_t firstInstance) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDEXED));
    // Graphics pipeline synchronization
    // Read index buffer (bound by CmdBindIndexBuffer, not an arg)
}

void VkVizCommandBuffer::DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDEXEDINDIRECT));
    // Graphics pipeline synchronization
    // Read buffer
}

void VkVizCommandBuffer::DrawIndexedIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                                     VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDEXEDINDIRECTCOUNTAMD));
    // Graphics pipeline synchronization
    // Read buffer
}

void VkVizCommandBuffer::DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDIRECT));
    // Graphics pipeline synchronization
    // Read buffer
}

void VkVizCommandBuffer::DrawIndirectCountAMD(VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                                              VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) {
    commands_.emplace_back(BasicCommand(CMD_DRAWINDIRECTCOUNTAMD));
    // Graphics pipeline synchronization
    // Read buffer
    // Read countBuffer
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

    // Mark that we are no longer in a renderpass
    current_render_pass_ = -1;

    // Generate intermediate access info?
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
    // Add a pipeline barrier

    // If outside render pass instance, first sync scope is all earlier submitted commands and second scope is all later commands
    // If inside a render pass instance, the same applies but the scope is limited to the current subpass
    // In both cases, the sync scopes are limited to the given stage masks in the pipeline

    // Add global, buffer, and image memory barriers
    // Queue family and image format transitions can be ignored?

    std::vector<MemoryBarrier> memory_barriers;
    for (int i = 0; i < memoryBarrierCount; ++i) {
        const VkMemoryBarrier& barrier = pMemoryBarriers[i];
        memory_barriers.emplace_back(MemoryBarrier::Global(barrier.srcAccessMask, barrier.dstAccessMask));
    }

    for (int i = 0; i < bufferMemoryBarrierCount; ++i) {
        const VkBufferMemoryBarrier& barrier = pBufferMemoryBarriers[i];
        memory_barriers.emplace_back(
            MemoryBarrier::Buffer(barrier.srcAccessMask, barrier.dstAccessMask, barrier.buffer, barrier.offset, barrier.size));
    }

    for (int i = 0; i < imageMemoryBarrierCount; ++i) {
        const VkImageMemoryBarrier& barrier = pImageMemoryBarriers[i];
        memory_barriers.emplace_back(
            MemoryBarrier::Image(barrier.srcAccessMask, barrier.dstAccessMask, barrier.image, barrier.subresourceRange));
    }

    commands_.emplace_back(PipelineBarrierCommand(
        CMD_PIPELINEBARRIER, VkVizPipelineBarrier(srcStageMask, dstStageMask, dependencyFlags, std::move(memory_barriers))));
}

void VkVizCommandBuffer::ProcessCommandsNVX(const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) {
    commands_.emplace_back(BasicCommand(CMD_PROCESSCOMMANDSNVX));
    // NVXCommands maybe not?
}

void VkVizCommandBuffer::PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                                       const void* pValues) {
    commands_.emplace_back(BasicCommand(CMD_PUSHCONSTANTS));
    // Write push constants?
}

void VkVizCommandBuffer::PushDescriptorSetKHR(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set,
                                              uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) {
    commands_.emplace_back(BasicCommand(CMD_PUSHDESCRIPTORSETKHR));
    // Write descriptor sets?
}

void VkVizCommandBuffer::PushDescriptorSetWithTemplateKHR(VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                          VkPipelineLayout layout, uint32_t set, const void* pData) {
    commands_.emplace_back(BasicCommand(CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR));
    // Write descriptor sets?
}

void VkVizCommandBuffer::ReserveSpaceForCommandsNVX(const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) {
    commands_.emplace_back(BasicCommand(CMD_RESERVESPACEFORCOMMANDSNVX));
    // NVXCommands maybe not?
}

void VkVizCommandBuffer::ResetEvent(VkEvent event, VkPipelineStageFlags stageMask) {
    commands_.emplace_back(BasicCommand(CMD_RESETEVENT));
    // Write event at stage
}

void VkVizCommandBuffer::ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) {
    commands_.emplace_back(BasicCommand(CMD_RESETQUERYPOOL));
    // Write query pool range
}

void VkVizCommandBuffer::ResolveImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                      VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) {
    commands_.emplace_back(BasicCommand(CMD_RESOLVEIMAGE));
    // Read srcImage regions
    // Write dstImage regions
}

void VkVizCommandBuffer::SetBlendConstants(const float blendConstants[4]) {
    commands_.emplace_back(BasicCommand(CMD_SETBLENDCONSTANTS));
    // Set blend constants
}

void VkVizCommandBuffer::SetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) {
    commands_.emplace_back(BasicCommand(CMD_SETDEPTHBIAS));
    // Set depth bias
}

void VkVizCommandBuffer::SetDepthBounds(float minDepthBounds, float maxDepthBounds) {
    commands_.emplace_back(BasicCommand(CMD_SETDEPTHBOUNDS));
    // Set depth bounds
}

void VkVizCommandBuffer::SetDeviceMask(uint32_t deviceMask) {
    commands_.emplace_back(BasicCommand(CMD_SETDEVICEMASK));
    // Set device mask
}

void VkVizCommandBuffer::SetDeviceMaskKHR(uint32_t deviceMask) {
    commands_.emplace_back(BasicCommand(CMD_SETDEVICEMASKKHR));
    // Set device mask
}

void VkVizCommandBuffer::SetDiscardRectangleEXT(uint32_t firstDiscardRectangle, uint32_t discardRectangleCount,
                                                const VkRect2D* pDiscardRectangles) {
    commands_.emplace_back(BasicCommand(CMD_SETDISCARDRECTANGLEEXT));
    // Set discard rectangle
}

void VkVizCommandBuffer::SetEvent(VkEvent event, VkPipelineStageFlags stageMask) {
    commands_.emplace_back(BasicCommand(CMD_SETEVENT));
    // Set event at stage mask
}

void VkVizCommandBuffer::SetLineWidth(float lineWidth) {
    commands_.emplace_back(BasicCommand(CMD_SETLINEWIDTH));
    // Set line width
}

void VkVizCommandBuffer::SetSampleLocationsEXT(const VkSampleLocationsInfoEXT* pSampleLocationsInfo) {
    commands_.emplace_back(BasicCommand(CMD_SETSAMPLELOCATIONSEXT));
    // Set sample locations
}

void VkVizCommandBuffer::SetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) {
    commands_.emplace_back(BasicCommand(CMD_SETSCISSOR));
    // Set scissor
}

void VkVizCommandBuffer::SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask) {
    commands_.emplace_back(BasicCommand(CMD_SETSTENCILCOMPAREMASK));
    // Set stencil compare
}

void VkVizCommandBuffer::SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference) {
    commands_.emplace_back(BasicCommand(CMD_SETSTENCILREFERENCE));
    // Set stencil reference
}

void VkVizCommandBuffer::SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask) {
    commands_.emplace_back(BasicCommand(CMD_SETSTENCILWRITEMASK));
    // Set stencil write mask
}

void VkVizCommandBuffer::SetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) {
    commands_.emplace_back(BasicCommand(CMD_SETVIEWPORT));
    // Set viewport
}

void VkVizCommandBuffer::SetViewportWScalingNV(uint32_t firstViewport, uint32_t viewportCount,
                                               const VkViewportWScalingNV* pViewportWScalings) {
    commands_.emplace_back(BasicCommand(CMD_SETVIEWPORTWSCALINGNV));
    // Set viewport
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
