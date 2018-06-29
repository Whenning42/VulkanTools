#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QVariant>
#include <QMainWindow>
#include <QLabel>
#include <QLayout>
#include <string>

#include "command_buffer.h"

class CommandBufferList { 
    QVBoxLayout* buffer_list_;
    QHBoxLayout* current_indent_ = nullptr;
    QVBoxLayout* command_list_ = nullptr;

public:
    CommandBufferList(QVBoxLayout* buffer_list_layout) : buffer_list_(buffer_list_layout) {}

    void AddBuffer(std::string buffer) {
        QLayoutItem* bottom_spacer_ = buffer_list_->takeAt(buffer_list_->count() - 1);

        if (current_indent_) {
            buffer_list_->addSpacing(10);
        }
        current_indent_ = nullptr;
        command_list_ = nullptr;

        buffer_list_->addWidget(new QLabel(QString::fromStdString(buffer)), 1);
        buffer_list_->addItem(bottom_spacer_);
    }

    void AddCommand(std::string command) {
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
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void AddCommandBuffer(const VkVizCommandBuffer& buffer);
    void ClearBufferList();
private:
    Ui::MainWindow *ui;
    CommandBufferList display_list_;
};

#endif // MAINWINDOW_H
