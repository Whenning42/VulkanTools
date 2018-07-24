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
    uint32_t command_index;

    bool operator==(const CommandRef& other) const {
        return (buffer == other.buffer) && (command_index == other.command_index);
    }
    struct Hash {
        // Should probably be an okay hash.
        size_t operator()(const CommandRef& ref) const {
            return std::hash<uint64_t>{}(std::hash<uint32_t>{}(ref.command_index) << 20 ^ std::hash<void*>{}(ref.buffer));
        }
    };
};
SERIALIZE2(CommandRef, buffer, command_index);

struct AccessRef {
    VkCommandBuffer buffer;
    uint32_t command_index;
    uint32_t access_index;

    bool operator==(const AccessRef& other) const {
        return (buffer == other.buffer) && (command_index == other.command_index) && (access_index == other.access_index);
    }
    struct Hash {
        // Should be an okay hash for conceivable std::hash implementations.
        size_t operator()(const AccessRef& ref) const {
            size_t location_hash = std::hash<uint64_t>{}(static_cast<uint64_t>(ref.command_index) << 32 + ref.access_index);
            return location_hash ^ (reinterpret_cast<std::uintptr_t>(ref.buffer) << 20);
        }
    };

    CommandRef GetCommandRef() const {
        return {buffer, command_index};
    }
};
SERIALIZE3(AccessRef, buffer, command_index, access_index);

enum SrcOrDst {
    SRC,
    DST
};

enum HazardType {
    WRITE_AFTER_WRITE,
    WRITE_AFTER_READ,
    READ_AFTER_WRITE
};

inline bool ReadWriteTypesCauseHazard(READ_WRITE src_rw, READ_WRITE dst_rw) {
    // No such thing as a read-read hazard.
    return src_rw != READ || dst_rw != READ;
}

inline HazardType GetHazardType(READ_WRITE src_rw, READ_WRITE dst_rw) {
    assert(ReadWriteTypesCauseHazard(src_rw, dst_rw));
    if(src_rw == WRITE && dst_rw == WRITE) return HazardType::WRITE_AFTER_WRITE;
    if(src_rw == READ && dst_rw == WRITE) return HazardType::WRITE_AFTER_READ;
    if(src_rw == WRITE && dst_rw == READ) return HazardType::READ_AFTER_WRITE;
}

struct Hazard {
    AccessRef src_access;
    AccessRef dst_access;
    HazardType type;
};
SERIALIZE3(Hazard, src_access, dst_access, type);

class SyncTracker {
 public:
    // We assume we're using unique objects layer so that each resource can be uniquely referenced by a pointer. Also, for some
    // reason, the json library we're using doesn't like to serialize unordered_maps with pointers as keys. We use uintptr_t as our
    // key to work around this.

    // Currently we only track and serialize this info so that it's available to the UI.
    std::unordered_map<std::uintptr_t, std::unordered_map<VkCommandBuffer, std::unordered_set<uint32_t>>> resource_references;
    std::unordered_map<CommandRef, std::unordered_map<uint32_t, Hazard>, CommandRef::Hash> command_hazards;
    std::unordered_set<std::uintptr_t> resources_with_hazards;
    std::unordered_map<std::uintptr_t, MEMORY_TYPE> resource_types;

 private:
    std::array<std::unordered_map<void*, std::array<std::vector<AccessRef>, 2>>, 2> outstanding_accesses;

    template<typename T>
    void AddResourceReference(T* resource, CommandRef location) {
        resource_references[reinterpret_cast<std::uintptr_t>(resource)][location.buffer].insert(location.command_index);
    }

    template<typename T>
    void AddHazard(T* resource, const AccessRef& src_access, const AccessRef& dst_access, const HazardType& hazard_type) {
        command_hazards[src_access.GetCommandRef()][src_access.access_index] = {src_access, dst_access, hazard_type};
        command_hazards[src_access.GetCommandRef()][dst_access.access_index] = {src_access, dst_access, hazard_type};
    }

