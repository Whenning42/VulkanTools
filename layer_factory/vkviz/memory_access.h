#ifndef MEMORY_ACCESS_H
#define MEMORY_ACCESS_H

#include <string>
#include <algorithm>
#include <vulkan.h>

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

// TODO clean this up
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

struct BufferAccess {
    VkBuffer buffer;
    std::vector<BufferRegion> regions;
};

struct MemoryAccess {
    READ_WRITE read_or_write;
    MEMORY_TYPE type;
    void* data;

    template <typename T>
    static MemoryAccess Image(READ_WRITE rw, VkImage image, uint32_t regionCount, const T* pRegions) {
        MemoryAccess access;
        access.type = IMAGE_MEMORY;
        access.read_or_write = rw;

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
        access.read_or_write = rw;

        std::vector<BufferRegion> regions;
        std::transform(pRegions, pRegions + regionCount, std::back_inserter(regions),
                       [rw](const T& region) { return BufferRegion(rw, region); });

        access.data = new BufferAccess({buffer, regions});
        return access;
    }

    static MemoryAccess Buffer(READ_WRITE rw, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
        MemoryAccess access;
        access.type = BUFFER_MEMORY;
        access.read_or_write = rw;
        access.data = new BufferAccess({buffer, {BufferRegion(offset, size)}});
        return access;
    }

    template <typename T>
    static MemoryAccess ImageView(READ_WRITE rw, VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
        // std::vector<ImageRegion> regions = ImageRegion(image_view, regionCount, pRegions);
        assert(0);
        return Image(rw, image_view.Image(), regionCount, pRegions);
    }

    void Log(std::ofstream& out_file) const {
        out_file << "Memory access" << std::endl;
        out_file << "  Is read or write: " << readWriteString(read_or_write) << std::endl;
        out_file << "  Of resource: " << memoryTypeString(type) << std::endl;
        void* handle;
        if (type == IMAGE_MEMORY) {
            ImageAccess access = *(ImageAccess*)data;
            handle = access.image;
        }
        if (type == BUFFER_MEMORY) {
            BufferAccess access = *(BufferAccess*)data;
            handle = access.buffer;
        }
        out_file << "  With handle: " << handle << std::endl;
    }
};

inline void AddAccess(VkCommandBuffer cmdBuffer, MemoryAccess access, CMD_TYPE type) {
    printf("Memory access in cmd buffer: %d.\n", cmdBuffer);
    printf("For command: %s.\n", cmdToString(type).c_str());
    switch (access.read_or_write) {
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
inline MemoryAccess ImageRead(VkImage image, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Image(READ, image, regionCount, pRegions);
}

template <typename T>
inline MemoryAccess ImageWrite(VkImage image, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Image(WRITE, image, regionCount, pRegions);
}

// Generates corresponding ImageRead()
template <typename T>
inline MemoryAccess ImageViewRead(VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
    assert(0);
    // return MemoryAccess::ImageView(READ, image_view, regionCount, pRegions);
}

// Generates corresponding ImageWrite()
template <typename T>
inline MemoryAccess ImageViewWrite(VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::ImageView(WRITE, image_view, regionCount, pRegions);
}

template <typename T>
inline MemoryAccess BufferRead(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(READ, buffer, regionCount, pRegions);
}

inline MemoryAccess BufferRead(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
    return MemoryAccess::Buffer(READ, buffer, offset, size);
}

template <typename T>
inline MemoryAccess BufferWrite(VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
    return MemoryAccess::Buffer(WRITE, buffer, regionCount, pRegions);
}

inline MemoryAccess BufferWrite(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
    return MemoryAccess::Buffer(WRITE, buffer, offset, size);
}

#endif  // MEMORY_ACCESS_H
