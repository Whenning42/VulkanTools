#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "command_enums.h"
#include "command_buffer.h"

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

void MainWindow::AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
    display_list_.AddBuffer(std::to_string((uint64_t)command_buffer.Handle()));
    for(const auto& command : command_buffer.Commands()) {
        display_list_.AddCommand(command.TypeString());
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    display_list_(ui->BufferList)
{
    ui->setupUi(this);
    display_list_ = CommandBufferList(ui->BufferList);
    ClearBufferList();

    /*
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
    }*/
}

MainWindow::~MainWindow()
{
    delete ui;
}
