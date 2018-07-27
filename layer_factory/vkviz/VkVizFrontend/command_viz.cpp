#include <QTreeWidget>
#include <QString>
#include <QVariant>

#include "color_tree_widget.h"
#include "command_tree_widget_item.h"
#include "command_viz.h"
#include "string_helpers.h"

std::pair<bool, QColor> CommandDrawer::AccessColor(const MemoryAccess& access, const AccessRef& access_location) const {
    if (draw_state_ == DrawState::HAZARDS_ALL || draw_state_ == DrawState::HAZARDS_FOR_RESOURCE) {
        HazardSrcOrDst hazard_type;
        if (draw_state_ == DrawState::HAZARDS_ALL) {
            hazard_type = capture_->sync.HazardIsSrcOrDst(access_location);
        } else {
            hazard_type = capture_->sync.HazardIsSrcOrDstForResource(access_location, focus_resource_);
        }

        if (hazard_type == HazardSrcOrDst::SRC)
            return {true, Colors::kHazardSource};
        else if (hazard_type == HazardSrcOrDst::DST)
            return {true, Colors::kHazardDestination};
    } else {
        SubmitRelationToBarrier submit_relation;
        if (capture_->sync.CommandComesBeforeOther(CommandRef{access_location.buffer, access_location.command_index}, focus_barrier_.location)) {
            submit_relation = SubmitRelationToBarrier::BEFORE;
        } else {
            submit_relation = SubmitRelationToBarrier::AFTER;
        }

        SyncRelationToBarrier sync_relation;
        if (draw_state_ == DrawState::BARRIER_EFFECTS_ALL) {
            sync_relation = capture_->sync.AccessRelationToBarrier(access, focus_barrier_.barrier, focus_barrier_.src_mask,
                                                                   focus_barrier_.dst_mask, submit_relation);
        } else {
            sync_relation = capture_->sync.ResourceAccessRelationToBarrier(
                access, focus_barrier_.barrier, focus_barrier_.src_mask, focus_barrier_.dst_mask, focus_resource_, submit_relation);
        }

        if (sync_relation == SyncRelationToBarrier::BEFORE)
            return {true, Colors::kBeforeBarrier};
        else if (sync_relation == SyncRelationToBarrier::AFTER)
            return {true, Colors::kAfterBarrier};
    }

    return {false, QColor()};
}

void CommandDrawer::ColorAccess(CommandTreeWidgetItem* widget, const MemoryAccess& access, const AccessRef& access_location) const {
    const auto set_color = AccessColor(access, access_location);
    if (set_color.first) {
        widget->SetColor(set_color.second);
    }
}

void CommandDrawer::AddMemoryAccessesToParent(CommandTreeWidgetItem* parent, const std::vector<MemoryAccess>& accesses,
                                              AccessRef* access_location) const {
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

        CommandTreeWidgetItem* access_widget = AddNewChildWidget(parent, access_text);
        AddNewChildWidget(access_widget, to_string(access.pipeline_stage));

        ColorAccess(access_widget, access, *access_location);

        ++access_location->access_index;
    }
}

CommandTreeWidgetItem* CommandDrawer::ToWidget(const BasicCommand& command, const CommandRef& command_location) const {
    return CommandTreeWidgetItem::NewWidget(cmdToString(command.type));
}

CommandTreeWidgetItem* CommandDrawer::ToWidget(const Access& access, const CommandRef& command_location) const {
    AccessRef current_access = {command_location.buffer, command_location.command_index, 0};
    CommandTreeWidgetItem* access_widget = ToWidget(static_cast<BasicCommand>(access), command_location);
    AddMemoryAccessesToParent(access_widget, access.accesses, &current_access);
    return access_widget;
}

CommandTreeWidgetItem* CommandDrawer::ToWidget(const DrawCommand& draw_command, const CommandRef& command_location) const {
    CommandTreeWidgetItem* draw_widget = ToWidget(static_cast<BasicCommand>(draw_command), command_location);
    AccessRef current_access = {command_location.buffer, command_location.command_index, 0};

    for (const auto& stage_access : draw_command.stage_accesses) {
        CommandTreeWidgetItem* stage_widget = AddNewChildWidget(draw_widget, string_VkShaderStageFlagBits(stage_access.first));
        AddMemoryAccessesToParent(stage_widget, stage_access.second, &current_access);
    }

    return draw_widget;
}

namespace {
template <typename MaskType, typename BitType>
static void AddBitmask(CommandTreeWidgetItem* parent, MaskType mask, const std::string& mask_display_name) {
    std::vector<std::string> mask_names = MaskNames<MaskType, BitType>(mask);
    if (mask_names.size() == 0) {
        mask_names.push_back("None");
    }

    if (mask_names.size() == 1) {
        // If there's only one bit we don't need to make a drop down for it, and can instead display it inline.
        AddNewChildWidget(parent, mask_display_name + ": " + mask_names[0]);
    } else {
        // Otherwise we add the bits to a dropdown.
        CommandTreeWidgetItem* mask_widget = AddNewChildWidget(parent, mask_display_name);

        for (const auto& bit_name : mask_names) {
            AddNewChildWidget(mask_widget, bit_name);
        }
    }
}
}  // namespace

