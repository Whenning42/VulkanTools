#include "vkviz.h"
#include <algorithm>

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
        std::vector<ImageRegion> regions = ImageRegion::RegionsFromView(image_view, regionCount, pRegions);
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
    return MemoryAccess::ImageView(READ, image_view, regionCount, pRegions);
}

// Generates corresponding ImageWrite()
template <typename T>
MemoryAccess ImageViewWrite(VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::ImageView(Write, image_view, regionCount, pRegions);
}

template <typename T>
MemoryAccess BufferRead(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(READ, buffer, regionCount, pRegions);
}

template <typename T>
MemoryAccess BufferWrite(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(WRITE, buffer, regionCount, pRegions);
}

void VkViz::AddCommand(VkCommandBuffer cmd_buffer, CMD_TYPE type) {
    cmdbuffer_map_[cmd_buffer].push_back(type);
    cmd_buffer_was_printed[cmd_buffer] = false;
}

VkResult VkViz::PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_BEGINCOMMANDBUFFER);
}

VkResult VkViz::PostCallEndCommandBuffer(VkCommandBuffer commandBuffer) {
    cmdbuffer_map_[commandBuffer].push_back(CMD_ENDCOMMANDBUFFER);
}

VkResult VkViz::PostCallResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    cmdbuffer_map_[commandBuffer].clear();
}


std::vector<MemoryAccess> ClearAttachments(VkVizRenderPass render_pass, uint32_t attachment_count, const VkClearAttachment* p_attachments, uint32_t rect_count, const VkClearRect* p_rects) {
    std::vector<MemoryAccess> accesses;
    const SubPass sub_pass = render_pass.SubPass();

    for(uint32_t i=0; i<attachment_count; ++i) {
        const VkClearAttachment& clear = p_attachments[i];

        if(clear.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT && clear.colorAttachment != VK_ATTACHMENT_UNUSED) {
            ImageView& cleared_view = sub_pass.ColorAttachments()[clear.colorAttachment].ImageView();
            accesses.push_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }

        if(clear.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT && sub_pass.HasDepthAttachment()) {
            ImageView& cleared_view = sub_pass.DepthAttachment().ImageView();
            accesses.push_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }

        if(clear.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT && sub_pass.HasStencilAttachment()) {
            ImageView& cleared_view = sub_pass.StencilAttachment().ImageView();
            accesses.push_back(ImageViewWrite(cleared_view, rect_count, p_rects));
        }
    }

    return accesses;
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

VkResult VKViz::PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    GetCommandBuffer(commandBuffer).Begin();
}

VkResult VKViz::PostCallEndCommandBuffer(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).End(); }

VkResult VKViz::PostCallResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    GetCommandBuffer(commandBuffer).Reset();
}

VkResult VKViz::PostCallAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
                                               VkCommandBuffer* pCommandBuffers) {
    AddCommandBuffers(pAllocateInfo, pCommandBuffers);
}

void VKViz::PostCallFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uin32_t commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers) {
    RemoveCommandBuffers(commandBufferCount, pCommandBuffers);
}

VkResult VKViz::PostCallCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    AddRenderPass(pRenderPass, pCreateInfo);
}

void VKViz::PostCallDestroyRenderPass(VkDevice deice, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    RemoveRenderPass(renderPass);
}

VkResult VKViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {}

// An instance needs to be declared to turn on a layer in the layer_factory framework
VkViz vkviz_layer;
