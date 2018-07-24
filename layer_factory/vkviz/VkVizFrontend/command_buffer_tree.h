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

#ifndef COMMAND_BUFFER_TREE_H
#define COMMAND_BUFFER_TREE_H

#include <QLabel>
#include <QTreeWidget>
#include <unordered_set>

#include "color_tree_widget.h"
#include "command_buffer.h"

class CommandBufferTree {
    ColorTree color_tree_;
    QTreeWidget* buffer_tree_;

    // Checks whether this view has any Command Buffers in it.
    bool IsEmpty() { return buffer_tree_->columnCount() == 0; }

    // Adds a new command to the bottom of the last added command buffer.
    void AddCommand(const CommandWrapper& command);

   public:
    CommandBufferTree(QTreeWidget* tree_widget): buffer_tree_(tree_widget), color_tree_(buffer_tree_) {}

    // Adds the given command buffer to this view.
    void AddCommandBuffer(const VkVizCommandBuffer& command_buffer);

    void AddCommandBuffers(const std::vector<VkVizCommandBuffer>& command_buffers);

    // Adds the given command buffer to this view.
    void AddFilteredCommandBuffer(const VkVizCommandBuffer& command_buffer, const std::unordered_set<uint32_t>& relevant_commands);

    void AddFilteredCommandBuffers(const std::vector<VkVizCommandBuffer>& command_buffers, std::unordered_map<VkCommandBuffer, std::unordered_set<uint32_t>>& resource_buffer_filters);

    // Clears all the command buffers from this view.
    void Clear() { buffer_tree_->clear(); }
};

#endif  // COMMAND_BUFFER_TREE_H
