#ifndef MEMORY_ACCESS_H
#define MEMORY_ACCESS_H

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <vulkan_core.h>
#include <fstream>

#include "third_party/json.hpp"
#include "command_enums.h"

using json = nlohmann::json;

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

    json to_json() const {
        return {{"Image region description", "Region serialization not yet implemented."}};
    }
    static ImageRegion from_json(const json& j) {
        // Deserialization is unimplemented.
        return ImageRegion(READ, VkImageCopy{});
    }
};

struct ImageAccess {
    VkImage image;
    std::vector<ImageRegion> regions;

    json to_json() const {
        json serialized = {{"image", reinterpret_cast<uintptr_t>(image)}};
        for (const auto& region : regions) {
            serialized["regions"].push_back(region.to_json());
        }
        return serialized;
    }
    static ImageAccess from_json(const json& j) {
        std::vector<ImageRegion> regions;
        for(int i=0; i<j["regions"].size(); ++i) {
            regions.push_back(ImageRegion::from_json(j["regions"][i]));
        }
        return ImageAccess{reinterpret_cast<VkImage>(j["image"].get<uintptr_t>()), std::move(regions)};
    }
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

    json to_json() const {
        return {{"Buffer region description", "Buffer serialization not yet implemented."}};
    }
    static BufferRegion from_json(const json& j) {
        // Deserialization is unimplemented.
        return BufferRegion(READ, VkBufferCopy{});
    }
};

struct BufferAccess {
    VkBuffer buffer;
    std::vector<BufferRegion> regions;

    json to_json() const {
        json serialized = {{"buffer", reinterpret_cast<uintptr_t>(buffer)}};
        for (const auto& region : regions) {
            serialized["regions"].push_back(region.to_json());
        }
        return serialized;
    }
    static BufferAccess from_json(const json& j) {
        std::vector<BufferRegion> regions;
        for(int i=0; i<j["regions"].size(); ++i) {
            regions.push_back(BufferRegion::from_json(j["regions"][i]));
        }
        return BufferAccess{reinterpret_cast<VkBuffer>(j["buffer"].get<uintptr_t>()), std::move(regions)};
    }
};

struct MemoryAccess {
    READ_WRITE read_or_write;
    MEMORY_TYPE type;
    void* data;

    MemoryAccess(const MemoryAccess& other): read_or_write(other.read_or_write), type(other.type) {
        if(type == IMAGE_MEMORY) {
            data = new ImageAccess(*static_cast<ImageAccess*>(other.data));
        } else {
            assert(type == BUFFER_MEMORY);
            data = new BufferAccess(*static_cast<BufferAccess*>(other.data));
        }
    }

    MemoryAccess(MemoryAccess&& other): read_or_write(other.read_or_write), type(other.type), data(other.data) {
        other.data = nullptr;
    }

    // Takes ownership of the passed in data pointer.
    MemoryAccess(READ_WRITE rw, MEMORY_TYPE type, void* access_data): read_or_write(rw), type(type), data(access_data) {}

    template <typename T>
    static MemoryAccess Image(READ_WRITE rw, VkImage image, uint32_t regionCount, const T* pRegions) {
        std::vector<ImageRegion> regions;
        for(uint32_t i=0; i<regionCount; ++i) {
            regions.push_back(ImageRegion(rw, pRegions[i]));
        }
        return MemoryAccess(rw, IMAGE_MEMORY, new ImageAccess({image, std::move(regions)}));
    }

    template <typename T>
    static MemoryAccess Buffer(READ_WRITE rw, VkBuffer buffer, uint32_t regionCount, const T* pRegions) {
        std::vector<BufferRegion> regions;
        for(uint32_t i=0; i<regionCount; ++i) {
            regions.push_back(BufferRegion(rw, pRegions[i]));
        }

        return MemoryAccess(rw, BUFFER_MEMORY, new BufferAccess{buffer, std::move(regions)});
    }

    static MemoryAccess Buffer(READ_WRITE rw, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size) {
        return MemoryAccess(rw, BUFFER_MEMORY, new BufferAccess{buffer, {BufferRegion(offset, size)}});
    }

    template <typename T>
    static MemoryAccess ImageView(READ_WRITE rw, VkVizImageView image_view, uint32_t regionCount, const T* pRegions) {
        // std::vector<ImageRegion> regions = ImageRegion(image_view, regionCount, pRegions);
        assert(0);
        return Image(rw, image_view.Image(), regionCount, pRegions);
    }

    ~MemoryAccess() {
        if (type == IMAGE_MEMORY) {
            delete static_cast<ImageAccess*>(data);
        } else if (type == BUFFER_MEMORY) {
            delete static_cast<BufferAccess*>(data);
        }
    }

    json to_json() const {
        json serialized = {{"type", type}, {"read_or_write", read_or_write}};

        void* handle;
        if (type == IMAGE_MEMORY) {
            ImageAccess access = *(ImageAccess*)data;
            serialized["access"] = access.to_json();
        }
        if (type == BUFFER_MEMORY) {
            BufferAccess access = *(BufferAccess*)data;
            serialized["access"] = access.to_json();
        }

        return serialized;
    }

    static MemoryAccess from_json(const json& j) {
        void* access;
        MEMORY_TYPE access_type = j["type"].get<MEMORY_TYPE>();
        if (access_type == IMAGE_MEMORY) {
            access = new ImageAccess(ImageAccess::from_json(j["access"]));
        } else if (access_type == BUFFER_MEMORY) {
            access = new BufferAccess(BufferAccess::from_json(j["access"]));
        }
        return MemoryAccess(j["read_or_write"].get<READ_WRITE>(), access_type, access);
    }
};

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
