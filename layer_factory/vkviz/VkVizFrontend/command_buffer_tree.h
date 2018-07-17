#ifndef COMMAND_BUFFER_TREE_H
#define COMMAND_BUFFER_TREE_H

#include <QLabel>
#include <QTreeWidget>

#include "command_buffer.h"

class CommandBufferTree {
    QTreeWidget* buffer_tree_;

    // Checks whether this view has any Command Buffers in it.
    bool IsEmpty() { return buffer_tree_->columnCount() == 0; }

    // Adds a new command to the bottom of the last added command buffer.
    void AddCommand(const CommandWrapper& command);

   public:
    CommandBufferTree(QTreeWidget* buffer_tree) : buffer_tree_(buffer_tree) {}

    // Adds the given command buffer to this view.
    void AddCommandBuffer(const VkVizCommandBuffer& command_buffer);

    // Clears all the command buffers from this view.
    void Clear() { buffer_tree_->clear(); }
};

#endif  // COMMAND_BUFFER_TREE_H
