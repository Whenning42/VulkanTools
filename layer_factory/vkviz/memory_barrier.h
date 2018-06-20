#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

#include <vulkan.h>

#include "device.h"

class MemoryBarrier {
   protected:
    VkAccessFlags src_access_mask_;
    VkAccessFlags dst_access_mask_;

    virtual ~MemoryBarrier() = default;
};

class ImageMemoryBarrier : public MemoryBarrier {
    VkImage image_;
    VkImageSubresourceRange subresource_range_;

   public:
    ImageMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image,
                       VkImageSubresourceRange subresource_range)
        : image_(image), subresource_range_(subresource_range) {
        src_access_mask_ = srcAccessMask;
        dst_access_mask_ = dstAccessMask;
    }
};

class BufferMemoryBarrier : public MemoryBarrier {
    VkBuffer buffer_;
    VkVizMemoryRegion memory_region_;

   public:
    BufferMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkVizMemoryRegion memory_region)
        : buffer_(buffer), memory_region_(memory_region) {
        src_access_mask_ = srcAccessMask;
        dst_access_mask_ = dstAccessMask;
    }
};

class GlobalMemoryBarrier : public MemoryBarrier {
   public:
    GlobalMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        src_access_mask_ = srcAccessMask;
        dst_access_mask_ = dstAccessMask;
    }
};

#endif  // MEMORY_BARRIER_H
