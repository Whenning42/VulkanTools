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

struct ImageBarrier {
    VkImage image;
    VkImageSubresourceRange subresource_range;
};
// We don't serialize subresource_range here since we don't yet track subresources.
SERIALIZE(ImageBarrier, image);

struct BufferBarrier {
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;
};
// We don't serialize offset or size here since we don't yet track subresources.
SERIALIZE(BufferBarrier, buffer);

enum class BarrierType { GLOBAL, BUFFER, IMAGE };
struct MemoryBarrier {
    VkAccessFlags src_access_mask;
    VkAccessFlags dst_access_mask;

    BarrierType barrier_type;
    union {
        BufferBarrier buffer_barrier;
        ImageBarrier image_barrier;
    };

    static MemoryBarrier Global(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        return MemoryBarrier(srcAccessMask, dstAccessMask);
    }

    static MemoryBarrier Buffer(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset,
                                VkDeviceSize size) {
        return MemoryBarrier(srcAccessMask, dstAccessMask, BufferBarrier{buffer, offset, size});
    }

    static MemoryBarrier Image(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image,
                               VkImageSubresourceRange subresourceRange) {
        return MemoryBarrier(srcAccessMask, dstAccessMask, ImageBarrier{image, subresourceRange});
    }

    MemoryBarrier() = default;

   private:
    // These constructors are private to force the use of the more explicit static create functions above
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
        : src_access_mask(srcAccessMask), dst_access_mask(dstAccessMask), barrier_type(BarrierType::GLOBAL){};

    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, BufferBarrier barrier)
        : src_access_mask(srcAccessMask),
          dst_access_mask(dstAccessMask),
          barrier_type(BarrierType::BUFFER),
          buffer_barrier(barrier) {};

    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, ImageBarrier barrier)
        : src_access_mask(srcAccessMask),
          dst_access_mask(dstAccessMask),
          barrier_type(BarrierType::IMAGE),
          image_barrier(barrier) {};

   public:
};

inline void to_json(json& j, const MemoryBarrier& barrier) {
    j = {{"src_access_mask", barrier.src_access_mask}, {"dst_access_mask", barrier.dst_access_mask}, {"barrier_type", barrier.barrier_type}};

    if (barrier.barrier_type == BarrierType::BUFFER) {
        j["buffer_barrier"] = barrier.buffer_barrier;
    } else if (barrier.barrier_type == BarrierType::IMAGE) {
        j["image_barrier"] = barrier.image_barrier;
    }
}
inline void from_json(const json& j, MemoryBarrier& barrier) {
    barrier.src_access_mask = j["src_access_mask"].get<VkAccessFlags>();
    barrier.dst_access_mask = j["dst_access_mask"].get<VkAccessFlags>();
    barrier.barrier_type = j["barrier_type"].get<BarrierType>();

    if (barrier.barrier_type == BarrierType::BUFFER) {
        barrier.buffer_barrier = j["buffer_barrier"].get<BufferBarrier>();
    } else if (barrier.barrier_type == BarrierType::IMAGE) {
        barrier.image_barrier = j["image_barrier"].get<ImageBarrier>();
    }
}

struct VkVizPipelineBarrier {
    VkPipelineStageFlags src_stage_mask;
    VkPipelineStageFlags dst_stage_mask;
    VkDependencyFlags dependency_flags;
    std::vector<MemoryBarrier> memory_barriers;

    VkVizPipelineBarrier() = default;
    VkVizPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                         std::vector<MemoryBarrier> barriers)
        : src_stage_mask(srcStageMask),
          dst_stage_mask(dstStageMask),
          dependency_flags(dependencyFlags),
          memory_barriers(barriers) {}
};
SERIALIZE4(VkVizPipelineBarrier, src_stage_mask, dst_stage_mask, dependency_flags, memory_barriers);

#endif  // MEMORY_BARRIER_H
