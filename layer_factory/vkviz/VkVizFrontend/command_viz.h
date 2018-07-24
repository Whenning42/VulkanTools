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

#include "color_tree_widget.h"
#include "command.h"
#include "string_helpers.h"
#include "vk_enum_string_helper.h"

#include <QColor>
#include <QTreeWidget>
#include <QString>
#include <QVariant>
#include <sstream>
#include <memory>

QTreeWidgetItem* AddChildWidget(QTreeWidgetItem* parent, const std::string& child_name) {
    QTreeWidgetItem* child = NewWidget(child_name);
    parent->addChild(child);
    return child;
}

bool NodeHasColor(QTreeWidgetItem* node) {
    return !node->data(0, 0x100).isNull();
}

QColor GetNodeColor(QTreeWidgetItem* node) {
    assert(NodeHasColor(node));
    return node->data(0, 0x100).value<QColor>();
}

void SetNodeColor(QTreeWidgetItem* node, QColor color) {
    node->setData(0, 0x100, QVariant(color));
    if(!node->isExpanded()) {
        node->setBackground(0, color);
    }
}

// Sets collapse colors up the tree.
void PropagateColor(QTreeWidgetItem* node, QColor color) {
    if(!node) return;

    if(!NodeHasColor(node)) {
        SetNodeColor(node, color);
    } else {
        if(GetNodeColor(node) == Colors::MultipleColors) {
            // This node already is set to multicolored.
            return;
        } else if(GetNodeColor(node) != color) {
            // This node was set to a different color and is now set to multicolored.
            SetNodeColor(node, Colors::MultipleColors);
        } else {
            // This node has already been set to the given color.
            return;
        }
    }

    PropagateColor(node->parent(), color);
}

void PropagateAddChild(QTreeWidgetItem* parent, QTreeWidgetItem* child) {
    parent->addChild(child);
    if(NodeHasColor(child)) {
        PropagateColor(parent, GetNodeColor(child));
    }
}

QTreeWidgetItem* AddChildWidget(QTreeWidgetItem* parent, const std::string& child_name, QColor color) {
    QTreeWidgetItem* child = AddChildWidget(parent, child_name);
    PropagateColor(child, color);
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
    const BasicCommand& command;

    virtual QTreeWidgetItem* ToWidget() const { return NewWidget(cmdToString(command.type)); }

    BasicCommandViz() = default;
    BasicCommandViz(const CommandWrapper& command) : command(command.Unwrap<BasicCommand>()) {}
};

struct AccessViz : BasicCommandViz {
    const Access& access;

    QTreeWidgetItem* ToWidget() const override {
        QTreeWidgetItem* access_widget = this->BasicCommandViz::ToWidget();
        AddMemoryAccessesToParent(access_widget, access.accesses);
        return access_widget;
    }

    AccessViz(const CommandWrapper& c) : BasicCommandViz(c), access(c.Unwrap<Access>()) {}
};

struct DrawCommandViz : BasicCommandViz {
    const DrawCommand& draw;

    QTreeWidgetItem* ToWidget() const override {
        QTreeWidgetItem* draw_widget = this->BasicCommandViz::ToWidget();

        for (const auto& stage_access : draw.stage_accesses) {
            QTreeWidgetItem* stage_widget = AddChildWidget(draw_widget, string_VkShaderStageFlagBits(stage_access.first), Colors::HazardSource);
            AddMemoryAccessesToParent(stage_widget, stage_access.second);
        }

        return draw_widget;
    }

    DrawCommandViz(const CommandWrapper& c) : BasicCommandViz(c), draw(c.Unwrap<DrawCommand>()) {}
};

struct IndexBufferBindViz : BasicCommandViz {
    IndexBufferBindViz(const CommandWrapper& c) : BasicCommandViz(c) {
        c.AssertHoldsType<IndexBufferBind>();
    }
};

struct PipelineBarrierCommandViz : BasicCommandViz {
    const PipelineBarrierCommand& barrier_command;

   private:
    static std::string to_string(VkAccessFlagBits access_flag) { return string_VkAccessFlagBits(access_flag); }

    static std::string to_string(VkPipelineStageFlagBits stage_flag) { return string_VkPipelineStageFlagBits(stage_flag); }

    template <typename MaskType, typename BitType>
    static std::vector<std::string> MaskNames(MaskType bitmask) {
        std::vector<std::string> set_bits;

        for (uint32_t bit = 0; bit < 32; ++bit) {
            uint32_t flag = 1 << bit;
            if (flag & bitmask) {
                set_bits.push_back(to_string(static_cast<BitType>(flag)));
            }
        }
        return set_bits;
    }

    template <typename MaskType, typename BitType>
    static void AddBitmask(QTreeWidgetItem* parent, MaskType mask, const std::string& mask_display_name) {
        std::vector<std::string> mask_names = MaskNames<MaskType, BitType>(mask);
        if (mask_names.size() == 0) {
            mask_names.push_back("None");
        }

        if (mask_names.size() == 1) {
            // If there's only one bit we don't need to make a drop down for it, and can instead display it inline.
            AddChildWidget(parent, mask_display_name + ": " + mask_names[0]);
        } else {
            // Otherwise we add the bits to a dropdown.
            QTreeWidgetItem* mask_widget = AddChildWidget(parent, mask_display_name);

            for (const auto& bit_name : mask_names) {
                AddChildWidget(mask_widget, bit_name);
            }
        }
    }

    static void AddPipelineStageFlags(QTreeWidgetItem* pipeline_barrier_widget, VkPipelineStageFlags stage_mask,
                                      const std::string& src_or_dst) {}

