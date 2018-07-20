#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "command_buffer.h"
#include "memory_barrier.h"

#include <stdint.h>
#include <unordered_map>
#include <vector>

struct CommandRef {
    VkCommandBuffer buffer;
    uint32_t command;
};

class SyncTracking {
    std::unordered_map<void*, std::vector<std::pair<CommandRef, MemoryAccess>>> resource_accesses;

    //std::array<std::array<std::unordered_map<void*, std::vector<CommandRef>>, 2>, 2> outstanding_accesses;
    std::array<std::unordered_map<void*, std::array<std::vector<CommandRef>, 2>>, 2> outstanding_accesses;

  public:

    void ClearOutstanding(const MemoryBarrier& barrier) {
        outstanding_accesses[IMAGE_MEMORY].clear();
        outstanding_accesses[BUFFER_MEMORY].clear();
    }

    void ClearOutstanding(const BufferBarrier& barrier) {
        outstanding_accesses[BUFFER_MEMORY][barrier.buffer][READ].clear();
        outstanding_accesses[BUFFER_MEMORY][barrier.buffer][WRITE].clear();
    }

    void ClearOutstanding(const ImageBarrier& barrier) {
        outstanding_accesses[IMAGE_MEMORY][barrier.image][READ].clear();
        outstanding_accesses[IMAGE_MEMORY][barrier.image][WRITE].clear();
    }

    void AddAccess(CommandRef loc, MemoryAccess access) {
        void* resource;
        if(access.type == BUFFER_MEMORY) {
            resource = static_cast<void*>(access.buffer_access.buffer);
        } else {
            resource = static_cast<void*>(access.image_access.image);
        }
        outstanding_accesses[access.type][resource][access.read_or_write].push_back(loc);

        if(access.read_or_write == READ) {
            // If outstanding write from different cmd buffer then flag
            std::vector<CommandRef> outstanding_writes_for_this = outstanding_accesses[access.type][resource][WRITE];
            for(const auto& outstanding_write : outstanding_writes_for_this) {
                if(outstanding_write.buffer != loc.buffer) {
                    printf("Read after write hazard\n");
                }
            }
        } else {
            // If outstanding read or write from different cmd buffer then flag
            for(const auto& outstanding_read : outstanding_accesses[access.type][resource][READ]) {
                if(outstanding_read.buffer != loc.buffer) {
                    printf("Write after read hazard\n");
                }
            }
            for(const auto& outstanding_write : outstanding_accesses[access.type][resource][WRITE]) {
                if(outstanding_write.buffer != loc.buffer) {
                    printf("Write after write hazard\n");
                }
            }
        }
    }

    void AddCommandBuffers(const std::vector<std::reference_wrapper<VkVizCommandBuffer>>& command_buffers) {
        CommandRef current = {0, 0};

        // The type here unwraps the command_buffer reference wrappers.
        for(const VkVizCommandBuffer& command_buffer : command_buffers) {
            current.buffer = command_buffer.Handle();
            for(const auto& command : command_buffer.Commands()) {
                for(const auto& access : command.GetAllAccesses()) {
                    AddAccess(current, access);
                }

                if(command.IsPipelineBarrier()) {
                    const VkVizPipelineBarrier& pipeline_barrier = command.Unwrap<PipelineBarrierCommand>().barrier;
                    for(const auto& barrier : pipeline_barrier.global_barriers) {
                        ClearOutstanding(barrier);
                    }
                    for(const auto& barrier : pipeline_barrier.buffer_barriers) {
                        ClearOutstanding(barrier);
                    }
                    for(const auto& barrier : pipeline_barrier.image_barriers) {
                        ClearOutstanding(barrier);
                    }
                }
                current.command++;
            }
        }
    }
};

#endif  // SYNCHRONIZATION_H
