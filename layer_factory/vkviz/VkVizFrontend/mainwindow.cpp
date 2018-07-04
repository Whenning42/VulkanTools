#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "command_enums.h"
#include "command_buffer.h"


#include <fstream>
#include <string>
#include "third_party/json.hpp"
#include <cassert>
using json = nlohmann::json;
std::vector<VkVizCommandBuffer> LoadFromFile(std::string filename) {
    std::vector<VkVizCommandBuffer> buffers;
    json j;

    std::ifstream file(filename);
    file >> std::ws;
    while(file.peek() == '{') {
        file >> j;
        buffers.emplace_back(VkVizCommandBuffer::from_json(j));
        file >> std::ws;
    }
    return buffers;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    command_buffer_view_(nullptr) // Can't initialize until ui->setupUi is called.
{
    ui->setupUi(this);

    // Clear any example Command Buffers
    command_buffer_view_ = CommandBufferView(ui->BufferList);
    command_buffer_view_.Clear();

    // Set the splits to be the same size
    ui->Splitter->setSizes({INT_MAX, INT_MAX});

    std::vector<VkVizCommandBuffer> buffers = LoadFromFile("vkviz_frame_start");
    for(const auto& buffer : buffers) {
        command_buffer_view_.AddCommandBuffer(buffer);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
