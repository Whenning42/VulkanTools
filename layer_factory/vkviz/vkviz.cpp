#include "vkviz.h"
#include <algorithm>


VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {
    // Write dot file with cmd buffers for each submit
    uint32_t node_num = 0;
    std::string filename = outfile_base_name_ + std::to_string(outfile_num_) + ".dot";
    outfile_.open(filename);
    //printf("Opened: %s for writing.\n", filename.c_str());
    // Write graphviz of cmd buffers
    outfile_ << "digraph G {\n  node [shape=record];\n";
    for (uint32_t i = 0; i < submitCount; ++i) {
        for (uint32_t j = 0; j < pSubmits[i].commandBufferCount; ++j) {
            outfile_ << "  node" << node_num << "[ label = \"{<n> COMMAND BUFFER 0x" << pSubmits[i].pCommandBuffers[j] << " ";
            for (const auto& cmd : cmdbuffer_map_[pSubmits[i].pCommandBuffers[j]]) {
                outfile_ << "| " << cmdToString(cmd);
            }
            outfile_ << "];\n";
        }
    }
    outfile_ << "}";
    outfile_.close();
}

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

void VkViz::PostCallFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uin32_t commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers) {
    RemoveCommandBuffers(commandBufferCount, pCommandBuffers);
}

VkResult VkViz::PostCallCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) {
    AddRenderPass(pRenderPass, pCreateInfo);
}

void VkViz::PostCallDestroyRenderPass(VkDevice deice, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) {
    RemoveRenderPass(renderPass);
}

VkResult VkViz::PostCallQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) {}

// An instance needs to be declared to turn on a layer in the layer_factory framework
VkViz vkviz_layer;
