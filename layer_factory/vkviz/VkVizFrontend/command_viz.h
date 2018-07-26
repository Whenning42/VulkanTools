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
#include "string_helpers.h"
#include "frame_capture.h"

#include <QColor>
#include <functional>
#include <sstream>
#include <memory>

class QTreeWidget;

// Draw cases:
// 	Current- Highlight all hazards
// 	To Implement- Highlight hazards on current resource
// 	To Implement- Hightlight all resources touched by barrier
// 	To Implement- Highlight this resource touched by barrier

class CommandDrawer {
    // String map
    // Hazard map
    // Per command draw functions

    // Hazards to draw                          // Map of accesses to hazard types
    // Or accesses touched by pipeline barrier  // Filter of touches for accesses

    const FrameCapture* capture_;

    void* focus_resource_;
    VkVizPipelineBarrier focus_barrier_;
    CommandRef focus_barrier_location_;

    enum class DrawState { HAZARDS_ALL, HAZARDS_FOR_RESOURCE, BARRIER_EFFECTS_ALL, BARRIER_EFFECTS_FOR_RESOURCE };
    DrawState draw_state_ = DrawState::HAZARDS_ALL;

    // Returns which color to set this access to.
    std::pair<bool, QColor> AccessColor(QTreeWidgetItem* item, const MemoryAccess& access, const AccessRef& access_location) const;
    // Sets the given access's widget to the color given by AccessColor().
    void ColorAccess(QTreeWidgetItem* item, const MemoryAccess& access, const AccessRef& access_location) const;

    void AddMemoryAccessesToParent(QTreeWidgetItem* parent, const std::vector<MemoryAccess>& accesses,
                                   AccessRef* current_access) const;

    QTreeWidgetItem* FilteredToWidget(
        const VkVizCommandBuffer& command_buffer,
        const std::function<bool(uint32_t command_index, const CommandWrapper& command)>& filter) const;

    std::string ResourceName(VkCommandBuffer buffer) const;
    std::string ResourceName(VkBuffer buffer) const;
    std::string ResourceName(VkImage image) const;

    template <typename T>
    std::string HandleName(T* handle) const;

    template <typename T>
    QTreeWidgetItem* AddBarriers(QTreeWidgetItem* pipeline_barrier_widget, const std::vector<T>& barriers,
                                 const std::string& barrier_type) const;

    void AddBarrier(QTreeWidgetItem* barrier_widget, const MemoryBarrier& barrier) const;
    void AddBarrier(QTreeWidgetItem* barrier_widget, const BufferBarrier& barrier) const;
    void AddBarrier(QTreeWidgetItem* barrier_widget, const ImageBarrier& barrier) const;

   public:
    CommandDrawer() {};
    CommandDrawer(const FrameCapture& capture) : capture_(&capture){};

    QTreeWidgetItem* ToWidget(const BasicCommand& command, const CommandRef& command_location) const;
    QTreeWidgetItem* ToWidget(const Access& access, const CommandRef& command_location) const;
    QTreeWidgetItem* ToWidget(const DrawCommand& draw_command, const CommandRef& command_location) const;
    QTreeWidgetItem* ToWidget(const PipelineBarrierCommand& barrier_command, const CommandRef& command_location) const;

    QTreeWidgetItem* ToWidget(const VkVizCommandBuffer& command_buffer) const;
    QTreeWidgetItem* RelevantCommandsToWidget(const VkVizCommandBuffer& command_buffer, const std::unordered_set<uint32_t>& relevant_commands) const;

    // Draw mode functions
    void ViewAllHazards() { draw_state_ = DrawState::HAZARDS_ALL; }
    void ViewHazardsForResource(void* resource) {
        draw_state_ = DrawState::HAZARDS_FOR_RESOURCE;
        focus_resource_ = resource;
    }

    void ViewAllBarrierEffects() { draw_state_ = DrawState::BARRIER_EFFECTS_ALL; }
    void ViewBarrierEffectsForResource(void* resource) {
        draw_state_ = DrawState::BARRIER_EFFECTS_FOR_RESOURCE;
        focus_resource_ = resource;
    }
};

struct BasicCommandViz {
    const CommandWrapper& command;
    BasicCommandViz(const CommandWrapper& command): command(command) {}

    virtual QTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const {
        return drawer.ToWidget(command.Unwrap<BasicCommand>(), command_location);
    }
};

struct AccessViz : BasicCommandViz {
    AccessViz(const CommandWrapper& command): BasicCommandViz(command) {}

    QTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const override {
        return drawer.ToWidget(command.Unwrap<Access>(), command_location);
    }
};

struct DrawCommandViz : BasicCommandViz {
    DrawCommandViz(const CommandWrapper& command): BasicCommandViz(command) {}

    QTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const override {
        return drawer.ToWidget(command.Unwrap<DrawCommand>(), command_location);
    }
};

struct PipelineBarrierCommandViz : BasicCommandViz {
    PipelineBarrierCommandViz(const CommandWrapper& command): BasicCommandViz(command) {}

    QTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const override {
        return drawer.ToWidget(command.Unwrap<PipelineBarrierCommand>(), command_location);
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
            case VkVizCommandType::PIPELINE_BARRIER:
                command_ = std::make_unique<PipelineBarrierCommandViz>(command);
                break;
            default:
                command_ = std::make_unique<BasicCommandViz>(command);
                break;
        }
    }

    QTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const { return command_->ToWidget(drawer, command_location); }

   protected:
    std::unique_ptr<const BasicCommandViz> command_;
};

#endif  // COMMAND_VIZ_H