    static void AddBarrier(QTreeWidgetItem* barrier_widget, const MemoryBarrier& barrier) {
        AddBitmask<VkAccessFlags, VkAccessFlagBits>(barrier_widget, barrier.src_access_mask, "Source Access Mask");
        AddBitmask<VkAccessFlags, VkAccessFlagBits>(barrier_widget, barrier.dst_access_mask, "Destination Access Mask");
    }

    static void AddBarrier(QTreeWidgetItem* barrier_widget, const BufferBarrier& barrier) {
        AddBarrier(barrier_widget, static_cast<MemoryBarrier>(barrier));
        AddChildWidget(barrier_widget, "Buffer: " + PointerToString(barrier.buffer));
    }

    static void AddBarrier(QTreeWidgetItem* barrier_widget, const ImageBarrier& barrier) {
        AddBarrier(barrier_widget, static_cast<MemoryBarrier>(barrier));
        AddChildWidget(barrier_widget, "Image: " + PointerToString(barrier.image));
    }

    template <typename T>
    static QTreeWidgetItem* AddBarriers(QTreeWidgetItem* pipeline_barrier_widget, const std::vector<T>& barriers,
                                        const std::string& barrier_type) {
        if (barriers.size() > 0) {
            QTreeWidgetItem* barriers_parent = pipeline_barrier_widget;

            if (barriers.size() > 1) {
                // If there are multiple barriers of a kind we put them in a dropdown. Otherwise we put them inline.
                barriers_parent = AddChildWidget(pipeline_barrier_widget, barrier_type + " Barriers");
            }

            for (uint32_t i = 0; i < barriers.size(); ++i) {
                QTreeWidgetItem* barrier_widget = AddChildWidget(barriers_parent, barrier_type + " Barrier");
                AddBarrier(barrier_widget, barriers[i]);
            }
        }
    }

   public:
    QTreeWidgetItem* ToWidget() const override {
        QTreeWidgetItem* pipeline_barrier_widget = this->BasicCommandViz::ToWidget();

        AddBitmask<VkPipelineStageFlags, VkPipelineStageFlagBits>(pipeline_barrier_widget, barrier_command.barrier.src_stage_mask,
                                                                  "Source Stage Mask");
        AddBitmask<VkPipelineStageFlags, VkPipelineStageFlagBits>(pipeline_barrier_widget, barrier_command.barrier.dst_stage_mask,
                                                                  "Destination Stage Mask");
        AddBarriers(pipeline_barrier_widget, barrier_command.barrier.global_barriers, "Global");
        AddBarriers(pipeline_barrier_widget, barrier_command.barrier.buffer_barriers, "Buffer");
        AddBarriers(pipeline_barrier_widget, barrier_command.barrier.image_barriers, "Image");

        return pipeline_barrier_widget;
    }

    PipelineBarrierCommandViz(const CommandWrapper& c)
        : BasicCommandViz(c), barrier_command(c.Unwrap<PipelineBarrierCommand>()) {}
};

struct VertexBufferBindViz : BasicCommandViz {
    VertexBufferBindViz(const CommandWrapper& c) : BasicCommandViz(c) {
        c.AssertHoldsType<VertexBufferBind>();
    }
};

struct BindDescriptorSetsCommandViz : BasicCommandViz {
    BindDescriptorSetsCommandViz(const CommandWrapper& c) : BasicCommandViz(c) {
        c.AssertHoldsType<BindDescriptorSetsCommand>();
    }
};

struct CommandWrapperViz {
    CommandWrapperViz(const CommandWrapper& command) {
        switch (command.VkVizType()) {
            case VkVizCommandType::BASIC:
                command_ = std::make_unique<BasicCommandViz>(command);
                break;
            case VkVizCommandType::ACCESS:
                command_ = std::make_unique<AccessViz>(command);
                break;
            case VkVizCommandType::DRAW:
                command_ = std::make_unique<DrawCommandViz>(command);
                break;
            case VkVizCommandType::INDEX_BUFFER_BIND:
                command_ = std::make_unique<IndexBufferBindViz>(command);
                break;
            case VkVizCommandType::VERTEX_BUFFER_BIND:
                command_ = std::make_unique<VertexBufferBindViz>(command);
                break;
            case VkVizCommandType::PIPELINE_BARRIER:
                command_ = std::make_unique<PipelineBarrierCommandViz>(command);
                break;
            case VkVizCommandType::BIND_DESCRIPTOR_SETS:
                command_ = std::make_unique<BindDescriptorSetsCommandViz>(command);
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
            PropagateAddChild(command_buffer_widget, command_viz.ToWidget());
        }
        return command_buffer_widget;
    }

    QTreeWidgetItem* ToFilteredWidget(const std::unordered_set<uint32_t> relevant_commands) {
        if(relevant_commands.size() > 0) {
            QTreeWidgetItem* command_buffer_widget = new QTreeWidgetItem();
            command_buffer_widget->setText(0, PointerToQString(handle_));

            for (uint32_t i=0; i<commands_.size(); ++i) {
                const auto& command = commands_[i];

                // Global barriers aren't recorded in filters for performance but touch all resources.
                if(relevant_commands.find(i) != relevant_commands.end() || command.IsGlobalBarrier()) {
                    CommandWrapperViz command_viz(command);
                    PropagateAddChild(command_buffer_widget, command_viz.ToWidget());
                }
            }

            return command_buffer_widget;
        }

        return nullptr;
    }
};

#endif  // COMMAND_VIZ_H
