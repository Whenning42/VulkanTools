#include "vkviz.h"

#include <algorithm>

std::string cmdToString(CMD_TYPE cmd) {
    switch (cmd) {
        case CMD_NONE:
            return "CMD_NONE";
        case CMD_BEGINCOMMANDBUFFER:  // Should be the first command
            return "CMD_BEGINCOMMANDBUFFER";
        case CMD_BEGINDEBUGUTILSLABELEXT:
            return "CMD_BEGINDEBUGUTILSLABELEXT";
        case CMD_BEGINQUERY:
            return "CMD_BEGINQUERY";
        case CMD_BEGINRENDERPASS:
            return "CMD_BEGINRENDERPASS";
        case CMD_BINDDESCRIPTORSETS:
            return "CMD_BINDDESCRIPTORSETS";
        case CMD_BINDINDEXBUFFER:
            return "CMD_BINDINDEXBUFFER";
        case CMD_BINDPIPELINE:
            return "CMD_BINDPIPELINE";
        case CMD_BINDVERTEXBUFFERS:
            return "CMD_BINDVERTEXBUFFERS";
        case CMD_BLITIMAGE:
            return "CMD_BLITIMAGE";
        case CMD_CLEARATTACHMENTS:
            return "CMD_CLEARATTACHMENTS";
        case CMD_CLEARCOLORIMAGE:
            return "CMD_CLEARCOLORIMAGE";
        case CMD_CLEARDEPTHSTENCILIMAGE:
            return "CMD_CLEARDEPTHSTENCILIMAGE";
        case CMD_COPYBUFFER:
            return "CMD_COPYBUFFER";
        case CMD_COPYBUFFERTOIMAGE:
            return "CMD_COPYBUFFERTOIMAGE";
        case CMD_COPYIMAGE:
            return "CMD_COPYIMAGE";
        case CMD_COPYIMAGETOBUFFER:
            return "CMD_COPYIMAGETOBUFFER";
        case CMD_COPYQUERYPOOLRESULTS:
            return "CMD_COPYQUERYPOOLRESULTS";
        case CMD_DEBUGMARKERBEGINEXT:
            return "CMD_DEBUGMARKERBEGINEXT";
        case CMD_DEBUGMARKERENDEXT:
            return "CMD_DEBUGMARKERENDEXT";
        case CMD_DEBUGMARKERINSERTEXT:
            return "CMD_DEBUGMARKERINSERTEXT";
        case CMD_DISPATCH:
            return "CMD_DISPATCH";
        case CMD_DISPATCHBASE:
            return "CMD_DISPATCHBASE";
        case CMD_DISPATCHBASEKHR:
            return "CMD_DISPATCHBASEKHR";
        case CMD_DISPATCHINDIRECT:
            return "CMD_DISPATCHINDIRECT";
        case CMD_DRAW:
            return "CMD_DRAW";
        case CMD_DRAWINDEXED:
            return "CMD_DRAWINDEXED";
        case CMD_DRAWINDEXEDINDIRECT:
            return "CMD_DRAWINDEXEDINDIRECT";
        case CMD_DRAWINDEXEDINDIRECTCOUNTAMD:
            return "CMD_DRAWINDEXEDINDIRECTCOUNTAMD";
        case CMD_DRAWINDIRECT:
            return "CMD_DRAWINDIRECT";
        case CMD_DRAWINDIRECTCOUNTAMD:
            return "CMD_DRAWINDIRECTCOUNTAMD";
        case CMD_ENDCOMMANDBUFFER:  // Should be the last command in any RECORDED cmd buffer
            return "CMD_ENDCOMMANDBUFFER";
        case CMD_ENDDEBUGUTILSLABELEXT:
            return "CMD_ENDDEBUGUTILSLABELEXT";
        case CMD_ENDQUERY:
            return "CMD_ENDQUERY";
        case CMD_ENDRENDERPASS:
            return "CMD_ENDRENDERPASS";
        case CMD_EXECUTECOMMANDS:
            return "CMD_EXECUTECOMMANDS";
        case CMD_FILLBUFFER:
            return "CMD_FILLBUFFER";
        case CMD_INSERTDEBUGUTILSLABELEXT:
            return "CMD_INSERTDEBUGUTILSLABELEXT";
        case CMD_NEXTSUBPASS:
            return "CMD_NEXTSUBPASS";
        case CMD_PIPELINEBARRIER:
            return "CMD_PIPELINEBARRIER";
        case CMD_PROCESSCOMMANDSNVX:
            return "CMD_PROCESSCOMMANDSNVX";
        case CMD_PUSHCONSTANTS:
            return "CMD_PUSHCONSTANTS";
        case CMD_PUSHDESCRIPTORSETKHR:
            return "CMD_PUSHDESCRIPTORSETKHR";
        case CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR:
            return "CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR";
        case CMD_RESERVESPACEFORCOMMANDSNVX:
            return "CMD_RESERVESPACEFORCOMMANDSNVX";
        case CMD_RESETCOMMANDBUFFER:
            return "CMD_RESETCOMMANDBUFFER";
        case CMD_RESETEVENT:
            return "CMD_RESETEVENT";
        case CMD_RESETQUERYPOOL:
            return "CMD_RESETQUERYPOOL";
        case CMD_RESOLVEIMAGE:
            return "CMD_RESOLVEIMAGE";
        case CMD_SETBLENDCONSTANTS:
            return "CMD_SETBLENDCONSTANTS";
        case CMD_SETDEPTHBIAS:
            return "CMD_SETDEPTHBIAS";
        case CMD_SETDEPTHBOUNDS:
            return "CMD_SETDEPTHBOUNDS";
        case CMD_SETDEVICEMASK:
            return "CMD_SETDEVICEMASK";
        case CMD_SETDEVICEMASKKHR:
            return "CMD_SETDEVICEMASKKHR";
        case CMD_SETDISCARDRECTANGLEEXT:
            return "CMD_SETDISCARDRECTANGLEEXT";
        case CMD_SETEVENT:
            return "CMD_SETEVENT";
        case CMD_SETLINEWIDTH:
            return "CMD_SETLINEWIDTH";
        case CMD_SETSAMPLELOCATIONSEXT:
            return "CMD_SETSAMPLELOCATIONSEXT";
        case CMD_SETSCISSOR:
            return "CMD_SETSCISSOR";
        case CMD_SETSTENCILCOMPAREMASK:
            return "CMD_SETSTENCILCOMPAREMASK";
        case CMD_SETSTENCILREFERENCE:
            return "CMD_SETSTENCILREFERENCE";
        case CMD_SETSTENCILWRITEMASK:
            return "CMD_SETSTENCILWRITEMASK";
        case CMD_SETVIEWPORT:
            return "CMD_SETVIEWPORT";
        case CMD_SETVIEWPORTWSCALINGNV:
            return "CMD_SETVIEWPORTWSCALINGNV";
        case CMD_UPDATEBUFFER:
            return "CMD_UPDATEBUFFER";
        case CMD_WAITEVENTS:
            return "CMD_WAITEVENTS";
        case CMD_WRITEBUFFERMARKERAMD:
            return "CMD_WRITEBUFFERMARKERAMD";
        case CMD_WRITETIMESTAMP:
            return "CMD_WRITETIMESTAMP";
        default:
            printf("Unknown command: %d\n", cmd);
            return "CMD_UNKNOWN";
    }
};

