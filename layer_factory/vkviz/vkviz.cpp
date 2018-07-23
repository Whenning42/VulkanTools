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

#include "vkviz.h"
#include "serialize.h"
#include "synchronization.h"

#include <algorithm>
#include <sstream>

VkResult VkViz::PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    GetCommandBuffer(commandBuffer).Begin();
}

VkResult VkViz::PostCallEndCommandBuffer(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).End(); }

VkResult VkViz::PostCallResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    GetCommandBuffer(commandBuffer).Reset();
}

VkResult VkViz::PostCallAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
                                               VkCommandBuffer* pCommandBuffers) {
    AddCommandBuffers(pAllocateInfo, pCommandBuffers, &device_map_.at(device));
}

void VkViz::PostCallFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers) {
    RemoveCommandBuffers(commandBufferCount, pCommandBuffers);
}
VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    if(current_frame_ % 20 == 0) {
        for (uint32_t i = 0; i < submitCount; ++i) {
            for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; ++j) {
                capturer_.AddCommandBuffer(GetCommandBuffer(pSubmits[i].pCommandBuffers[i]));
            }
        }
    }
}

VkResult VkViz::PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
    capturer_.End();

    current_frame_++;

    capturer_.Begin("vkviz_frame" + std::to_string(current_frame_));
}

// An instance needs to be declared to turn on a layer in the layer_factory framework
VkViz vkviz_layer;
