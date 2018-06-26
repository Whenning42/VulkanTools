#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

#include <vulkan.h>
#include <fstream>
#include <memory>
#include <vector>

#include "device.h"

#include "third_party/json.hpp"

using json = nlohmann::json;

struct ImageBarrier {
    VkImage image;
    VkImageSubresourceRange subresource_range;

    json Serialize() const {
//        return {{"image", image}, {"subresource", subresource_range}};
        return {};
    }
};

struct BufferBarrier {
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;

    json Serialize() const {
//        return {{"buffer", buffer}, {"offset", offset}, {"size", size}};
        return {};
    }
};

struct MemoryBarrier {
    const VkAccessFlags src_access_mask;
    const VkAccessFlags dst_access_mask;

    enum BarrierType { GLOBAL, BUFFER, IMAGE } const barrier_type;
    union {
        const BufferBarrier buffer_barrier;
        const ImageBarrier image_barrier;
    };

    json Serialize() const {
        json serialized = {{"source access mask", src_access_mask}, {"destination access mask", dst_access_mask}};

        if (barrier_type == BUFFER) {
            serialized["buffer barrier"] = buffer_barrier.Serialize();
        } else if (barrier_type == IMAGE) {
            serialized["image barrier"] = image_barrier.Serialize();
        }
        return serialized;
    }

    static MemoryBarrier Global(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        return MemoryBarrier(srcAccessMask, dstAccessMask);
    }

    static MemoryBarrier Buffer(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset,
                                VkDeviceSize size) {
        return MemoryBarrier(srcAccessMask, dstAccessMask, buffer, offset, size);
    }

    static MemoryBarrier Image(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image,
                               VkImageSubresourceRange subresourceRange) {
        return MemoryBarrier(srcAccessMask, dstAccessMask, image, subresourceRange);
    }

   private:
    // These constructors are private to force the use of the more explicit static create functions above
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
        : src_access_mask(srcAccessMask), dst_access_mask(dstAccessMask), barrier_type(BarrierType::GLOBAL){};
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
        : src_access_mask(srcAccessMask),
          dst_access_mask(dstAccessMask),
          barrier_type(BarrierType::BUFFER),
          buffer_barrier({buffer, offset, size}) {};
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image, VkImageSubresourceRange subresourceRange)
        : src_access_mask(srcAccessMask),
          dst_access_mask(dstAccessMask),
          barrier_type(BarrierType::IMAGE),
          image_barrier({image, subresourceRange}) {};

   public:
};

struct VkVizPipelineBarrier {
    VkPipelineStageFlags src_stage_mask;
    VkPipelineStageFlags dst_stage_mask;
    VkDependencyFlags dependency_flags;
    std::vector<MemoryBarrier> memory_barriers;

    VkVizPipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                         std::vector<MemoryBarrier> barriers)
        : src_stage_mask(srcStageMask),
          dst_stage_mask(dstStageMask),
          dependency_flags(dependencyFlags),
          memory_barriers(barriers) {}

    json Serialize() const {
        json serialized = {{"source stage mask", src_stage_mask}, {"destination stage mask", dst_stage_mask}, {"dependency flags", dependency_flags}};
        for (const auto& barrier : memory_barriers) {
            serialized["Memory barriers"].push_back(barrier.Serialize());
        }
    }
};

#endif  // MEMORY_BARRIER_H
