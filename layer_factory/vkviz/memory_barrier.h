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

#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

#include <vulkan_core.h>
#include <fstream>
#include <memory>
#include <cassert>
#include <vector>

#include "device.h"
#include "serialize.h"

struct MemoryBarrier {
    VkAccessFlags src_access_mask;
    VkAccessFlags dst_access_mask;

    MemoryBarrier() = default;
    MemoryBarrier(const VkMemoryBarrier& barrier)
        : src_access_mask(barrier.srcAccessMask), dst_access_mask(barrier.dstAccessMask) {}
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
        : src_access_mask(srcAccessMask), dst_access_mask(dstAccessMask) {}
};
SERIALIZE2(MemoryBarrier, src_access_mask, dst_access_mask);

struct BufferBarrier : MemoryBarrier {
    VkBuffer buffer;
    // VkDeviceSize offset;
    // VkDeviceSize size;

    BufferBarrier() = default;
    BufferBarrier(const VkBufferMemoryBarrier& barrier)
        : MemoryBarrier(barrier.srcAccessMask, barrier.dstAccessMask), buffer(barrier.buffer) {}
};
SERIALIZE3(BufferBarrier, src_access_mask, dst_access_mask, buffer);

struct ImageBarrier : MemoryBarrier {
    VkImage image;
    // VkImageSubresourceRange subresource_range;

    ImageBarrier() = default;
    ImageBarrier(const VkImageMemoryBarrier& barrier)
        : MemoryBarrier(barrier.srcAccessMask, barrier.dstAccessMask), image(barrier.image) {}
};
SERIALIZE3(ImageBarrier, src_access_mask, dst_access_mask, image);

struct VkVizPipelineBarrier {
    VkPipelineStageFlags src_stage_mask;
    VkPipelineStageFlags dst_stage_mask;
    VkDependencyFlags dependency_flags;
    std::vector<MemoryBarrier> global_barriers;
    std::vector<BufferBarrier> buffer_barriers;
    std::vector<ImageBarrier> image_barriers;

    VkVizPipelineBarrier() = default;
    VkVizPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                         std::vector<MemoryBarrier> global_barriers, std::vector<BufferBarrier> buffer_barriers,
                         std::vector<ImageBarrier> image_barriers)
        : src_stage_mask(srcStageMask),
          dst_stage_mask(dstStageMask),
          dependency_flags(dependencyFlags),
          global_barriers(global_barriers),
          buffer_barriers(buffer_barriers),
          image_barriers(image_barriers) {}
};
SERIALIZE6(VkVizPipelineBarrier, src_stage_mask, dst_stage_mask, dependency_flags, global_barriers, image_barriers,
           buffer_barriers);

#endif  // MEMORY_BARRIER_H
