#ifndef FRAME_CAPTURE_H
#define FRAME_CAPTURE_H

#include "command_buffer.h"
#include "synchronization.h"

#include <iostream>
#include <vector>

struct FrameCapture {
    std::vector<VkVizCommandBuffer> command_buffers;
    SyncTracker sync;
};

class FrameCapturer {
    std::ofstream outfile_;
    SyncTracker sync_;

 public:
  void Begin(std::string filename) {
    outfile_.open(filename);
  }

  void AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
    outfile_  << "CommandBuffer" << std::endl;
    outfile_  << json(command_buffer) << std::endl << std::endl;

    sync_.AddCommandBuffer(command_buffer);
  }

  void End() {
    outfile_ << "Synchronization" << std::endl;
    outfile_ << json(sync_) << std::endl << std::endl;

    outfile_.close();

    // Reset the synchronization object.
    sync_ = SyncTracker();
  }

  static FrameCapture LoadFile(std::string filename) {
    FrameCapture capture;
    json j;
    std::ifstream file(filename);
    std::string next_obj;

    file >> next_obj;
    while(next_obj == "CommandBuffer") {
        file >> j;
        capture.command_buffers.emplace_back(j.get<VkVizCommandBuffer>());
        file >> next_obj;
    }

    file >> j;
    capture.sync = j.get<SyncTracker>();

    return capture;
  }
};

#endif  // FRAME_CAPTURE_H
