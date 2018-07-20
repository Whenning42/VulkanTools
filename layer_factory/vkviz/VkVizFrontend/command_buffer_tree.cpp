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
    CommandBufferViz viz(command_buffer.Handle(), command_buffer.Commands());
    buffer_tree_->addTopLevelItem(viz.ToWidget());
}
