#ifndef MEMORY_BARRIER_H
#define MEMORY_BARRIER_H

#include <vulkan.h>
#include <fstream>
#include <memory>

#include "device.h"

class MemoryBarrierImpl {
   protected:
    VkAccessFlags src_access_mask_;
    VkAccessFlags dst_access_mask_;

   public:
    virtual ~MemoryBarrierImpl() = default;
    virtual void Log(std::ofstream& out_file) const {
        out_file << "  Memory barrier" << std::endl;
        out_file << "    Source access mask: " << std::hex << std::showbase << src_access_mask_ << std::endl;
        out_file << "    Destination access mask: " << std::hex << std::showbase << dst_access_mask_ << std::endl;
    }
};

class ImageMemoryBarrierImpl : public MemoryBarrierImpl {
    VkImage image_;
    VkImageSubresourceRange subresource_range_;

   public:
    ImageMemoryBarrierImpl(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image,
                           VkImageSubresourceRange subresource_range)
        : image_(image), subresource_range_(subresource_range) {
        src_access_mask_ = srcAccessMask;
        dst_access_mask_ = dstAccessMask;
    }

    void Log(std::ofstream& out_file) const override {
        MemoryBarrierImpl::Log(out_file);
        out_file << "    Is an image barrier" << std::endl;
        out_file << "    Image handle: " << image_ << std::endl;
    }
};

class BufferMemoryBarrierImpl : public MemoryBarrierImpl {
    VkBuffer buffer_;
    VkVizMemoryRegion memory_region_;

   public:
    BufferMemoryBarrierImpl(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer, VkDeviceSize offset,
                            VkDeviceSize size)
        : BufferMemoryBarrierImpl(srcAccessMask, dstAccessMask, buffer, {offset, size}) {}

    BufferMemoryBarrierImpl(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer,
                            VkVizMemoryRegion memory_region)
        : buffer_(buffer), memory_region_(memory_region) {
        src_access_mask_ = srcAccessMask;
        dst_access_mask_ = dstAccessMask;
    }

    void Log(std::ofstream& out_file) const override {
        MemoryBarrierImpl::Log(out_file);
        out_file << "    Is a buffer barrier" << std::endl;
        out_file << "    Buffer handle: " << buffer_ << std::endl;
    }
};

class GlobalMemoryBarrierImpl : public MemoryBarrierImpl {
   public:
    GlobalMemoryBarrierImpl(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        src_access_mask_ = srcAccessMask;
        dst_access_mask_ = dstAccessMask;
    }

    void Log(std::ofstream& out_file) const override {
        MemoryBarrierImpl::Log(out_file);
        out_file << "    Is a global barrier" << std::endl;
    }
};

class MemoryBarrier {
    std::unique_ptr<MemoryBarrierImpl> barrier_;

   public:
    MemoryBarrier(std::unique_ptr<MemoryBarrierImpl> barrier) : barrier_(std::move(barrier)) {}

    static MemoryBarrier GlobalBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask) {
        return MemoryBarrier(std::make_unique<GlobalMemoryBarrierImpl>(srcAccessMask, dstAccessMask));
    }

    static MemoryBarrier BufferBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkBuffer buffer,
                                       VkDeviceSize offset, VkDeviceSize size) {
        return MemoryBarrier(std::make_unique<BufferMemoryBarrierImpl>(srcAccessMask, dstAccessMask, buffer, offset, size));
    }

    static MemoryBarrier ImageBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImage image,
                                      VkImageSubresourceRange subresource_range) {
        return MemoryBarrier(std::make_unique<ImageMemoryBarrierImpl>(srcAccessMask, dstAccessMask, image, subresource_range));
    }

    void Log(std::ofstream& out_file) const {
        printf("Logging pipeline\n");
        barrier_->Log(out_file);
    }
};

#endif  // MEMORY_BARRIER_H
