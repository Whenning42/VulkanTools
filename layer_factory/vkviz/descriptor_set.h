#ifndef DESCRIPTOR_SET_H
#define DESCRIPTOR_SET_H

#include "serialize.h"

#include <cassert>
#include <vector>
#include <vulkan_core.h>

enum DescriptorType {UNINITIALIZED_DESCRIPTOR, IMAGE_DESCRIPTOR, BUFFER_DESCRIPTOR, TEXEL_DESCRIPTOR};


struct ImageDescriptor {
    VkImageView image_view;
};
SERIALIZE(ImageDescriptor, VkImageView, image_view);

struct BufferDescriptor {
    VkBuffer buffer;
};
SERIALIZE(BufferDescriptor, VkBuffer, buffer);

struct TexelDescriptor {
    VkBufferView buffer_view;
};
SERIALIZE(TexelDescriptor, VkBufferView, buffer_view);

struct VkVizDescriptor{

    DescriptorType descriptor_type;
    union {
        ImageDescriptor image_descriptor;
        BufferDescriptor buffer_descriptor;
        TexelDescriptor texel_descriptor;
    };
 public:

    VkVizDescriptor(): descriptor_type(UNINITIALIZED_DESCRIPTOR) {};
    VkVizDescriptor(VkImageView image_view): descriptor_type(IMAGE_DESCRIPTOR), image_descriptor({image_view}) {};
    VkVizDescriptor(VkBuffer buffer): descriptor_type(BUFFER_DESCRIPTOR), buffer_descriptor({buffer}) {};
    VkVizDescriptor(VkBufferView buffer_view): descriptor_type(TEXEL_DESCRIPTOR), texel_descriptor({buffer_view}) {};
};
inline void to_json(json& j, const VkVizDescriptor& desc) {
    assert(desc.descriptor_type != UNINITIALIZED_DESCRIPTOR);

    switch(desc.descriptor_type) {
        case IMAGE_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}, {"image_descriptor", desc.image_descriptor}};
        case BUFFER_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}, {"buffer_descriptor", desc.buffer_descriptor}};
        case TEXEL_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}, {"texel_descriptor", desc.texel_descriptor}};
    }
}
inline void from_json(const json& j, VkVizDescriptor& desc) {
    switch(j["descriptor_type"].get<DescriptorType>()) {
        case IMAGE_DESCRIPTOR:
            desc = VkVizDescriptor(j["image_descriptor"].get<ImageDescriptor>().image_view);
        case BUFFER_DESCRIPTOR:
            desc = VkVizDescriptor(j["buffer_descriptor"].get<BufferDescriptor>().buffer);
        case TEXEL_DESCRIPTOR:
            desc = VkVizDescriptor(j["texel_descriptor"].get<TexelDescriptor>().buffer_view);
    }
}

typedef std::vector<VkVizDescriptor> Binding;
typedef std::vector<Binding> Set;

struct DescriptorIterator {
    Set& set;
    uint32_t set_index;
    uint32_t binding_index;
    uint32_t element_index;

    void AssertIndexInBounds() {
        assert(binding_index < set.size());
        assert(element_index < set[binding_index].size());
    }

    DescriptorIterator(uint32_t binding_index, uint32_t element_index, Set& set): binding_index(binding_index), element_index(element_index), set(set) {
        AssertIndexInBounds();
    }

    // This will show an iterator from different descriptor sets at the same spot as being equal.
    bool operator==(const DescriptorIterator& other) {
        return binding_index == other.binding_index &&
               element_index == other.element_index;
    }
    bool operator!=(const DescriptorIterator& other) {
        return !(*this == other);
    }

    DescriptorIterator operator++() {
        element_index++;
        while(element_index >= set[binding_index].size()) {
            element_index = 0;
            binding_index++;
        }

        AssertIndexInBounds();
        return *this;
    }

    DescriptorIterator operator++(int) {
        DescriptorIterator before = *this;
        ++*this;
        return before;
    }

    VkVizDescriptor &operator*() { return set[binding_index][element_index]; }
    VkVizDescriptor const &operator*() const { return *(*this); }
};

// This implementation doesn't handle large binding indices well.
class VkVizDescriptorSet {
public:
    VkDescriptorSet vk_set_;
    Set set_;

    DescriptorIterator GetIt(uint32_t binding, uint32_t element) {return DescriptorIterator(binding, element, set_);}

    VkVizDescriptorSet() = default;
    VkVizDescriptorSet(const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSet handle): vk_set_(handle) {
        for(uint32_t i=0; i<pCreateInfo->bindingCount; ++i) {
            const VkDescriptorSetLayoutBinding& layout_binding = pCreateInfo->pBindings[i];
            if(layout_binding.binding > set_.size()) {
                set_.resize(layout_binding.binding);
            }
            set_[layout_binding.binding].resize(layout_binding.descriptorCount);

            // If we add sampler tracking, then we need to populate our bindings with pImmutableSamplers when non-null.
        }
    }

    static bool IsImage(VkDescriptorType type) {
        return type == VK_DESCRIPTOR_TYPE_SAMPLER ||
               type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
               type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
               type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
               type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    }

    static bool IsBufferView(VkDescriptorType type) {
        return type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER ||
               type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    }

    static bool IsBuffer(VkDescriptorType type) {
        return type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
               type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
               type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
               type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    }
};
SERIALIZE2(VkVizDescriptorSet, VkDescriptorSet, vk_set_, Set, set_);

#endif  // DESCRIPTOR_SET_H
