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

void MainWindow::OnDropdownSelect(int index) {
    void* resource;
    if(index == 0) {
        resource = nullptr;
    } else {
        resource = frame_resources_[index - 1];
    }

    resource_view_.SetResource(resource);
}

void MainWindow::SetupResourceDropdown() {
    const auto& resource_references = capture_.sync.resource_references;

    // Add buffers then images to have resources sorted by type.
    for (MEMORY_TYPE current_type_to_add : {BUFFER_MEMORY, IMAGE_MEMORY}) {
        for(const auto& key_value : resource_references) {
            void* resource = reinterpret_cast<void*>(key_value.first);
            std::uintptr_t resource_val = key_value.first;

            MEMORY_TYPE type = capture_.sync.resource_types[resource_val];
            if (type != current_type_to_add) continue;

            frame_resources_.push_back(resource);

            QString dropdown_text;
            if(type == BUFFER_MEMORY) {
                // ResourceName() needs a typed handle.
                dropdown_text = QString::fromStdString(capture_.ResourceName(static_cast<VkBuffer>(resource)));
            } else {
                dropdown_text = QString::fromStdString(capture_.ResourceName(static_cast<VkImage>(resource)));
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
//    ui->Splitter->setSizes({INT_MAX, INT_MAX});
    ui->Splitter->setSizes({INT_MAX, 0});

    SetupResourceDropdown();
    submitted_command_buffer_view_ = CommandBufferTree(ui->CmdBufferTree, capture_, FocusMode::ALL);
    submitted_command_buffer_view_.SetupCallbacks();
    resource_view_ = CommandBufferTree(ui->ResourceTree, capture_, FocusMode::RESOURCE);
    resource_view_.SetupCallbacks();

    // Sets the command buffer view as visible tab.
    ui->tabWidget->setCurrentIndex(1);

    connect(ui->ResourcePicker, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) { this->OnDropdownSelect(index); });
}

MainWindow::~MainWindow()
{
    delete ui;
}
