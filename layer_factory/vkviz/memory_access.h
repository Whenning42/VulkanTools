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

#ifndef MEMORY_ACCESS_H
#define MEMORY_ACCESS_H

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <vulkan_core.h>
#include <fstream>

#include "serialize.h"
#include "command_enums.h"

class VkVizImage {
    VkImage image_;
    VkDevice device_;

   public:
    VkVizImage(VkImage image, VkDevice device) : image_(image), device_(device) {}
};

class VkVizBuffer {
    VkBuffer buffer_;
    VkDevice device_;

   public:
    VkVizBuffer(VkBuffer buffer, VkDevice device) : buffer_(buffer), device_(device) {}
};

class VkVizImageView {
    // Subset of VkImageViewCreateInfo members
    VkImage image;
    VkImageSubresourceRange subresourceRange;

   public:
    VkImage Image() { return image; }
};

enum MEMORY_TYPE { IMAGE_MEMORY, BUFFER_MEMORY };
std::string memoryTypeString(MEMORY_TYPE type);

enum READ_WRITE { READ, WRITE };
std::string readWriteString(READ_WRITE rw);

inline VkExtent3D extentFromOffsets(VkOffset3D first, VkOffset3D second) {
    return {(uint32_t)std::abs((int64_t)first.x - second.x), (uint32_t)std::abs((int64_t)first.y - second.y),
            (uint32_t)std::abs((int64_t)first.z - second.z)};
}

inline VkOffset3D cornerFromOffsets(VkOffset3D first, VkOffset3D second) {
    return {std::min(first.x, second.x), std::min(first.y, second.y), std::min(first.z, second.z)};
}

inline VkImageSubresourceRange RangeFromLayers(VkImageSubresourceLayers layers) {
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

    ImageRegion() = default;
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
// We haven't implemented ImageRegion serialization yet.
SERIALIZE0(ImageRegion);

struct ImageAccess {
    VkImage image;
    std::vector<ImageRegion> regions;
};
SERIALIZE2(ImageAccess, image, regions);

struct BufferRegion {
    VkDeviceSize offset;
    VkDeviceSize size;
    uint32_t row_length = 0;
    uint32_t image_height = 0;

    BufferRegion() = default;
    BufferRegion(VkDeviceSize offset, VkDeviceSize size) : offset(offset), size(size){};

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
        image_height = copy.bufferImageHeight;
    }
};
// BufferRegion serialization is unimplemented.
SERIALIZE0(BufferRegion);

struct BufferAccess {
    VkBuffer buffer;
    std::vector<BufferRegion> regions;
};
SERIALIZE2(BufferAccess, buffer, regions);

struct MemoryAccess {
    READ_WRITE read_or_write;
    MEMORY_TYPE type;
    BufferAccess buffer_access;
    ImageAccess image_access;
    VkPipelineStageFlagBits pipeline_stage;

    MemoryAccess(){};
    MemoryAccess(READ_WRITE rw, BufferAccess buffer_access, VkPipelineStageFlagBits pipeline_stage)
        : read_or_write(rw), type(BUFFER_MEMORY), buffer_access(std::move(buffer_access)), pipeline_stage(pipeline_stage) {}
    MemoryAccess(READ_WRITE rw, ImageAccess image_access, VkPipelineStageFlagBits pipeline_stage)
        : read_or_write(rw), type(IMAGE_MEMORY), image_access(std::move(image_access)), pipeline_stage(pipeline_stage) {}

    // Returning a void* only makes sense because we're using the unique objects layer.
    void* Handle() const {
        if(type == BUFFER_MEMORY) {
            return static_cast<void*>(buffer_access.buffer);
        } else {
            return static_cast<void*>(image_access.image);
        }
    }

    template <typename T>
    static MemoryAccess Image(READ_WRITE rw, VkImage image, uint32_t regionCount, const T* pRegions,
                              VkPipelineStageFlagBits pipeline_stage) {
        std::vector<ImageRegion> regions;
        for(uint32_t i=0; i<regionCount; ++i) {
            regions.push_back(ImageRegion(rw, pRegions[i]));
        }
        return MemoryAccess(rw, ImageAccess({image, std::move(regions)}), pipeline_stage);
    }

    template <typename T>
    static MemoryAccess Buffer(READ_WRITE rw, VkBuffer buffer, uint32_t regionCount, const T* pRegions,
                               VkPipelineStageFlagBits pipeline_stage) {
        std::vector<BufferRegion> regions;
        for(uint32_t i=0; i<regionCount; ++i) {
            regions.push_back(BufferRegion(rw, pRegions[i]));
        }
        return MemoryAccess(rw, BufferAccess{buffer, std::move(regions)}, pipeline_stage);
    }

    static MemoryAccess Buffer(READ_WRITE rw, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size,
                               VkPipelineStageFlagBits pipeline_stage) {
        return MemoryAccess(rw, BufferAccess{buffer, {BufferRegion(offset, size)}}, pipeline_stage);
    }
};
SERIALIZE5(MemoryAccess, read_or_write, type, image_access, buffer_access, pipeline_stage);

#endif  // MEMORY_ACCESS_H
