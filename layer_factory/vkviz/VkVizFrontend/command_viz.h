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
#include "command_tree_widget_item.h"
#include "frame_capture.h"
#include "string_helpers.h"
#include "synchronization.h"
#include "util.h"

#include <QColor>
#include <QTreeWidgetItem>
#include <functional>
#include <sstream>
#include <memory>

class CommandTreeWidgetItem;

// Draw cases:
// 	Current- Highlight all hazards
// 	To Implement- Highlight hazards on current resource
// 	To Implement- Hightlight all resources touched by barrier
// 	To Implement- Highlight this resource touched by barrier

enum class DrawMode { HAZARDS, BARRIER_EFFECTS /*, BARRIERS_AFFECTED_BY*/ };
enum class FocusMode { ALL, RESOURCE };

// inline void SafeTreeClear(QTreeWidget* tree) {
//    QTreeWidgetItem* to_delete;
//    while(to_delete = tree->takeTopLevelItem(0)) {
//        delete to_delete;
//        //to_delete->deleteLater();
//    }
//}

// This is a slight misnoner. This class is in charge of not just setting up command buffer tree views, but also ensuring their
// meta-data is set up right. This includes storing the command locations of certain command widgets in the widgets themselves.
class CommandDrawer {
    // String map
    // Hazard map
    // Per command draw functions

    // Hazards to draw                          // Map of accesses to hazard types
    // Or accesses touched by pipeline barrier  // Filter of touches for accesses

    const FrameCapture* capture_;

   public:
    QTreeWidget* tree_;

   private:
    void* focus_resource_ = nullptr;
    BarrierOccurance focus_barrier_;

    DrawMode draw_mode_ = DrawMode::HAZARDS;
    FocusMode focus_mode_ = FocusMode::ALL;

    void Clear();
    void Redraw() {
        tree_->clear();
        if (focus_mode_ == FocusMode::ALL) {
            for (const auto& buffer : capture_->command_buffers) {
                tree_->addTopLevelItem(CommandTreeWidgetItem::ToQItem(ToWidget(buffer)));
            }
        } else {
            const auto& buffers_commands_for_resource =
                FindOrDefault(capture_->sync.resource_references, reinterpret_cast<std::uintptr_t>(focus_resource_), {});
            for (const auto& buffer : capture_->command_buffers) {
                const auto& commands_to_show = FindOrDefault(buffers_commands_for_resource, buffer.Handle(), {});
                tree_->addTopLevelItem(CommandTreeWidgetItem::ToQItem(RelevantCommandsToWidget(buffer, commands_to_show)));
            }
        }
    };
    void SetDrawMode(DrawMode new_mode) {
        if (draw_mode_ != new_mode) {
            draw_mode_ = new_mode;
            Redraw();
        }
    }
    void SetFocusMode(FocusMode new_mode) {
        if (focus_mode_ != new_mode) {
            focus_mode_ = new_mode;
            Redraw();
        }
    }
    void SetFocusResource(void* resource) {
        if (focus_resource_ = resource) {
            focus_resource_ = resource;
            Redraw();
        }
    }
    void SetFocusBarrier(const BarrierOccurance& barrier) {
        // BarrierOccurance doesn't have an equality operator yet.
        focus_barrier_ = barrier;
        Redraw();
    }

    // Returns which color to set this access to.
    std::pair<bool, QColor> AccessColor(const MemoryAccess& access, const AccessRef& access_location) const;
    // Sets the given access's widget to the color given by AccessColor().
    void ColorAccess(CommandTreeWidgetItem* item, const MemoryAccess& access, const AccessRef& access_location) const;

    void AddMemoryAccessesToParent(CommandTreeWidgetItem* parent, const std::vector<MemoryAccess>& accesses,
                                   AccessRef* current_access) const;

    CommandTreeWidgetItem* FilteredToWidget(
        const VkVizCommandBuffer& command_buffer,
        const std::function<bool(uint32_t command_index, const CommandWrapper& command)>& filter) const;

    std::string ResourceName(VkCommandBuffer buffer) const;
    std::string ResourceName(VkBuffer buffer) const;
    std::string ResourceName(VkImage image) const;

    template <typename T>
    std::string HandleName(T* handle) const;

    template <typename T>
    void AddBarriers(CommandTreeWidgetItem* pipeline_barrier_widget, const std::vector<T>& barriers, VkPipelineStageFlags src_mask,
                     VkPipelineStageFlags dst_mask, const std::string& barrier_type, const CommandRef& barrier_location) const;

    void AddBarrierInfo(CommandTreeWidgetItem* barrier_widget, const MemoryBarrier& barrier) const;
    void AddBarrierInfo(CommandTreeWidgetItem* barrier_widget, const BufferBarrier& barrier) const;
    void AddBarrierInfo(CommandTreeWidgetItem* barrier_widget, const ImageBarrier& barrier) const;

   public:
    CommandDrawer() {};
    CommandDrawer(const FrameCapture& capture, QTreeWidget* tree, FocusMode focus_mode)
        : capture_(&capture), tree_(tree), focus_mode_(focus_mode) {
        Redraw();
    };

    CommandTreeWidgetItem* ToWidget(const BasicCommand& command, const CommandRef& command_location) const;
    CommandTreeWidgetItem* ToWidget(const Access& access, const CommandRef& command_location) const;
    CommandTreeWidgetItem* ToWidget(const DrawCommand& draw_command, const CommandRef& command_location) const;
    CommandTreeWidgetItem* ToWidget(const PipelineBarrierCommand& barrier_command, const CommandRef& command_location) const;

    CommandTreeWidgetItem* ToWidget(const VkVizCommandBuffer& command_buffer) const;
    CommandTreeWidgetItem* RelevantCommandsToWidget(const VkVizCommandBuffer& command_buffer,
                                                    const std::unordered_set<uint32_t>& relevant_commands) const;

    void ShowHazards() { SetDrawMode(DrawMode::HAZARDS); }
    void ShowBarrierEffects(const BarrierOccurance& barrier) {
        SetFocusBarrier(barrier);
        SetDrawMode(DrawMode::BARRIER_EFFECTS);
    }

    void FocusAll() { SetFocusMode(FocusMode::ALL); }
    void FocusResource(void* resource) {
        SetFocusResource(resource);
        SetFocusMode(FocusMode::RESOURCE);
    }
};

struct BasicCommandViz {
    const CommandWrapper& command;
    BasicCommandViz(const CommandWrapper& command): command(command) {}

    virtual CommandTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const {
        return drawer.ToWidget(command.Unwrap<BasicCommand>(), command_location);
    }
};

struct AccessViz : BasicCommandViz {
    AccessViz(const CommandWrapper& command): BasicCommandViz(command) {}

    CommandTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const override {
        return drawer.ToWidget(command.Unwrap<Access>(), command_location);
    }
};

struct DrawCommandViz : BasicCommandViz {
    DrawCommandViz(const CommandWrapper& command): BasicCommandViz(command) {}

    CommandTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const override {
        return drawer.ToWidget(command.Unwrap<DrawCommand>(), command_location);
    }
};

struct PipelineBarrierCommandViz : BasicCommandViz {
    PipelineBarrierCommandViz(const CommandWrapper& command): BasicCommandViz(command) {}

    CommandTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const override {
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

    CommandTreeWidgetItem* ToWidget(const CommandDrawer& drawer, const CommandRef& command_location) const {
        return command_->ToWidget(drawer, command_location);
    }

   protected:
    std::unique_ptr<const BasicCommandViz> command_;
};

#endif  // COMMAND_VIZ_H
