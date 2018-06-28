#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <string>

//#include "command_enums.h"
//#include "command_buffer.h"

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

void NestedDelete(QLayoutItem* to_delete) {
    if(to_delete->layout()) {
        QLayoutItem* item;
        while((item = to_delete->layout()->takeAt(0)) != 0) {
            NestedDelete(item);
        }
        delete to_delete;
    }

    delete to_delete->widget();
}

// Clears all the cmd buffer while leaving the bottom spacer
void MainWindow::ClearBufferList() {
    while(ui->BufferList->count() > 1) {
        NestedDelete(ui->BufferList->takeAt(0));
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ClearBufferList();

    CommandBufferList cbl(ui->BufferList);
    cbl.AddBuffer("Buffer: 0x1");
    cbl.AddCommand("Begin");
    cbl.AddCommand("Draw");
    cbl.AddCommand("End");

    cbl.AddBuffer("Buffer: 0x2");
    cbl.AddCommand("Begin");
    cbl.AddCommand("Clear");

    for(int i=0; i<12; ++i) {
        cbl.AddBuffer("Buffer: 0x1");
        cbl.AddCommand("Begin");
        cbl.AddCommand("Draw");
        cbl.AddCommand("End");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
