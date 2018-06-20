#include <unordered_map>
#include <cassert>
#include <vulkan.h>

struct VkVizMemoryRegion {
    VkDeviceSize offset;
    VkDeviceSize size;
};

class VkVizDevice {
    VkDevice device_;
    std::unordered_map<VkImage, VkVizMemoryRegion> image_bindings_;
    std::unordered_map<VkBuffer, VkVizMemoryRegion> buffer_bindings_;

   public:
    VkVizDevice(VkDevice device) : device_(device) {}

    VkResult BindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        // Could probably cache requirements from an intercepted GetReqs call
        VkMemoryRequirements requirements;
        vkGetImageMemoryRequirements(device_, image, &requirements);
        assert(image_bindings_.find(image) == image_bindings_.end());
        image_bindings_[image] = {memoryOffset, requirements.size};
    }

    VkResult BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
        // Could probably cache requirements from an intercepted GetReqs call
        VkMemoryRequirements requirements;
        vkGetBufferMemoryRequirements(device_, buffer, &requirements);
        assert(buffer_bindings_.find(buffer) == buffer_bindings_.end());
        buffer_bindings_[buffer] = {memoryOffset, requirements.size};
    }

    VkResult MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) {
        printf("Mapped memory at offset: %d, of size: %d\n", offset, size);
    }
};
