#include <QColor>
#include <QTreeWidget>
#include <QString>
#include <QVariant>

#include "color_tree_widget.h"
#include "command_viz.h"
#include "string_helpers.h"

namespace {
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

bool NodeHasColor(QTreeWidgetItem* node) { return !node->data(0, 0x100).isNull(); }

QColor GetNodeColor(QTreeWidgetItem* node) {
    assert(NodeHasColor(node));
    return node->data(0, 0x100).value<QColor>();
}

void SetNodeColor(QTreeWidgetItem* node, QColor color) {
    node->setData(0, 0x100, QVariant(color));
    if (!node->isExpanded()) {
        node->setBackground(0, color);
    }
}

// Sets collapse colors up the tree.
void ColorWidget(QTreeWidgetItem* node, QColor color) {
    if (!node) return;

    if (!NodeHasColor(node)) {
        SetNodeColor(node, color);
    } else {
        if (GetNodeColor(node) == Colors::MultipleColors) {
            // This node already is set to multicolored.
            return;
        } else if (GetNodeColor(node) != color) {
            // This node was set to a different color and is now set to multicolored.
            SetNodeColor(node, Colors::MultipleColors);
        } else {
            // This node has already been set to the given color.
            return;
        }
    }

    ColorWidget(node->parent(), color);
}

// Handles tree custom coloring unlike QTreeWidgetItem::addChild.
void AddChild(QTreeWidgetItem* parent, QTreeWidgetItem* child) {
    parent->addChild(child);
    if (NodeHasColor(child)) {
        ColorWidget(parent, GetNodeColor(child));
    }
}
}  // namespace

void CommandDrawer::ColorHazards(QTreeWidgetItem* widget, const AccessRef& access_location) const {
    auto hazard_type = capture_->sync.HazardIsSrcOrDst(access_location);
    if (hazard_type == HazardSrcOrDst::SRC) {
        ColorWidget(widget, Colors::kHazardSource);
    } else if (hazard_type == HazardSrcOrDst::DST) {
        ColorWidget(widget, Colors::kHazardDestination);
    }
}

void CommandDrawer::AddMemoryAccessesToParent(QTreeWidgetItem* parent, const std::vector<MemoryAccess>& accesses,
                                              AccessRef* current_access) const {
    for (const auto& access : accesses) {
        std::string access_text;

        if (access.read_or_write == READ) {
            access_text += "Read:  ";
        } else {
            access_text += "Write: ";
        }

        if (access.type == IMAGE_MEMORY) {
            access_text += capture_->ResourceName(access.image_access.image);
        } else {
            access_text += capture_->ResourceName(access.buffer_access.buffer);
        }

        QTreeWidgetItem* access_widget = AddChildWidget(parent, access_text);
        AddChildWidget(access_widget, to_string(access.pipeline_stage));
        ColorHazards(access_widget, *current_access);
        ++current_access->access_index;
    }
}

QTreeWidgetItem* CommandDrawer::ToWidget(const BasicCommand& command, const CommandRef& command_location) const {
    return NewWidget(cmdToString(command.type));
}

QTreeWidgetItem* CommandDrawer::ToWidget(const Access& access, const CommandRef& command_location) const {
    AccessRef current_access = {command_location.buffer, command_location.command_index, 0};
    QTreeWidgetItem* access_widget = ToWidget(static_cast<BasicCommand>(access), command_location);
    AddMemoryAccessesToParent(access_widget, access.accesses, &current_access);
    return access_widget;
}

QTreeWidgetItem* CommandDrawer::ToWidget(const DrawCommand& draw_command, const CommandRef& command_location) const {
    QTreeWidgetItem* draw_widget = ToWidget(static_cast<BasicCommand>(draw_command), command_location);
    AccessRef current_access = {command_location.buffer, command_location.command_index, 0};

    for (const auto& stage_access : draw_command.stage_accesses) {
        QTreeWidgetItem* stage_widget = AddChildWidget(draw_widget, string_VkShaderStageFlagBits(stage_access.first));
        AddMemoryAccessesToParent(stage_widget, stage_access.second, &current_access);
    }

    return draw_widget;
}

