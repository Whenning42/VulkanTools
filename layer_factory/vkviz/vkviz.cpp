#include "vkviz.h"
#include <algorithm>

VkResult VkViz::PostCallBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
    GetCommandBuffer(commandBuffer).Begin();
}

VkResult VkViz::PostCallEndCommandBuffer(VkCommandBuffer commandBuffer) { GetCommandBuffer(commandBuffer).End(); }

VkResult VkViz::PostCallResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    GetCommandBuffer(commandBuffer).Reset();
}

VkResult VkViz::PostCallAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
                                               VkCommandBuffer* pCommandBuffers) {
    AddCommandBuffers(pAllocateInfo, pCommandBuffers);
}

void VkViz::PostCallFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers) {
    RemoveCommandBuffers(commandBufferCount, pCommandBuffers);
}

VkResult VkViz::PostCallCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    AddRenderPass(device, pCreateInfo, pRenderPass);
}

void VkViz::PostCallDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    RemoveRenderPass(device, renderPass);
}

#include "third_party/json.hpp"
#include <sstream>
using json = nlohmann::json;
VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    for (uint32_t i = 0; i < submitCount; ++i) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; ++j) {
            /* test code to check serialization/deserialization */
            std::stringstream stream;
            json js;
            stream << GetCommandBuffer(pSubmits[i].pCommandBuffers[i]).to_json();
            std::string debug = stream.str();
            stream >> js;
            VkVizCommandBuffer buf = VkVizCommandBuffer::from_json(js);
            out_file_ << js.dump(2) << std::endl << std::endl;

            //out_file_ << GetCommandBuffer(pSubmits[i].pCommandBuffers[i]).to_json().dump(2) << std::endl;
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
