#include "commands.h"

#include <algorithm>


/* CodeGen? */
class VkVizImageView {
    // Subset of VkImageViewCreateInfo members
    VkImage image;
    VkImageSubresourceRange subresourceRange;

 public:
    VkImage Image() {return image;}
};


// TODO clean this up
enum MEMORY_TYPE { IMAGE_MEMORY, BUFFER_MEMORY };

enum READ_WRITE { READ, WRITE };

VkExtent3D extentFromOffsets(VkOffset3D first, VkOffset3D second) {
    return {(uint32_t)std::abs((int64_t)first.x - second.x), (uint32_t)std::abs((int64_t)first.y - second.y),
            (uint32_t)std::abs((int64_t)first.z - second.z)};
}

VkOffset3D cornerFromOffsets(VkOffset3D first, VkOffset3D second) {
    return {std::min(first.x, second.x), std::min(first.y, second.y), std::min(first.z, second.z)};
}


VkImageSubresourceRange RangeFromLayers(VkImageSubresourceLayers layers) {
    VkImageSubresourceRange range;
    range.aspectMask = layers.aspectMask;
    range.baseMipLevel = layers.mipLevel;
    range.levelCount = 1;
    range.baseArrayLayer = layers.baseArrayLayer;
    range.layerCount = layers.layerCount;
    return range;
}

struct ImageRegion {
    VkImageSubresourceRange subresource;
    VkOffset3D offset;
    VkExtent3D extent;
    bool entire_image = false;

    ImageRegion(READ_WRITE rw, const VkImageBlit& blit) {
        if (rw == READ) {
            subresource = RangeFromLayers(blit.srcSubresource);
            offset = cornerFromOffsets(blit.srcOffsets[0], blit.srcOffsets[1]);
            extent = extentFromOffsets(blit.srcOffsets[0], blit.srcOffsets[1]);
        } else {
            subresource = RangeFromLayers(blit.dstSubresource);
            offset = cornerFromOffsets(blit.dstOffsets[0], blit.dstOffsets[1]);
            extent = extentFromOffsets(blit.dstOffsets[0], blit.dstOffsets[1]);
        }
    }

    ImageRegion(READ_WRITE rw, const VkImageCopy& copy) {
        if (rw == READ) {
            subresource = RangeFromLayers(copy.srcSubresource);
            offset = copy.srcOffset;
            extent = copy.extent;
        } else {
            subresource = RangeFromLayers(copy.dstSubresource);
            offset = copy.dstOffset;
            extent = copy.extent;
        }
    }

    ImageRegion(READ_WRITE rw, const VkBufferImageCopy& copy) {
        subresource = RangeFromLayers(copy.imageSubresource);
        offset = copy.imageOffset;
        extent = copy.imageExtent;
    }

    ImageRegion(READ_WRITE rw, const VkImageSubresourceRange& subresource_range) {
        subresource = subresource_range;
        entire_image = true;
    }
};

struct ImageAccess {
    VkImage image;
    std::vector<ImageRegion> regions;
};

struct BufferRegion {
    VkDeviceSize offset;
    VkDeviceSize size = 0;
    uint32_t row_length = 0;
    uint32_t image_height = 0;

    BufferRegion(READ_WRITE rw, const VkBufferCopy& copy) {
        if (rw == READ) {
            offset = copy.srcOffset;
        } else {
            offset = copy.dstOffset;
        }
        size = copy.size;
    }

    BufferRegion(READ_WRITE rw, const VkBufferImageCopy& copy) {
        offset = copy.bufferOffset;
        row_length = copy.bufferRowLength;
        image_height= copy.bufferImageHeight;
    }
};

struct BufferAccess {
    VkBuffer buffer;
    std::vector<BufferRegion> regions;
};

struct MemoryAccess {
    bool is_read;
    MEMORY_TYPE type;
    void* data;

    template <typename T>
    static MemoryAccess Image(READ_WRITE rw, VkImage image, uint32_t regionCount, const T* pRegions) {
        MemoryAccess access;
        access.type = IMAGE_MEMORY;
        access.is_read = rw;

        std::vector<ImageRegion> regions;
        std::transform(pRegions, pRegions + regionCount, std::back_inserter(regions),
                       [rw](const T& region) { return ImageRegion(rw, region); });

        access.data = new ImageAccess({image, regions});
        return access;
    }

    template <typename T>
    static MemoryAccess Buffer(READ_WRITE rw, VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
        MemoryAccess access;
        access.type = BUFFER_MEMORY;
        access.is_read = rw;

        std::vector<BufferRegion> regions;
        std::transform(pRegions, pRegions + regionCount, std::back_inserter(regions),
                       [rw](const T& region) { return BufferRegion(rw, region);});

        access.data = new BufferAccess({buffer, regions});
        return access;
    }

    template <typename T>
    static MemoryAccess ImageView(READ_WRITE rw, VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
        std::vector<ImageRegion> regions = ImageRegion(image_view, regionCount, pRegions);
        return Image(rw, image_view.Image(), regions.size(), regions.data());
    }
};

