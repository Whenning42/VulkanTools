#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "command_buffer.h"
#include "memory_barrier.h"
#include "serialize.h"

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CommandRef {
    VkCommandBuffer buffer;
    uint32_t command;
};
SERIALIZE2(CommandRef, buffer, command);

enum HazardType {
    READ_AFTER_WRITE,
    WRITE_AFTER_READ,
    WRITE_AFTER_WRITE
};

struct Hazard {
    CommandRef src;
    CommandRef dst;
    HazardType type;
};
SERIALIZE3(Hazard, src, dst, type);

class SyncTracker {
 public:
    // We assume we're using unique objects layer so that each resource can be uniquely referenced by a pointer. Also, for some
    // reason, the json library we're using doesn't like to serialize unordered_maps with pointers as keys. We use uintptr_t as our
    // key to work around this.

    // Currently we only track and serialize this info so that it's available to the UI.
    std::unordered_map<std::uintptr_t, std::unordered_map<VkCommandBuffer, std::unordered_set<uint32_t>>> resource_references;
    std::unordered_map<std::uintptr_t, std::unordered_map<VkCommandBuffer, std::unordered_map<uint32_t, Hazard>>> resource_hazards;
    std::unordered_map<std::uintptr_t, MEMORY_TYPE> resource_types;

 private:
    std::array<std::unordered_map<void*, std::array<std::vector<CommandRef>, 2>>, 2> outstanding_accesses;

  public:

    // We don't yet track which resources this barrier affects.
    void ClearOutstanding(const MemoryBarrier& barrier, CommandRef location) {
        outstanding_accesses[IMAGE_MEMORY].clear();
        outstanding_accesses[BUFFER_MEMORY].clear();
    }

    void ClearOutstanding(const BufferBarrier& barrier, CommandRef location) {
        outstanding_accesses[BUFFER_MEMORY][barrier.buffer][READ].clear();
        outstanding_accesses[BUFFER_MEMORY][barrier.buffer][WRITE].clear();

        resource_references[reinterpret_cast<std::uintptr_t>(barrier.buffer)][location.buffer].insert(location.command);
    }

    void ClearOutstanding(const ImageBarrier& barrier, CommandRef location) {
        outstanding_accesses[IMAGE_MEMORY][barrier.image][READ].clear();
        outstanding_accesses[IMAGE_MEMORY][barrier.image][WRITE].clear();

        resource_references[reinterpret_cast<std::uintptr_t>(barrier.image)][location.buffer].insert(location.command);
    }

    void AddAccess(MemoryAccess access, CommandRef location) {
        void* resource = access.GetHandle();
        resource_references[reinterpret_cast<std::uintptr_t>(resource)][location.buffer].insert(location.command);
        resource_types[reinterpret_cast<std::uintptr_t>(resource)] = access.type;

        outstanding_accesses[access.type][resource][access.read_or_write].push_back(location);

        if(access.read_or_write == READ) {
            // If outstanding write from different cmd buffer then flag
            std::vector<CommandRef> outstanding_writes_for_this = outstanding_accesses[access.type][resource][WRITE];
            for(const auto& outstanding_write : outstanding_writes_for_this) {
                if(outstanding_write.buffer != location.buffer) {
                    printf("Read after write hazard\n");
                    resource_hazards[reinterpret_cast<std::uintptr_t>(resource)][location.buffer][location.command] = {outstanding_write, location, READ_AFTER_WRITE};
                    resource_hazards[reinterpret_cast<std::uintptr_t>(resource)][outstanding_write.buffer][outstanding_write.command] = {outstanding_write, location, READ_AFTER_WRITE};
                }
            }
        } else {
            // If outstanding read or write from different cmd buffer then flag
            for(const auto& outstanding_read : outstanding_accesses[access.type][resource][READ]) {
                if(outstanding_read.buffer != location.buffer) {
                    printf("Write after read hazard\n");
                    resource_hazards[reinterpret_cast<std::uintptr_t>(resource)][location.buffer][location.command] = {outstanding_read, location, WRITE_AFTER_READ};
                    resource_hazards[reinterpret_cast<std::uintptr_t>(resource)][outstanding_read.buffer][outstanding_read.command] = {outstanding_read, location, WRITE_AFTER_READ};
                }
            }
            for(const auto& outstanding_write : outstanding_accesses[access.type][resource][WRITE]) {
                if(outstanding_write.buffer != location.buffer) {
                    printf("Write after write hazard\n");
                    resource_hazards[reinterpret_cast<std::uintptr_t>(resource)][location.buffer][location.command] = {outstanding_write, location, WRITE_AFTER_WRITE};
                    resource_hazards[reinterpret_cast<std::uintptr_t>(resource)][outstanding_write.buffer][outstanding_write.command] = {outstanding_write, location, WRITE_AFTER_WRITE};
                }
            }
        }
    }

    void AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
        CommandRef current_command = {command_buffer.Handle(), 0};

        for(const auto& command : command_buffer.Commands()) {
            for(const auto& access : command.GetAllAccesses()) {
                AddAccess(access, current_command);
            }

            if(command.IsPipelineBarrier()) {
                const VkVizPipelineBarrier& pipeline_barrier = command.Unwrap<PipelineBarrierCommand>().barrier;
                for(const auto& barrier : pipeline_barrier.global_barriers) {
                    ClearOutstanding(barrier, current_command);
                }
                for(const auto& barrier : pipeline_barrier.buffer_barriers) {
                    ClearOutstanding(barrier, current_command);
                }
                for(const auto& barrier : pipeline_barrier.image_barriers) {
                    ClearOutstanding(barrier, current_command);
                }
            }
            current_command.command++;
        }
    }

    void AddCommandBuffers(const std::vector<std::reference_wrapper<VkVizCommandBuffer>>& command_buffers) {
        for(const VkVizCommandBuffer& command_buffer : command_buffers) {
            AddCommandBuffer(command_buffer);
        }
    }

    void EndFrame() {

    }
};
SERIALIZE3(SyncTracker, resource_references, resource_types, resource_hazards);

#endif  // SYNCHRONIZATION_H
