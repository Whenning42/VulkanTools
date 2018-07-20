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
SERIALIZE(ImageDescriptor, image_view);

struct BufferDescriptor {
    VkBuffer buffer;
};
SERIALIZE(BufferDescriptor, buffer);

struct TexelDescriptor {
    VkBufferView buffer_view;
};
SERIALIZE(TexelDescriptor, buffer_view);

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

    VkImageView ImageView() const {
        assert(descriptor_type == IMAGE_DESCRIPTOR);
        return image_descriptor.image_view;
    }

    VkBuffer Buffer() const {
        assert(descriptor_type == BUFFER_DESCRIPTOR);
        return buffer_descriptor.buffer;
    }

    VkBufferView BufferView() const {
        assert(descriptor_type == TEXEL_DESCRIPTOR);
        return texel_descriptor.buffer_view;
    }
};
inline void to_json(json& j, const VkVizDescriptor& desc) {
    switch(desc.descriptor_type) {
        case IMAGE_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}, {"image_descriptor", desc.image_descriptor}};
            break;
        case BUFFER_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}, {"buffer_descriptor", desc.buffer_descriptor}};
            break;
        case TEXEL_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}, {"texel_descriptor", desc.texel_descriptor}};
            break;
        case UNINITIALIZED_DESCRIPTOR:
            j = {{"descriptor_type", desc.descriptor_type}};
            break;
    }
}
inline void from_json(const json& j, VkVizDescriptor& desc) {
    desc.descriptor_type = j["descriptor_type"].get<DescriptorType>();
    switch(j["descriptor_type"].get<DescriptorType>()) {
        case IMAGE_DESCRIPTOR:
            desc.image_descriptor = j["image_descriptor"].get<ImageDescriptor>();
            break;
        case BUFFER_DESCRIPTOR:
            desc.buffer_descriptor =j["buffer_descriptor"].get<BufferDescriptor>();
            break;
        case TEXEL_DESCRIPTOR:
            desc.texel_descriptor = j["texel_descriptor"].get<TexelDescriptor>();
            break;
    }
}

typedef std::vector<VkVizDescriptor> Binding;
typedef std::vector<Binding> Set;

// A helper class for stepping through descriptor sets' bindings and elements.
struct DescriptorIterator {
    Set& set;
    uint32_t binding_index;
    uint32_t element_index;

private:
    uint32_t binding_count() const {
        return set.size();
    }

    uint32_t element_count() const {
        return set[binding_index].size();
    }

    bool InBounds() const {
        return binding_index >= 0 && binding_index < binding_count() && element_index >= 0 && element_index < element_count();
    }

    bool IsEnd() const {
        return binding_index == set.size() && element_index == 0;
    }
public:

    DescriptorIterator(uint32_t binding_index, uint32_t element_index, Set& set): binding_index(binding_index), element_index(element_index), set(set) {
        assert(InBounds());
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
        while(element_index >= element_count() && binding_index < binding_count()) {
            element_index = 0;
            binding_index++;
        }

        assert(InBounds() || IsEnd());
        return *this;
    }

    DescriptorIterator operator++(int) {
        DescriptorIterator before = *this;
        ++*this;
        return before;
    }

    VkVizDescriptor &operator*() { assert(InBounds()); return set[binding_index][element_index]; }
    VkVizDescriptor const &operator*() const { assert(InBounds()); return *(*this); }
};

struct VkVizDescriptorSetLayoutCreateInfo {
    // Note that each binding has pImmutableSamplers set to nullptr since they are not yet tracked.
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    VkVizDescriptorSetLayoutCreateInfo(const VkDescriptorSetLayoutCreateInfo& create_info) {
        layout_bindings.resize(create_info.bindingCount);
        std::copy(create_info.pBindings, create_info.pBindings + create_info.bindingCount, layout_bindings.begin());
        for(auto& layout_binding : layout_bindings) {
            layout_binding.pImmutableSamplers = nullptr;
        }
    }
};

// This implementation doesn't handle large binding indices well, because it allocates a vector for each binding number up to the
// highest binding. This could waste a lot of memory for sparse bindings with very large indices.
class VkVizDescriptorSet {
public:
    VkDescriptorSet vk_set_;
    Set set_;

    DescriptorIterator GetIt(uint32_t binding, uint32_t element) {return DescriptorIterator(binding, element, set_);}

    VkVizDescriptorSet() = default;
    VkVizDescriptorSet(const VkVizDescriptorSetLayoutCreateInfo& create_info, VkDescriptorSet handle): vk_set_(handle) {
        for(uint32_t i=0; i < create_info.layout_bindings.size(); ++i) {
            const VkDescriptorSetLayoutBinding& layout_binding = create_info.layout_bindings[i];
            if(layout_binding.binding >= set_.size()) {
                set_.resize(layout_binding.binding + 1);
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
SERIALIZE2(VkVizDescriptorSet, vk_set_, set_);

#endif  // DESCRIPTOR_SET_H