void AddAccess(VkCommandBuffer cmdBuffer, MemoryAccess access, CMD_TYPE type) {
    printf("Memory access in cmd buffer: %d.\n", cmdBuffer);
    printf("For command: %s.\n", cmdToString(type).c_str());
    switch (access.is_read) {
        case READ:
            printf("Was a read.\n");
        case WRITE:
            printf("Was a write.\n");
    }

    switch (access.type) {
       case IMAGE_MEMORY: {
            ImageAccess data = *(ImageAccess*)access.data;
            printf("Of image: %d.\n", data.image);
       }
       case BUFFER_MEMORY: {
            BufferAccess data = *(BufferAccess*)access.data;
            printf("Of buffer: %d.\n", data.buffer);
       }
    }
    return;
}

template <typename T>
MemoryAccess ImageRead(VkImage image, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Image(READ, image, regionCount, pRegions);
}

template <typename T>
MemoryAccess ImageWrite(VkImage image, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Image(WRITE, image, regionCount, pRegions);
}

// Generates corresponding ImageRead()
template <typename T>
MemoryAccess ImageViewRead(VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
    assert(0);
    //return MemoryAccess::ImageView(READ, image_view, regionCount, pRegions);
}

// Generates corresponding ImageWrite()
template <typename T>
MemoryAccess ImageViewWrite(VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::ImageView(WRITE, image_view, regionCount, pRegions);
}

template <typename T>
MemoryAccess BufferRead(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(READ, buffer, regionCount, pRegions);
}

template <typename T>
MemoryAccess BufferWrite(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(WRITE, buffer, regionCount, pRegions);
}

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
    static std::unordered_map<VkRenderPass, VkVizRenderPass*> render_passes_;

   public:
    static VkVizRenderPass& Get(VkRenderPass render_pass) { return *render_passes_[render_pass];};

    VkVizRenderPass(VkDevice device, VkRenderPassCreateInfo* pRenderPassCreateInfo, VkRenderPass* pRenderPass) {
        for (int i = 0; i < pRenderPassCreateInfo->attachmentCount; ++i) {
            attachments.push_back(pRenderPassCreateInfo->pAttachments[i]);
        }
        for (int i = 0; i < pRenderPassCreateInfo->subpassCount; ++i) {
            subpasses.push_back(pRenderPassCreateInfo->pSubpasses[i]);
        }
        for (int i = 0; i < pRenderPassCreateInfo->dependencyCount; ++i) {
            dependencies.push_back(pRenderPassCreateInfo->pDependencies[i]);
        }

        render_passes_[*pRenderPass] = this;
    }

    VkSubpassDescription Subpass(size_t index) const {return subpasses[index];}
};

class VkVizFramebuffer {};

class VkVizRenderPassInstance {
    const VkVizRenderPass& render_pass;
    int current_subpass_index = 0;
    VkVizFramebuffer& framebuffer;

   public:
    VkVizRenderPassInstance(VkRenderPass render_pass, VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents)
        : render_pass(VkVizRenderPass::Get(render_pass)), framebuffer(framebuffer){};

    void NextSubpass(VkSubpassContents) { ++current_subpass_index; }

    void EndRenderPass() { current_subpass_index = -1; }

    VkSubpassDescription CurrentSubpass() { return render_pass.Subpass(current_subpass_index); }
};

class Command {
    CMD_TYPE type_ = CMD_NONE;
    std::vector<MemoryAccess> accesses_;

   public:
    Command(CMD_TYPE type) : type_(type){};
    Command(CMD_TYPE type, MemoryAccess access) : type_(type), accesses_({access}){};
    Command(CMD_TYPE type, std::vector<MemoryAccess> accesses) : type_(type), accesses_(accesses){};
};

class VkVizCommandBuffer {
    std::vector<Command> commands_;

    int current_render_pass_ = -1;
    std::vector<VkVizRenderPassInstance> render_pass_instances_;

    void AddCommand(CMD_TYPE type) { commands_.push_back(Command(type)); };
    void AddCommand(CMD_TYPE type, MemoryAccess access) { commands_.push_back(Command(type, access)); };
    void AddCommand(CMD_TYPE type, std::vector<MemoryAccess> accesses) { commands_.push_back(Command(type, accesses)); };

   public:
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
    std::vector<MemoryAccess> ClearAttachments(VkVizRenderPass render_pass, uint32_t attachment_count,
                                               const VkClearAttachment* p_attachments, uint32_t rect_count,
                                               const VkClearRect* p_rects);
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
    void DebugMarkerEndEXT(VkCommandBuffer commandBuffer);
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
    void EndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer);
    void EndQuery(VkQueryPool queryPool, uint32_t query);
    void EndRenderPass(VkCommandBuffer commandBuffer);
    void ExecuteCommands(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
    void FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
    void InsertDebugUtilsLabelEXT(const VkDebugUtilsLabelEXT* pLabelInfo);
    void NextSubpass(VkSubpassContents contents);
    void PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                         uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                         const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                         const VkImageMemoryBarrier* pImageMemoryBarriers);
    void ProcessCommandsNVX(const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo);
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
/* CodeGen? */
