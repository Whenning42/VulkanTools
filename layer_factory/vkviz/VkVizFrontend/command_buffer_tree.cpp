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

#include "command_buffer_tree.h"
#include "command_viz.h"

void CommandBufferTree::AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
    CommandBufferViz command_buffer_viz(command_buffer.Handle(), command_buffer.Commands());
    buffer_tree_->addTopLevelItem(command_buffer_viz.ToWidget(color_tree_));
}

void CommandBufferTree::AddCommandBuffers(const std::vector<VkVizCommandBuffer>& command_buffers) {
    for(const auto& buffer : command_buffers) {
        AddCommandBuffer(buffer);
    }
}

void CommandBufferTree::AddFilteredCommandBuffer(const VkVizCommandBuffer& command_buffer, const std::unordered_set<uint32_t>& relevant_commands) {
    CommandBufferViz command_buffer_viz(command_buffer.Handle(), command_buffer.Commands());
    QTreeWidgetItem* buffer_widget = command_buffer_viz.ToFilteredWidget(relevant_commands, color_tree_);
    if(buffer_widget) {
        buffer_tree_->addTopLevelItem(buffer_widget);
    }
}

void CommandBufferTree::AddFilteredCommandBuffers(const std::vector<VkVizCommandBuffer>& command_buffers, std::unordered_map<VkCommandBuffer, std::unordered_set<uint32_t>>& buffer_filters) {
    for(const auto& buffer : command_buffers) {
        if(buffer_filters.find(buffer.Handle()) != buffer_filters.end()) {
            AddFilteredCommandBuffer(buffer, buffer_filters.at(buffer.Handle()));
        } else {
            AddFilteredCommandBuffer(buffer, {});
        }
    }
}
