#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

#include <vulkan_core.h>
#include <fstream>
#include <memory>
#include <cassert>
#include <vector>

#include "device.h"

#include "third_party/json.hpp"

using json = nlohmann::json;

struct ImageBarrier {
    VkImage image;
    VkImageSubresourceRange subresource_range;

    json to_json() const {
        return {{"image", reinterpret_cast<uintptr_t>(image)}, {"subresource_range", "Not yet serialized."}};
    }
    static ImageBarrier from_json(const json& j) {
        return {reinterpret_cast<VkImage>(j["image"].get<uintptr_t>()), {}};
    }
};

struct BufferBarrier {
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;

    json to_json() const {
        return {{"buffer", reinterpret_cast<uintptr_t>(buffer)}, {"offset", offset}, {"size", size}};
    }
    static BufferBarrier from_json(const json& j) {
        return {reinterpret_cast<VkBuffer>(j["buffer"].get<uintptr_t>()), j["offset"].get<VkDeviceSize>(), j["size"].get<VkDeviceSize>()};
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

    json to_json() const {
        json serialized = {{"src_access_mask", src_access_mask}, {"dst_access_mask", dst_access_mask}, {"barrier_type", barrier_type}};

        if (barrier_type == BUFFER) {
            serialized["buffer_barrier"] = buffer_barrier.to_json();
        } else if (barrier_type == IMAGE) {
            serialized["image_barrier"] = image_barrier.to_json();
        }
        return serialized;
    }
    static MemoryBarrier from_json(const json& j) {
        VkAccessFlags src_access_mask = j["src_access_mask"].get<VkAccessFlags>();
        VkAccessFlags dst_access_mask = j["dst_access_mask"].get<VkAccessFlags>();
        BarrierType barrier_type = j["barrier_type"].get<BarrierType>();

        if(barrier_type == BUFFER) {
            return MemoryBarrier(src_access_mask, dst_access_mask, BufferBarrier::from_json(j["buffer_barrier"]));
        } else if(barrier_type == IMAGE) {
            return MemoryBarrier(src_access_mask, dst_access_mask, ImageBarrier::from_json(j["image_barrier"]));
        } else {
            assert(barrier_type == GLOBAL);
            return MemoryBarrier(src_access_mask, dst_access_mask);
        }
    }

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

    json to_json() const {
        json serialized = {{"src_stage_mask", src_stage_mask}, {"dst_stage_mask", dst_stage_mask}, {"dependency_flags", dependency_flags}};
        for (const auto& barrier : memory_barriers) {
            serialized["memory_barriers"].push_back(barrier.to_json());
        }
        return serialized;
    }

    static VkVizPipelineBarrier from_json(const json& j) {
        std::vector<MemoryBarrier> memory_barriers;
        for(int i=0; i<j["memory_barriers"].size(); ++i) {
            memory_barriers.push_back(MemoryBarrier::from_json(j["memory_barriers"][i]));
        }
        return VkVizPipelineBarrier(j["src_stage_mask"].get<VkPipelineStageFlags>(), j["dst_stage_mask"].get<VkPipelineStageFlags>(), j["dependency_flags"].get<VkDependencyFlags>(), std::move(memory_barriers));
    }
};

#endif  // MEMORY_BARRIER_H
