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

#include "command.h"

#include <QTreeWidget>
#include <QString>
#include <sstream>
#include <memory>

std::string PointerToString(void* v) {
    std::stringstream temp;
    temp << v;
    return temp.str();
}

QString PointerToQString(void* v) { return QString::fromStdString(PointerToString(v)); }

QString CmdToQString(CMD_TYPE type) { return QString::fromStdString(cmdToString(type)); }

QString StageName(VkShaderStageFlagBits flag) {
    switch (flag) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return "Vertex Stage";
        case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
            return "Tessellation Control Stage";
        case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
            return "Tessellation Evaluation Stage";
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return "Geometry Stage";
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return "Fragment Stage";
        case VK_SHADER_STAGE_ALL_GRAPHICS:
            return "All Graphics Stages";
        case VK_SHADER_STAGE_ALL:
            return "All Stages";
    }
}

void AddMemoryAccessesToParent(QTreeWidgetItem* parent, const std::vector<MemoryAccess>& accesses) {
    for (const auto& access : accesses) {
        QTreeWidgetItem* access_item = new QTreeWidgetItem();
        std::string access_text;

        if (access.read_or_write == READ) {
            access_text += "Read:  ";
        } else {
            access_text += "Write: ";
        }

        if (access.type == IMAGE_MEMORY) {
            access_text += "Image:  " + PointerToString(access.image_access.image);
        } else {
            access_text += "Buffer: " + PointerToString(access.buffer_access.buffer);
        }

        access_item->setText(0, QString::fromStdString(access_text));
        parent->addChild(access_item);
    }
}

struct BasicCommandViz {
    const std::unique_ptr<BasicCommand>& command;

    virtual QTreeWidgetItem* ToWidget() const {
        QTreeWidgetItem* command_widget = new QTreeWidgetItem();
        command_widget->setText(0, CmdToQString(command->type));
        return command_widget;
    }

    BasicCommandViz() = default;
    BasicCommandViz(const std::unique_ptr<BasicCommand>& command) : command(command) {}
};

struct AccessViz : BasicCommandViz {
    const Access& access;

    QTreeWidgetItem* ToWidget() const override {
        QTreeWidgetItem* access_widget = this->BasicCommandViz::ToWidget();
        AddMemoryAccessesToParent(access_widget, access.accesses);
        return access_widget;
    }

    AccessViz(const std::unique_ptr<BasicCommand>& c) : BasicCommandViz(c), access(*dynamic_cast<Access*>(c.get())) {}
};

struct DrawCommandViz : BasicCommandViz {
    const DrawCommand& draw;

    QTreeWidgetItem* ToWidget() const override {
        QTreeWidgetItem* draw_widget = this->BasicCommandViz::ToWidget();
        for (const auto& stage_access : draw.stage_accesses) {
            QTreeWidgetItem* stage_widget = new QTreeWidgetItem();
            QString stage_name = StageName(stage_access.first);
            stage_widget->setText(0, stage_name);
            AddMemoryAccessesToParent(stage_widget, stage_access.second);

            draw_widget->addChild(stage_widget);
        }
        return draw_widget;
    }

    DrawCommandViz(const std::unique_ptr<BasicCommand>& c) : BasicCommandViz(c), draw(*dynamic_cast<DrawCommand*>(c.get())) {}
};

struct IndexBufferBindViz : BasicCommandViz {
    IndexBufferBindViz(const std::unique_ptr<BasicCommand>& c) : BasicCommandViz(c) {
        // Assert that the command passed in is an IndexBufferBind Command.
        assert(dynamic_cast<IndexBufferBind*>(c.get()));
    }
};

struct PipelineBarrierCommandViz : BasicCommandViz {
    PipelineBarrierCommandViz(const std::unique_ptr<BasicCommand>& c) : BasicCommandViz(c) {
        // Assert that the command passed in is a PipelineBarrier Command.
        assert(dynamic_cast<PipelineBarrierCommand*>(c.get()));
    }
};

struct VertexBufferBindViz : BasicCommandViz {
    VertexBufferBindViz(const std::unique_ptr<BasicCommand>& c) : BasicCommandViz(c) {
        // Assert that the command passed in is a VertexBufferBind Command.
        assert(dynamic_cast<VertexBufferBind*>(c.get()));
    }
};

struct BindDescriptorSetsCommandViz : BasicCommandViz {
    BindDescriptorSetsCommandViz(const std::unique_ptr<BasicCommand>& c) : BasicCommandViz(c) {
        // Assert that the command passed in is a BindDescriptorSets Command.
        assert(dynamic_cast<BindDescriptorSetsCommand*>(c.get()));
    }
};

struct CommandWrapperViz {
    CommandWrapperViz(const CommandWrapper& command) {
        switch (command.VkVizType()) {
            case VkVizCommandType::BASIC:
                command_ = std::make_unique<BasicCommandViz>(command.Unwrap());
                break;
            case VkVizCommandType::ACCESS:
                command_ = std::make_unique<AccessViz>(command.Unwrap());
                break;
            case VkVizCommandType::DRAW:
                command_ = std::make_unique<DrawCommandViz>(command.Unwrap());
                break;
            case VkVizCommandType::INDEX_BUFFER_BIND:
                command_ = std::make_unique<IndexBufferBindViz>(command.Unwrap());
                break;
            case VkVizCommandType::VERTEX_BUFFER_BIND:
                command_ = std::make_unique<VertexBufferBindViz>(command.Unwrap());
                break;
            case VkVizCommandType::PIPELINE_BARRIER:
                command_ = std::make_unique<PipelineBarrierCommandViz>(command.Unwrap());
                break;
            case VkVizCommandType::BIND_DESCRIPTOR_SETS:
                command_ = std::make_unique<BindDescriptorSetsCommandViz>(command.Unwrap());
                break;
        }
    }

    QTreeWidgetItem* ToWidget() const { return command_->ToWidget(); }

   protected:
    std::unique_ptr<const BasicCommandViz> command_;
};

class CommandBufferViz {
    VkCommandBuffer handle_;
    const std::vector<CommandWrapper>& commands_;

   public:
    CommandBufferViz(VkCommandBuffer handle, const std::vector<CommandWrapper>& commands) : handle_(handle), commands_(commands) {}
    QTreeWidgetItem* ToWidget() const {
        QTreeWidgetItem* command_buffer_widget = new QTreeWidgetItem();
        command_buffer_widget->setText(0, PointerToQString(handle_));

        for (const auto& command : commands_) {
            CommandWrapperViz command_viz(command);
            command_buffer_widget->addChild(command_viz.ToWidget());
        }
        return command_buffer_widget;
    }
};
