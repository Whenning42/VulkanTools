#ifndef COMMAND_BUFFER_VIEW_H
#define COMMAND_BUFFER_VIEW_H

#include <QLabel>
#include <QLayout>

#include "command_buffer.h"

class CommandBufferView {
    QVBoxLayout* buffer_list_;
    QHBoxLayout* current_indent_ = nullptr;
    QVBoxLayout* command_list_ = nullptr;
    bool is_collpased_ = false;

    // Checks whether this view has any Command Buffers in it.
    bool IsEmpty() {
        return buffer_list_->count() <= 1;
    }

    // Deletes all the layouts and widgets in a given layout.
    void NestedDelete(QLayoutItem* to_delete);

    // Adds a new command buffer at the bottom of this view.
    void AddBuffer(std::string buffer);

    // Adds a new command to the bottom of the last added command buffer.
    void AddCommand(std::string command);

public:
    CommandBufferView(QVBoxLayout* buffer_list_layout) : buffer_list_(buffer_list_layout) {}

    // Adds the given command buffer to this view.
    void AddCommandBuffer(const VkVizCommandBuffer& command_buffer);

    // Clears all the command buffers from this view.
    void Clear();
};

#endif  // COMMAND_BUFFER_VIEW_H
