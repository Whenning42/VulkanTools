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

VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    // Write cmd buffers to file
    std::string filename = "vkvizout.frame" + std::to_string(current_frame_);
    printf("File to write to: %s\n", filename.c_str());
    std::ofstream outfile(filename, std::ios_base::app);
    for (uint32_t i = 0; i < submitCount; ++i) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; ++j) {
            outfile << "Submitted command buffer: " << pSubmits[i].pCommandBuffers[j] << std::endl;
            for (const auto& command : command_buffer_map_[pSubmits[i].pCommandBuffers[j]].Commands()) {
                outfile << cmdToString(command.Type()).c_str() << std::endl;
            }
            outfile << std::endl;
        }
    }
    outfile.close();
}

VkResult VkViz::PostCallQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
    printf("Frame++\n");
    current_frame_++;
}

// An instance needs to be declared to turn on a layer in the layer_factory framework
VkViz vkviz_layer;
