#include "vkviz.h"
#include "serialize.h"

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
    for (uint32_t i = 0; i < submitCount; ++i) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; ++j) {
            /* Test code to check serialization/deserialization. Missing expected fields will fail asserts. */
            std::stringstream stream;
            json js;
            stream << json(GetCommandBuffer(pSubmits[i].pCommandBuffers[i]));
            std::string debug = stream.str();
            stream >> js;
            VkVizCommandBuffer buf = js.get<VkVizCommandBuffer>();
            out_file_ << js.dump(2) << std::endl << std::endl;
        }
    }
}

VkResult VkViz::PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
    out_file_.close();
    out_file_.open("vkviz_frame" + std::to_string(current_frame_));
    current_frame_++;
}

// An instance needs to be declared to turn on a layer in the layer_factory framework
VkViz vkviz_layer;
