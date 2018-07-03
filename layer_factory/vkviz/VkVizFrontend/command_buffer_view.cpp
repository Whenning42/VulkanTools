#include "command_buffer_view.h"

void CommandBufferView::NestedDelete(QLayoutItem* to_delete) {
    if(to_delete->layout()) {
        QLayoutItem* item;
        while((item = to_delete->layout()->takeAt(0)) != 0) {
            NestedDelete(item);
        }
        delete to_delete;
    }

    delete to_delete->widget();
}

void CommandBufferView::AddBuffer(std::string buffer) {
    QLayoutItem* bottom_spacer_ = buffer_list_->takeAt(buffer_list_->count() - 1);

    // We might want to do this regardless of current_indent_ being nullptr.
    if (current_indent_) {
        buffer_list_->addSpacing(10);
    }
    current_indent_ = nullptr;
    command_list_ = nullptr;

    buffer_list_->addWidget(new QLabel(QString::fromStdString(buffer)), 1);
    buffer_list_->addItem(bottom_spacer_);
}

void CommandBufferView::AddCommand(std::string command) {
    if(!current_indent_) {
        current_indent_ = new QHBoxLayout();
        current_indent_->addSpacing(20);

        QLayoutItem* bottom_spacer_ = buffer_list_->takeAt(buffer_list_->count() - 1);
        buffer_list_->addLayout(current_indent_, 1);
        buffer_list_->addItem(bottom_spacer_);

        command_list_ = new QVBoxLayout();
        current_indent_->addLayout(command_list_, 1);
    }

    command_list_->addWidget(new QLabel(QString::fromStdString(command)), 1);
}

void CommandBufferView::AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
    AddBuffer(command_buffer.Name());
    for(const auto& command : command_buffer.Commands()) {
        AddCommand(command.Name());
    }
}