namespace {
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
}  // namespace

void CommandDrawer::AddBarrier(QTreeWidgetItem* barrier_widget, const MemoryBarrier& barrier) const {
    AddBitmask<VkAccessFlags, VkAccessFlagBits>(barrier_widget, barrier.src_access_mask, "Source Access Mask");
    AddBitmask<VkAccessFlags, VkAccessFlagBits>(barrier_widget, barrier.dst_access_mask, "Destination Access Mask");
}

void CommandDrawer::AddBarrier(QTreeWidgetItem* barrier_widget, const BufferBarrier& barrier) const {
    AddBarrier(barrier_widget, static_cast<MemoryBarrier>(barrier));
    AddChildWidget(barrier_widget, capture_->ResourceName(barrier.buffer));
}

void CommandDrawer::AddBarrier(QTreeWidgetItem* barrier_widget, const ImageBarrier& barrier) const {
    AddBarrier(barrier_widget, static_cast<MemoryBarrier>(barrier));
    AddChildWidget(barrier_widget, capture_->ResourceName(barrier.image));
}

template <typename T>
QTreeWidgetItem* CommandDrawer::AddBarriers(QTreeWidgetItem* pipeline_barrier_widget, const std::vector<T>& barriers,
                                            const std::string& barrier_type) const {
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

QTreeWidgetItem* CommandDrawer::ToWidget(const PipelineBarrierCommand& barrier_command, const CommandRef& command_location) const {
    QTreeWidgetItem* pipeline_barrier_widget = ToWidget(static_cast<BasicCommand>(barrier_command), command_location);

    AddBitmask<VkPipelineStageFlags, VkPipelineStageFlagBits>(pipeline_barrier_widget, barrier_command.barrier.src_stage_mask,
                                                              "Source Stage Mask");
    AddBitmask<VkPipelineStageFlags, VkPipelineStageFlagBits>(pipeline_barrier_widget, barrier_command.barrier.dst_stage_mask,
                                                              "Destination Stage Mask");
    AddBarriers(pipeline_barrier_widget, barrier_command.barrier.global_barriers, "Global");
    AddBarriers(pipeline_barrier_widget, barrier_command.barrier.buffer_barriers, "Buffer");
    AddBarriers(pipeline_barrier_widget, barrier_command.barrier.image_barriers, "Image");

    return pipeline_barrier_widget;
}

QTreeWidgetItem* CommandDrawer::FilteredToWidget(
    const VkVizCommandBuffer& command_buffer,
    const std::function<bool(uint32_t command_index, const CommandWrapper& command)>& filter) const {
    QTreeWidgetItem* command_buffer_widget = NewWidget(capture_->ResourceName(command_buffer.Handle()));

    uint32_t added_commands = 0;
    CommandRef command_location = {command_buffer.Handle(), 0};
    for (; command_location.command_index < command_buffer.Commands().size(); ++command_location.command_index) {
        const auto& command = command_buffer.Commands()[command_location.command_index];

        if (filter(command_location.command_index, command)) {
            ++added_commands;

            CommandWrapperViz command_viz(command);
            AddChild(command_buffer_widget, command_viz.ToWidget(*this, command_location));
        }
    }

    if (added_commands > 0) {
        return command_buffer_widget;
    } else {
        delete command_buffer_widget;
        return nullptr;
    }
}

QTreeWidgetItem* CommandDrawer::ToWidget(const VkVizCommandBuffer& command_buffer) const {
    return FilteredToWidget(command_buffer, [](uint32_t, const CommandWrapper&) { return true; });
}

QTreeWidgetItem* CommandDrawer::RelevantCommandsToWidget(const VkVizCommandBuffer& command_buffer,
                                                         const std::unordered_set<uint32_t>& relevant_commands) const {
    auto relevant_command_filter = [&relevant_commands](int command_index, const CommandWrapper& command) {
        return relevant_commands.find(command_index) != relevant_commands.end() || command.IsGlobalBarrier();
    };

    return FilteredToWidget(command_buffer, relevant_command_filter);
}
