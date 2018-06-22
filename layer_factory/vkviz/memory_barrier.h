#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

#include <vulkan.h>
#include <fstream>
#include <memory>
#include <vector>

#include "device.h"

struct ImageBarrier {
    VkImage image_;
    VkImageSubresourceRange subresource_range_;

    ImageBarrier(VkImage image, VkImageSubresourceRange subresource_range) : image_(image), subresource_range_(subresource_range) {}

    void Log(std::ofstream& out_file) const {
        out_file << "    Is an image barrier" << std::endl;
        out_file << "    Image handle: " << image_ << std::endl;
    }
};

struct BufferBarrier {
    VkBuffer buffer;
    VkDeviceSize offset;
    VkDeviceSize size;

    BufferBarrier(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) : buffer(buffer), offset(offset), size(size) {}

    void Log(std::ofstream& out_file) const {
        out_file << "    Is a buffer barrier" << std::endl;
        out_file << "    Buffer handle: " << buffer << std::endl;
        out_file << "    Starting at offset: " << offset << std::endl;
        out_file << "    Of size: " << size << std::endl;
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

    void Log(std::ofstream& out_file) const {
        out_file << "  Memory barrier" << std::endl;
        out_file << "    Source access mask: " << std::hex << std::showbase << src_access_mask << std::endl;
        out_file << "    Destination access mask: " << std::hex << std::showbase << dst_access_mask << std::endl;

        if (barrier_type == BUFFER) {
            image_barrier.Log(out_file);
        } else if (barrier_type == IMAGE) {
            buffer_barrier.Log(out_file);
        }
    }

   private:
    // These constructors are private to force the use of the more explicit static create functions below
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
        : src_access_mask(srcAccessMask), dst_access_mask(dstAccessMask), barrier_type(BarrierType::GLOBAL){};
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size)
        : src_access_mask(srcAccessMask),
          dst_access_mask(dstAccessMask),
          barrier_type(BarrierType::BUFFER),
          buffer_barrier(buffer, offset, size){};
    MemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image, VkImageSubresourceRange subresourceRange)
        : src_access_mask(srcAccessMask),
          dst_access_mask(dstAccessMask),
          barrier_type(BarrierType::IMAGE),
          image_barrier(image, subresourceRange){};

   public:
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
};

#endif  // MEMORY_BARRIER_H
