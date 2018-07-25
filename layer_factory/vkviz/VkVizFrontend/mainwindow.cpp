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
#include "frame_capture.h"
#include "string_helpers.h"
#include "synchronization.h"

#include <fstream>
#include <string>
#include <cassert>
#include <QCommonStyle>
#include <QStackedLayout>

void MainWindow::ShowSubmittedCommandBufferView() {
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::PopulateSubmittedCommandBufferView() {
    submitted_command_buffer_view_.Clear();
    submitted_command_buffer_view_.AddCommandBuffers(capture_.command_buffers);
}

void MainWindow::ShowResourceView() {
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::PopulateResourceView(void* resource) {
    std::uintptr_t resource_val = reinterpret_cast<uintptr_t>(resource);
    printf("Populating dropdown: %p\n", resource);

    resource_view_.Clear();
    std::unordered_map<VkCommandBuffer, std::unordered_set<uint32_t>> buffer_filters = capture_.sync.resource_references[resource_val];
    resource_view_.AddFilteredCommandBuffers(capture_.command_buffers, buffer_filters);
}

void MainWindow::OnDropdownSelect(int index) {
    if(index == 0) {
        resource_view_.Clear();
        return;
    }

    void* resource = frame_resources_[index-1];
    PopulateResourceView(resource);
}

void MainWindow::SetupResourceDropdown() {
    const auto& resource_references = capture_.sync.resource_references;

    // Add buffers then images to have resources sorted by type.
    for(MEMORY_TYPE to_add_type : {BUFFER_MEMORY, IMAGE_MEMORY}) {
        for(const auto& key_value : resource_references) {
            void* resource = reinterpret_cast<void*>(key_value.first);
            std::uintptr_t resource_val = key_value.first;

            MEMORY_TYPE type = capture_.sync.resource_types[resource_val];
            if(type != to_add_type) continue;

            frame_resources_.push_back(resource);

            QString dropdown_text;
            if(type == BUFFER_MEMORY) {
                dropdown_text = QString::fromStdString(capture_.ResourceName(reinterpret_cast<VkBuffer>(resource)));
            } else {
                dropdown_text = QString::fromStdString(capture_.ResourceName(reinterpret_cast<VkImage>(resource)));
            }

            if(capture_.sync.ResourceHasHazard(resource)) {
                ui->ResourcePicker->insertItem(ui->ResourcePicker->count(), QCommonStyle().standardIcon(QStyle::SP_MessageBoxWarning), dropdown_text);
            } else {
                ui->ResourcePicker->insertItem(ui->ResourcePicker->count(), dropdown_text);
            }
        }
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      submitted_command_buffer_view_(),  // Can't initialize with a widget until ui->setupUi is called.
      resource_view_(),  // Can't initialize with a widget until ui->setupUi is called.
      capture_(FrameCapturer::LoadFile("vkviz_frame_start"))
{
    ui->setupUi(this);

    // Set the splits to be the same size
    ui->Splitter->setSizes({INT_MAX, INT_MAX});

    submitted_command_buffer_view_ = CommandBufferTree(ui->CmdBufferTree, capture_);
    resource_view_ = CommandBufferTree(ui->ResourceTree, capture_);

    PopulateSubmittedCommandBufferView();
    ShowSubmittedCommandBufferView();

    SetupResourceDropdown();
    connect(ui->ResourcePicker, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) { this->OnDropdownSelect(index); });
}

MainWindow::~MainWindow()
{
    delete ui;
}