void CommandDrawer::AddBarrierInfo(CommandTreeWidgetItem* barrier_widget, const MemoryBarrier& barrier) const {
    AddBitmask<VkAccessFlags, VkAccessFlagBits>(barrier_widget, barrier.src_access_mask, "Source Access Mask");
    AddBitmask<VkAccessFlags, VkAccessFlagBits>(barrier_widget, barrier.dst_access_mask, "Destination Access Mask");
}

void CommandDrawer::AddBarrierInfo(CommandTreeWidgetItem* barrier_widget, const BufferBarrier& barrier) const {
    AddBarrierInfo(barrier_widget, static_cast<MemoryBarrier>(barrier));
    AddNewChildWidget(barrier_widget, capture_->ResourceName(barrier.buffer));
}

void CommandDrawer::AddBarrierInfo(CommandTreeWidgetItem* barrier_widget, const ImageBarrier& barrier) const {
    AddBarrierInfo(barrier_widget, static_cast<MemoryBarrier>(barrier));
    AddNewChildWidget(barrier_widget, capture_->ResourceName(barrier.image));
}

template <typename T>
CommandTreeWidgetItem* CommandDrawer::AddBarriers(CommandTreeWidgetItem* pipeline_barrier_widget, const std::vector<T>& barriers, VkPipelineStageFlags src_mask, VkPipelineStageFlags dst_mask, const std::string& barrier_type, const CommandRef& barrier_location) const {
    if (barriers.size() > 0) {
        CommandTreeWidgetItem* barriers_parent = pipeline_barrier_widget;

        if (barriers.size() > 1) {
            // If there are multiple barriers of a kind we put them in a dropdown. Otherwise we put them inline.
            barriers_parent = AddNewChildWidget(pipeline_barrier_widget, barrier_type + " Barriers");
        }

        for (uint32_t i = 0; i < barriers.size(); ++i) {
            BarrierOccurance occurance = {src_mask, dst_mask, &barriers[i], barrier_location};
            CommandTreeWidgetItem* barrier_widget = CommandTreeWidgetItem::NewBarrier(barrier_type + " Barrier", occurance);
            barriers_parent->addChild(barrier_widget);

            AddBarrierInfo(barrier_widget, barriers[i]);
        }
    }
}

CommandTreeWidgetItem* CommandDrawer::ToWidget(const PipelineBarrierCommand& barrier_command,
                                               const CommandRef& command_location) const {
    CommandTreeWidgetItem* pipeline_barrier_widget = ToWidget(static_cast<BasicCommand>(barrier_command), command_location);

    AddBitmask<VkPipelineStageFlags, VkPipelineStageFlagBits>(pipeline_barrier_widget, barrier_command.barrier.src_stage_mask,
                                                              "Source Stage Mask");
    AddBitmask<VkPipelineStageFlags, VkPipelineStageFlagBits>(pipeline_barrier_widget, barrier_command.barrier.dst_stage_mask,
                                                              "Destination Stage Mask");

    AddBarriers(pipeline_barrier_widget, barrier_command.barrier.global_barriers, barrier_command.barrier.src_stage_mask, barrier_command.barrier.dst_stage_mask, "Global", command_location);
    AddBarriers(pipeline_barrier_widget, barrier_command.barrier.buffer_barriers, barrier_command.barrier.src_stage_mask, barrier_command.barrier.dst_stage_mask, "Buffer", command_location);
    AddBarriers(pipeline_barrier_widget, barrier_command.barrier.image_barriers, barrier_command.barrier.src_stage_mask, barrier_command.barrier.dst_stage_mask, "Image", command_location);

    return pipeline_barrier_widget;
}

CommandTreeWidgetItem* CommandDrawer::FilteredToWidget(
    const VkVizCommandBuffer& command_buffer,
    const std::function<bool(uint32_t command_index, const CommandWrapper& command)>& filter) const {
    CommandTreeWidgetItem* command_buffer_widget =
        CommandTreeWidgetItem::NewWidget(capture_->ResourceName(command_buffer.Handle()));

    uint32_t added_commands = 0;
    CommandRef command_location = {command_buffer.Handle(), 0};
    for (; command_location.command_index < command_buffer.Commands().size(); ++command_location.command_index) {
        const auto& command = command_buffer.Commands()[command_location.command_index];

        if (filter(command_location.command_index, command)) {
            ++added_commands;

            CommandWrapperViz command_viz(command);
            command_buffer_widget->addChild(command_viz.ToWidget(*this, command_location));
        }
    }

    if (added_commands > 0) {
        return command_buffer_widget;
    } else {
        delete command_buffer_widget;
        return nullptr;
    }
}

CommandTreeWidgetItem* CommandDrawer::ToWidget(const VkVizCommandBuffer& command_buffer) const {
    return FilteredToWidget(command_buffer, [](uint32_t, const CommandWrapper&) { return true; });
}

CommandTreeWidgetItem* CommandDrawer::RelevantCommandsToWidget(const VkVizCommandBuffer& command_buffer,
                                                               const std::unordered_set<uint32_t>& relevant_commands) const {
    auto relevant_command_filter = [&relevant_commands](int command_index, const CommandWrapper& command) {
        return relevant_commands.find(command_index) != relevant_commands.end() || command.IsGlobalBarrier();
    };

    return FilteredToWidget(command_buffer, relevant_command_filter);
}
