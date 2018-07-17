#include "command_buffer_tree.h"
#include "command_viz.h"

void CommandBufferTree::AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
    CommandBufferViz viz(command_buffer.Handle(), command_buffer.Commands());
    buffer_tree_->addTopLevelItem(viz.ToWidget());
}
