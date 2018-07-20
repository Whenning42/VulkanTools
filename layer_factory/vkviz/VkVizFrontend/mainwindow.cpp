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
        buffers.emplace_back(j.get<VkVizCommandBuffer>());
        file >> std::ws;
    }
    return buffers;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      command_buffer_tree_(nullptr)  // Can't initialize until ui->setupUi is called.
{
    ui->setupUi(this);

    // Clear any example Command Buffers
    command_buffer_tree_ = CommandBufferTree(ui->CmdBufferTree);
    // command_buffer_view_.Clear();

    // Set the splits to be the same size
    ui->Splitter->setSizes({INT_MAX, INT_MAX});

    std::vector<VkVizCommandBuffer> buffers = LoadFromFile("vkviz_frame_start");
    for(const auto& buffer : buffers) {
        command_buffer_tree_.AddCommandBuffer(buffer);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
