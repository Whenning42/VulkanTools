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

#include <QColor>
#include <QLabel>
#include <QTreeWidget>
#include <QVariant>
#include <QObject>
#include <unordered_set>

#include "color_tree_widget.h"
#include "command_tree_widget_item.h"
#include "command_buffer.h"
#include "command_viz.h"
#include "synchronization.h"

class CommandBufferTree {
    CommandDrawer drawer_;

    void OnExpand(QTreeWidgetItem* item) {
        CommandTreeWidgetItem* tree_item = CommandTreeWidgetItem::FromQItem(item);
        tree_item->Expand();
    }

    void OnCollapse(QTreeWidgetItem* item) {
        CommandTreeWidgetItem* tree_item = CommandTreeWidgetItem::FromQItem(item);
        tree_item->Collapse();
    }

    void OnActivated(QTreeWidgetItem* item) {
        CommandTreeWidgetItem* tree_item = CommandTreeWidgetItem::FromQItem(item);
        if(tree_item->HasBarrierOccurance()) {
            drawer_.ShowBarrierEffects(tree_item->GetBarrierOccurance());
        }
    }

   public:
    CommandBufferTree() {}
    CommandBufferTree(QTreeWidget* tree_widget, const FrameCapture& capture, FocusMode focus_mode)
        : drawer_(capture, tree_widget, focus_mode) {}

    static CommandBufferTree Create(QTreeWidget* tree_widget, const FrameCapture& capture, FocusMode focus_mode) {
        CommandBufferTree out(tree_widget, capture, focus_mode);
        out.SetupCallbacks();
        return out;
    }

    void SetupCallbacks() {
        QObject::connect(drawer_.tree_, &QTreeWidget::itemCollapsed, [this](QTreeWidgetItem* item) { OnCollapse(item); });
        QObject::connect(drawer_.tree_, &QTreeWidget::itemExpanded, [this](QTreeWidgetItem* item) { OnExpand(item); });
        QObject::connect(drawer_.tree_, &QTreeWidget::itemActivated, [this](QTreeWidgetItem* item) { OnActivated(item); });
    }

    CommandBufferTree& operator=(const CommandBufferTree& other) = delete;
    CommandBufferTree& operator=(CommandBufferTree&& other) = default;
    CommandBufferTree(CommandBufferTree&& other) = default;

    void SetResource(void* resource) { drawer_.FocusResource(resource); }
};

#endif  // COMMAND_BUFFER_TREE_H