// This function will be called for every API call
// void VkViz::PreCallApiFunction(const char *api_name) { printf("Calling %s\n", api_name); }

// TODO(whenning): ResetCommandBuffer is missing
// TODO(whenning): FreeCommandBuffers is missing
// TODO(whenning): SubmitInfo is missing
// TODO(whenning): AllocateCommandBuffers is missing

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

template <typename T>
MemoryAccess BufferRead(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(READ, buffer, regionCount, pRegions);
}

template <typename T>
MemoryAccess BufferWrite(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(WRITE, buffer, regionCount, pRegions);
}

VkResult VkViz::PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BEGINCOMMANDBUFFER);
    // Start buffer
}
VkResult VkViz::PostCallEndCommandBuffer(VkCommandBuffer commandBuffer) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_ENDCOMMANDBUFFER);
    // End buffer
}
void VkViz::PostCallCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BEGINDEBUGUTILSLABELEXT);
}
void VkViz::PostCallCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BEGINQUERY);
}
void VkViz::PostCallCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
                                       VkSubpassContents contents) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BEGINRENDERPASS);
}
void VkViz::PostCallCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                          VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
                                          const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
                                          const uint32_t* pDynamicOffsets) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BINDDESCRIPTORSETS);
}
void VkViz::PostCallCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BINDINDEXBUFFER);
}
void VkViz::PostCallCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BINDPIPELINE);
}
void VkViz::PostCallCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                         const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BINDVERTEXBUFFERS);
}
void VkViz::PostCallCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BLITIMAGE);

    AddAccess(commandBuffer, ImageRead(srcImage, regionCount, pRegions), CMD_BLITIMAGE);
    AddAccess(commandBuffer, ImageWrite(dstImage, regionCount, pRegions), CMD_BLITIMAGE);
}
void VkViz::PostCallCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                        const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_CLEARATTACHMENTS);

    //TODO(whenning): fix this
    //AddAccess(commandBuffer, ImageWrite(attachmentCount, pAttachments, rectCount, pRects));
}
void VkViz::PostCallCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                       const VkClearColorValue* pColor, uint32_t rangeCount,
                                       const VkImageSubresourceRange* pRanges) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_CLEARCOLORIMAGE);

    AddAccess(commandBuffer, ImageWrite(image, rangeCount, pRanges), CMD_CLEARCOLORIMAGE);
}
void VkViz::PostCallCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                              const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
                                              const VkImageSubresourceRange* pRanges) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_CLEARDEPTHSTENCILIMAGE);

    AddAccess(commandBuffer, ImageWrite(image, rangeCount, pRanges), CMD_CLEARDEPTHSTENCILIMAGE);
}
void VkViz::PostCallCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount,
                                  const VkBufferCopy* pRegions) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_COPYBUFFER);

    AddAccess(commandBuffer, BufferRead(srcBuffer, regionCount, pRegions), CMD_COPYBUFFER);
    AddAccess(commandBuffer, BufferWrite(dstBuffer, regionCount, pRegions), CMD_COPYBUFFER);
}
void VkViz::PostCallCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                         VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_COPYBUFFERTOIMAGE);

    AddAccess(commandBuffer, BufferRead(srcBuffer, regionCount, pRegions), CMD_COPYBUFFERTOIMAGE);
    AddAccess(commandBuffer, ImageWrite(dstImage, regionCount, pRegions), CMD_COPYBUFFERTOIMAGE);
}
void VkViz::PostCallCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                 VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_COPYIMAGE);

    AddAccess(commandBuffer, ImageRead(srcImage, regionCount, pRegions), CMD_COPYIMAGE);
    AddAccess(commandBuffer, ImageWrite(dstImage, regionCount, pRegions), CMD_COPYIMAGE);
}
void VkViz::PostCallCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                         VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_COPYIMAGETOBUFFER);

    AddAccess(commandBuffer, ImageRead(srcImage, regionCount, pRegions), CMD_COPYIMAGETOBUFFER);
    AddAccess(commandBuffer, BufferWrite(dstBuffer, regionCount, pRegions), CMD_COPYIMAGETOBUFFER);
}
void VkViz::PostCallCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                            uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
                                            VkQueryResultFlags flags) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_COPYQUERYPOOLRESULTS);
    // Implement later?
    // Read queryPool range

    // We don't really know the write size unless we keep track of
    // GetQueryPoolResults I think
    // Write dstBuffer range
}
void VkViz::PostCallCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DEBUGMARKERBEGINEXT);
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}
void VkViz::PostCallCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DEBUGMARKERENDEXT);
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}
void VkViz::PostCallCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DEBUGMARKERINSERTEXT);
    // TODO(whenning): Not sure what the synchronization details are
    // Implement later?
}
void VkViz::PostCallCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DISPATCH);
    // Compute pipeline synchronization
}
void VkViz::PostCallCmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                    uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DISPATCHBASE);
    // Compute pipeline synchronization
}
void VkViz::PostCallCmdDispatchBaseKHR(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                       uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DISPATCHBASEKHR);
    // Compute pipeline synchronization
}
void VkViz::PostCallCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DISPATCHINDIRECT);
    // Compute pipeline synchronization
    // Read buffer
}
void VkViz::PostCallCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                            uint32_t firstInstance) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DRAW);
    // Graphics pipeline synchronization
}
void VkViz::PostCallCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                   int32_t vertexOffset, uint32_t firstInstance) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DRAWINDEXED);
    // Graphics pipeline synchronization
    // Read index buffer (bound by CmdBindIndexBuffer, not an arg)
}
void VkViz::PostCallCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                           uint32_t stride) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DRAWINDEXEDINDIRECT);
    // Graphics pipeline synchronization
    // Read buffer
}
void VkViz::PostCallCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                   VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                                   uint32_t stride) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DRAWINDEXEDINDIRECTCOUNTAMD);
    // Graphics pipeline synchronization
    // Read buffer
}
void VkViz::PostCallCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                    uint32_t stride) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DRAWINDIRECT);
    // Graphics pipeline synchronization
    // Read buffer
}
void VkViz::PostCallCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                            VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                            uint32_t stride) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_DRAWINDIRECTCOUNTAMD);
    // Graphics pipeline synchronization
    // Read buffer
    // Read countBuffer
}
void VkViz::PostCallCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_ENDDEBUGUTILSLABELEXT);
    // TODO(whenning): unsure
}
void VkViz::PostCallCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_ENDQUERY);
    // TODO(whenning): unsure
}
void VkViz::PostCallCmdEndRenderPass(VkCommandBuffer commandBuffer) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_ENDRENDERPASS);
    // Finish up renderpass logic?
}
void VkViz::PostCallCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_EXECUTECOMMANDS);
    // Secondary command buffer
}
void VkViz::PostCallCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size,
                                  uint32_t data) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_FILLBUFFER);
    // Write dstBuffer range
}
void VkViz::PostCallCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_INSERTDEBUGUTILSLABELEXT);
    // TODO(whenning): unsure
}
void VkViz::PostCallCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_NEXTSUBPASS);
    // TODO(whenning): Subpass transition?
}
void VkViz::PostCallCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                       uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
                                       uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                                       uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_PIPELINEBARRIER);
    // Add a pipeline barrier
}
void VkViz::PostCallCmdProcessCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_PROCESSCOMMANDSNVX);
    // NVXCommands maybe not?
}
void VkViz::PostCallCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                     uint32_t offset, uint32_t size, const void* pValues) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_PUSHCONSTANTS);
    // Write push constants?
}
void VkViz::PostCallCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                            VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount,
                                            const VkWriteDescriptorSet* pDescriptorWrites) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_PUSHDESCRIPTORSETKHR);
    // Write descriptor sets?
}
void VkViz::PostCallCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer,
                                                        VkDescriptorUpdateTemplate descriptorUpdateTemplate,
                                                        VkPipelineLayout layout, uint32_t set, const void* pData) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_PUSHDESCRIPTORSETWITHTEMPLATEKHR);
    // Write descriptor sets?
}
void VkViz::PostCallCmdReserveSpaceForCommandsNVX(VkCommandBuffer commandBuffer,
                                                  const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_RESERVESPACEFORCOMMANDSNVX);
    // NVXCommands maybe not?
}
void VkViz::PostCallCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_RESETEVENT);
    // Write event at stage
}
void VkViz::PostCallCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                      uint32_t queryCount) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_RESETQUERYPOOL);
    // Write query pool range
}
void VkViz::PostCallCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                                    VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_RESOLVEIMAGE);
    // Read srcImage regions
    // Write dstImage regions
}
void VkViz::PostCallCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETBLENDCONSTANTS);
    // Set blend constants
}
void VkViz::PostCallCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                    float depthBiasSlopeFactor) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETDEPTHBIAS);
    // Set depth bias
}
void VkViz::PostCallCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETDEPTHBOUNDS);
    // Set depth bounds
}
void VkViz::PostCallCmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETDEVICEMASK);
    // Set device mask
}
void VkViz::PostCallCmdSetDeviceMaskKHR(VkCommandBuffer commandBuffer, uint32_t deviceMask) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETDEVICEMASKKHR);
    // Set device mask
}
void VkViz::PostCallCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle,
                                              uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETDISCARDRECTANGLEEXT);
    // Set discard rectangle
}
void VkViz::PostCallCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETEVENT);
    // Set event at stage mask
}
void VkViz::PostCallCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETLINEWIDTH);
    // Set line width
}
void VkViz::PostCallCmdSetSampleLocationsEXT(VkCommandBuffer commandBuffer, const VkSampleLocationsInfoEXT* pSampleLocationsInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETSAMPLELOCATIONSEXT);
    // Set sample locations
}
void VkViz::PostCallCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                                  const VkRect2D* pScissors) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETSCISSOR);
    // Set scissor
}
void VkViz::PostCallCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETSTENCILCOMPAREMASK);
    // Set stencil compare
}
void VkViz::PostCallCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETSTENCILREFERENCE);
    // Set stencil reference
}
void VkViz::PostCallCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETSTENCILWRITEMASK);
    // Set stencil write mask
}
void VkViz::PostCallCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                   const VkViewport* pViewports) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETVIEWPORT);
    // Set viewport
}
void VkViz::PostCallCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                             const VkViewportWScalingNV* pViewportWScalings) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_SETVIEWPORTWSCALINGNV);
    // Set viewport
}
void VkViz::PostCallCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                    VkDeviceSize dataSize, const void* pData) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_UPDATEBUFFER);
    // Write dstBuffer range
}
void VkViz::PostCallCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
                                  VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount,
                                  const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount,
                                  const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
                                  const VkImageMemoryBarrier* pImageMemoryBarriers) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_WAITEVENTS);
    // Synchronization
}
void VkViz::PostCallCmdWriteBufferMarkerAMD(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
                                            VkBuffer dstBuffer, VkDeviceSize dstOffset, uint32_t marker) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_WRITEBUFFERMARKERAMD);
    // Write to dstBuffer after pipelineStage
    // Maybe not implement?
}
void VkViz::PostCallCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
                                      uint32_t query) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_WRITETIMESTAMP);
    // Write to queryPool after pipelineStage
    // maybe not implement?
}
VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    // Write dot file with cmd buffers for each submit
    uint32_t node_num = 0;
    std::string filename = outfile_base_name_ + std::to_string(outfile_num_) + ".dot";
    outfile_.open(filename);
    //printf("Opened: %s for writing.\n", filename.c_str());
    // Write graphviz of cmd buffers
    outfile_ << "digraph G {\n  node [shape=record];\n";
    for (uint32_t i = 0; i < submitCount; ++i) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; ++j) {
            outfile_ << "  node" << node_num << "[ label = \"{<n> COMMAND BUFFER 0x" << pSubmits[i].pCommandBuffers[j] << " ";
            for (const auto& cmd : cmdbuffer_map_[pSubmits[i].pCommandBuffers[j]]) {
                outfile_ << "| " << cmdToString(cmd);
            }
            outfile_ << "];\n";
        }
    }
    outfile_ << "}";
    outfile_.close();
}

VkViz vkviz_layer;
