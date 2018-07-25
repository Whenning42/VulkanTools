#ifndef FRAME_CAPTURE_H
#define FRAME_CAPTURE_H

#include "command_buffer.h"
#include "string_helpers.h"
#include "synchronization.h"

#include <iostream>
#include <string>
#include <vector>

struct FrameCapture {
    std::vector<VkVizCommandBuffer> command_buffers;
    SyncTracker sync;
    std::unordered_map<std::uintptr_t, std::string> handle_names;

    // Viz logic.
    template <typename T>
    std::string HandleName(T* handle) const {
        std::uintptr_t handle_val = reinterpret_cast<std::uintptr_t>(handle);
        if (handle_names.find(handle_val) != handle_names.end()) {
            return handle_names.at(handle_val);
        } else {
            return PointerToString(handle);
        }
    }

    std::string ResourceName(VkCommandBuffer buffer) const { return "Command Buffer: " + HandleName(buffer); }

    std::string ResourceName(VkBuffer buffer) const { return "Buffer: " + HandleName(buffer); }

    std::string ResourceName(VkImage image) const { return "Image: " + HandleName(image); }
};

class FrameCapturer {
    std::ofstream outfile_;
    SyncTracker sync_;
    std::unordered_map<std::uintptr_t, std::string> handle_names_;

   public:
    FrameCapturer(){};

    void Begin(std::string filename) { outfile_.open(filename); }

    template <typename T>
    void WriteSection(std::string section_name, const T& obj) {
        outfile_ << section_name << std::endl;
        outfile_ << json(obj) << std::endl << std::endl;
    }

  void AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
      WriteSection("CommandBuffer", command_buffer);
      sync_.AddCommandBuffer(command_buffer);
  }

  void NameResource(void* resource, std::string name) { handle_names_[reinterpret_cast<std::uintptr_t>(resource)] = name; }

  void End() {
      WriteSection("Synchronization", sync_);
      WriteSection("HandleNames", handle_names_);

      outfile_.close();

      // Reset the synchronization object at the end of a frame.
      sync_ = SyncTracker();
  }

  static FrameCapture LoadFile(std::string filename) {
    FrameCapture capture;
    json j;
    std::ifstream file(filename);
    std::string next_obj;

    while (file >> next_obj) {
        file >> j;
        if (next_obj == "CommandBuffer") {
            capture.command_buffers.emplace_back(j.get<VkVizCommandBuffer>());
        } else if (next_obj == "Synchronization") {
            capture.sync = j.get<SyncTracker>();
        } else if (next_obj == "HandleNames") {
            capture.handle_names = j.get<std::unordered_map<std::uintptr_t, std::string>>();
        }
    }

    return capture;
  }
};

#endif  // FRAME_CAPTURE_H