    void CheckForHazard(MemoryAccess access, AccessRef access_location) {
        void* resource = access.GetHandle();

        MEMORY_TYPE resource_type = access.type;
        READ_WRITE dst_access_rw = access.read_or_write;

        for(READ_WRITE src_access_rw : {READ, WRITE}) {
            if(!ReadWriteTypesCauseHazard(src_access_rw, dst_access_rw)) continue;

            std::vector<AccessRef> outstanding_src_accesses = outstanding_accesses[resource_type][resource][src_access_rw];
            for(const auto& outstanding_src_access : outstanding_src_accesses) {
                if(outstanding_src_access.buffer != access_location.buffer) {
                    AddHazard(resource, outstanding_src_access, access_location, GetHazardType(src_access_rw, dst_access_rw));
                    resources_with_hazards.insert(reinterpret_cast<std::uintptr_t>(resource));
                }
            }
        }
    }

    void ClearOutstanding(const MemoryBarrier& barrier, CommandRef location) {
        outstanding_accesses[IMAGE_MEMORY].clear();
        outstanding_accesses[BUFFER_MEMORY].clear();
    }

    void ClearOutstanding(const BufferBarrier& barrier, CommandRef location) {
        outstanding_accesses[BUFFER_MEMORY][barrier.buffer][READ].clear();
        outstanding_accesses[BUFFER_MEMORY][barrier.buffer][WRITE].clear();

        AddResourceReference(barrier.buffer, location);
    }

    void ClearOutstanding(const ImageBarrier& barrier, CommandRef location) {
        outstanding_accesses[IMAGE_MEMORY][barrier.image][READ].clear();
        outstanding_accesses[IMAGE_MEMORY][barrier.image][WRITE].clear();

        AddResourceReference(barrier.image, location);
    }

    void AddAccess(MemoryAccess access, AccessRef location) {
        void* resource = access.GetHandle();
        AddResourceReference(resource, {location.buffer, location.command_index});
        resource_types[reinterpret_cast<std::uintptr_t>(resource)] = access.type;

        CheckForHazard(access, location);

        outstanding_accesses[access.type][resource][access.read_or_write].push_back(location);
    }

  public:
    void AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
        CommandRef current_command = {command_buffer.Handle(), 0};

        for(const auto& command : command_buffer.Commands()) {

            std::vector<MemoryAccess> command_accesses = command.GetAllAccesses();
            for(uint32_t access_index=0; access_index<command_accesses.size(); ++access_index) {
                AccessRef access_location{current_command.buffer, current_command.command_index, access_index};
                AddAccess(command_accesses[access_index], access_location);
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

            current_command.command_index++;
        }
    }

    void AddCommandBuffers(const std::vector<std::reference_wrapper<VkVizCommandBuffer>>& command_buffers) {
        for(const VkVizCommandBuffer& command_buffer : command_buffers) {
            AddCommandBuffer(command_buffer);
        }
    }

    bool CommandHasHazards(const CommandRef& command) const {
        return command_hazards.find(command) != command_hazards.end();
    }

    std::unordered_map<uint32_t, Hazard>& CommandHazards(const CommandRef& command) {
        return command_hazards.at(command);
    }

    const std::unordered_map<uint32_t, Hazard>& CommandHazards(const CommandRef& command) const {
        CommandHazards(command);
    }

    // Sync logic to enable frontend features.
    bool AccessIsHazard(const AccessRef& access_location) const {
        if(CommandHasHazards(access_location.GetCommandRef())) {
            const auto& hazards = CommandHazards(access_location.GetCommandRef());
            return hazards.find(access_location.access_index) != hazards.end();
        }
        return false;
    }

    SrcOrDst HazardIsSrcOrDst(const AccessRef& access_location) const {
        return CommandHazards(access_location.GetCommandRef()).at(access_location.access_index).src_access == access_location ? SRC : DST;
    }

    bool ResourceHasHazard(const void* resource) const {
        return resources_with_hazards.find(reinterpret_cast<std::uintptr_t>(resource)) != resources_with_hazards.end();
    }
};
SERIALIZE4(SyncTracker, resource_references, resource_types, command_hazards, resources_with_hazards);

#endif  // SYNCHRONIZATION_H
