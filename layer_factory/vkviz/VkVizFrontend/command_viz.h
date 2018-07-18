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

#ifndef COMMAND_VIZ_H
#define COMMAND_VIZ_H

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

std::string StageName(VkShaderStageFlagBits flag) {
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

QTreeWidgetItem* NewWidget(const std::string& name) {
    QTreeWidgetItem* widget = new QTreeWidgetItem();
    widget->setText(0, QString::fromStdString(name));
    return widget;
}

QTreeWidgetItem* AddChildWidget(QTreeWidgetItem* parent, const std::string& child_name) {
    QTreeWidgetItem* child = NewWidget(child_name);
    parent->addChild(child);
    return child;
}

void AddMemoryAccessesToParent(QTreeWidgetItem* parent, const std::vector<MemoryAccess>& accesses) {
    for (const auto& access : accesses) {
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

        AddChildWidget(parent, access_text);
    }
}

struct BasicCommandViz {
    const std::unique_ptr<BasicCommand>& command;

    virtual QTreeWidgetItem* ToWidget() const { return NewWidget(cmdToString(command->type)); }

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
            QTreeWidgetItem* stage_widget = AddChildWidget(draw_widget, StageName(stage_access.first));
            AddMemoryAccessesToParent(stage_widget, stage_access.second);
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
    const PipelineBarrierCommand& barrier_command;

   private:
    static void AddBarrierInfo(QTreeWidgetItem* barrier_widget, const MemoryBarrier& barrier) {
        AddChildWidget(barrier_widget, "Source Access Mask: " + std::to_string(barrier.src_access_mask));
        AddChildWidget(barrier_widget, "Destination Access Mask: " + std::to_string(barrier.dst_access_mask));
    }

    static void AddBarrierInfo(QTreeWidgetItem* barrier_widget, const BufferBarrier& barrier) {
        AddBarrierInfo(barrier_widget, static_cast<MemoryBarrier>(barrier));
        AddChildWidget(barrier_widget, "Buffer: " + PointerToString(barrier.buffer));
    }

    static void AddBarrierInfo(QTreeWidgetItem* barrier_widget, const ImageBarrier& barrier) {
        AddBarrierInfo(barrier_widget, static_cast<MemoryBarrier>(barrier));
        AddChildWidget(barrier_widget, "Image: " + PointerToString(barrier.image));
    }

    template <typename T>
    static QTreeWidgetItem* AddBarriers(QTreeWidgetItem* pipeline_barrier_widget, const std::vector<T>& barriers,
                                        const std::string& barrier_type) {
        if (barriers.size() > 0) {
            QTreeWidgetItem* barriers_widget = AddChildWidget(pipeline_barrier_widget, barrier_type + " Barriers");

            for (uint32_t i = 0; i < barriers.size(); ++i) {
                std::string barrier_name = barrier_type + " Barrier " + std::to_string(i);
                QTreeWidgetItem* barrier_widget = AddChildWidget(barriers_widget, barrier_name);
                AddBarrierInfo(barrier_widget, barriers[i]);
            }
        }
    }

   public:
    QTreeWidgetItem* ToWidget() const override {
        QTreeWidgetItem* pipeline_barrier_widget = this->BasicCommandViz::ToWidget();

        AddBarriers(pipeline_barrier_widget, barrier_command.barrier.global_barriers, "Global");
        AddBarriers(pipeline_barrier_widget, barrier_command.barrier.buffer_barriers, "Buffer");
        AddBarriers(pipeline_barrier_widget, barrier_command.barrier.image_barriers, "Image");

        return pipeline_barrier_widget;
    }

    PipelineBarrierCommandViz(const std::unique_ptr<BasicCommand>& c)
        : BasicCommandViz(c), barrier_command(*dynamic_cast<PipelineBarrierCommand*>(c.get())) {}
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

#endif  // COMMAND_VIZ_H
