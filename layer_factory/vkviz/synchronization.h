#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "command_buffer.h"
#include "memory_access.h"
#include "memory_barrier.h"
#include "serialize.h"
#include "util.h"

#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct CommandRef {
    VkCommandBuffer buffer;
    uint32_t command_index;
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
};
SERIALIZE3(AccessRef, buffer, command_index, access_index);

enum class HazardSrcOrDst {
    NONE,
    SRC,
    DST
};

struct BarrierOccurance {
    VkPipelineStageFlags src_mask;
    VkPipelineStageFlags dst_mask;
    const MemoryBarrier* barrier;
    CommandRef location;
};

// Represents whether an access comes before or after a barrier.
enum class SubmitRelationToBarrier { BEFORE, AFTER };

// Represents whether an access is in the synchronization scope before or after a barrier or neither.
enum class SyncRelationToBarrier { NONE, BEFORE, AFTER };

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
    std::uintptr_t resource;
};
SERIALIZE4(Hazard, src_access, dst_access, type, resource);

static VkPipelineStageFlags ALL_GRAPHICS_BITS =
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

static VkPipelineStageFlags ALL_COMPUTE_BITS = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                                               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

static VkPipelineStageFlags ALL_TRANSFER_BITS =
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

static VkPipelineStageFlags ALL_BITS = 0x001ffff;

inline VkPipelineStageFlags ExpandPipelineBits(VkPipelineStageFlags flag) {
    if (flag & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT) {
        flag = flag | ALL_GRAPHICS_BITS;
    }
    if (flag & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) {
        flag = flag | ALL_BITS;
    }
    return flag;
}

enum class SyncOrdering { BEFORE, AFTER };

// Asserts that at least one bit is set in stage_flags.
inline VkPipelineStageFlags EarliestStage(VkPipelineStageFlags stage_flags) {
    assert(stage_flags != 0);
    VkPipelineStageFlags earliest_stage = 0;
    for (int bit = 0; bit < 32; ++bit) {
        uint32_t flag = 1 << bit;
        if (flag & stage_flags) return flag;
    }
}

// Asserts that at least one bit is set in stage_flags.
inline VkPipelineStageFlags LatestStage(VkPipelineStageFlags stage_flags) {
    assert(stage_flags != 0);
    VkPipelineStageFlags earliest_stage = 0;
    for (int bit = 31; bit >= 0; --bit) {
        uint32_t flag = 1 << bit;
        if (flag & stage_flags) return flag;
    }
}

// Checks whether or not a pipeline barrier's pipeline stage flags catch a pipeline stage flag in the before or after
// synchronization scopes of the barrier.
inline bool SyncGuaranteesOrdering(VkPipelineStageFlags access_bit, VkPipelineStageFlags sync_flags,
                                   VkPipelineStageFlags implicit_scope_mask, SyncOrdering ordering) {
    // These early returns are for when access_bit or sync_flags aren't in the given scope, i.e. access_bit is a graphics pipeline
    // stage and sync_flags are graphics pipeline stages and we're using the compute stages mask.
    VkPipelineStageFlags scoped_sync_flags = sync_flags & implicit_scope_mask;
    if (scoped_sync_flags == 0) return false;

    if (access_bit & implicit_scope_mask == 0) return false;

    if (ordering == SyncOrdering::BEFORE) {
        return access_bit <= LatestStage(scoped_sync_flags);
    } else {
        return access_bit >= EarliestStage(scoped_sync_flags);
    }
}

inline bool AccessRelationToBarrier(VkPipelineStageFlags source_bit, VkPipelineStageFlags sync_flags, SyncOrdering ordering) {
    sync_flags = ExpandPipelineBits(sync_flags);

    return SyncGuaranteesOrdering(source_bit, sync_flags, ALL_GRAPHICS_BITS, ordering) |
           SyncGuaranteesOrdering(source_bit, sync_flags, ALL_COMPUTE_BITS, ordering) |
           SyncGuaranteesOrdering(source_bit, sync_flags, ALL_TRANSFER_BITS, ordering);
}

inline bool SrcAccessIsBeforeBarrier(VkPipelineStageFlags source_bit, VkPipelineStageFlags sync_flags) {
    return AccessRelationToBarrier(source_bit, sync_flags, SyncOrdering::BEFORE);
}

inline bool DstAccessIsAfterBarrier(VkPipelineStageFlags destination_bit, VkPipelineStageFlags sync_flags) {
    return AccessRelationToBarrier(destination_bit, sync_flags, SyncOrdering::AFTER);
}

class SyncTracker {
 public:
    // We assume we're using unique objects layer so that each resource can be uniquely referenced by a pointer. Also, for some
    // reason, the json library we're using doesn't like to serialize unordered_maps with pointers as keys. We use uintptr_t as our
    // key to work around this.

    // Currently we only track and serialize this info so that it's available to the UI.
    std::unordered_map<std::uintptr_t, std::unordered_map<VkCommandBuffer, std::unordered_set<uint32_t>>> resource_references;
    // This map assumes an access causes at most one hazard. I'm not sure how safe that assumption is in practice.
    std::unordered_map<AccessRef, Hazard, AccessRef::Hash> hazards;
    std::unordered_set<std::uintptr_t> resources_with_hazards;
    std::unordered_map<std::uintptr_t, MEMORY_TYPE> resource_types;
    std::unordered_map<VkCommandBuffer, uint32_t> command_buffer_submit_order;

   private:
    uint32_t current_command_buffer_index = 0;
    std::array<std::unordered_map<void*, std::array<std::vector<AccessRef>, 2>>, 2> outstanding_accesses;

    template<typename T>
    void AddResourceReference(T* resource, CommandRef location) {
        resource_references[reinterpret_cast<std::uintptr_t>(resource)][location.buffer].insert(location.command_index);
    }

    template<typename T>
    void AddHazard(T* resource, const AccessRef& src_access, const AccessRef& dst_access, const HazardType& hazard_type) {
        hazards[src_access] = {src_access, dst_access, hazard_type, reinterpret_cast<std::uintptr_t>(resource)};
        hazards[dst_access] = {src_access, dst_access, hazard_type, reinterpret_cast<std::uintptr_t>(resource)};
    }

    void CheckForHazard(MemoryAccess access, AccessRef access_location) {
        void* resource = access.Handle();

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
        void* resource = access.Handle();
        AddResourceReference(resource, {location.buffer, location.command_index});
        resource_types[reinterpret_cast<std::uintptr_t>(resource)] = access.type;

        CheckForHazard(access, location);

        outstanding_accesses[access.type][resource][access.read_or_write].push_back(location);
    }

  public:
    void AddCommandBuffer(const VkVizCommandBuffer& command_buffer) {
        CommandRef current_command = {command_buffer.Handle(), 0};
        command_buffer_submit_order[command_buffer.Handle()] = current_command_buffer_index;

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
        current_command_buffer_index++;
    }

    void AddCommandBuffers(const std::vector<std::reference_wrapper<VkVizCommandBuffer>>& command_buffers) {
        for(const VkVizCommandBuffer& command_buffer : command_buffers) {
            AddCommandBuffer(command_buffer);
        }
    }

    // Sync logic to enable frontend features.

    uint32_t BufferSubmitIndex(const CommandRef& command) const { return command_buffer_submit_order.at(command.buffer); }

    bool CommandComesBeforeOther(const CommandRef& a, const CommandRef& b) const {
        if (BufferSubmitIndex(a) < BufferSubmitIndex(b))
            return true;
        else if (BufferSubmitIndex(a) == BufferSubmitIndex(b))
            return a.command_index < b.command_index;
        else
            return false;
    }

    bool AccessIsHazard(const AccessRef& access_location) const {
        return hazards.find(access_location) != hazards.end();
    }

    bool AccessIsHazardForResource(const AccessRef& access_location, void* resource) const {
        return FindOrDefault(hazards, access_location, {}).resource == reinterpret_cast<std::uintptr_t>(resource);
    }

    // Checks if the given access has any hazard.
    HazardSrcOrDst HazardIsSrcOrDst(const AccessRef& access_location) const {
        if(!AccessIsHazard(access_location)) return HazardSrcOrDst::NONE;
        else return hazards.at(access_location).src_access == access_location ? HazardSrcOrDst::SRC : HazardSrcOrDst::DST;
    }

    // Checks if the given access has a hazard for the given resource.
    HazardSrcOrDst HazardIsSrcOrDstForResource(const AccessRef& access_location, void* resource) const {
        if (!AccessIsHazardForResource(access_location, resource))
            return HazardSrcOrDst::NONE;
        else
            return hazards.at(access_location).src_access == access_location ? HazardSrcOrDst::SRC : HazardSrcOrDst::DST;
    }

    bool ResourceHasHazard(const void* resource) const {
        return resources_with_hazards.find(reinterpret_cast<std::uintptr_t>(resource)) != resources_with_hazards.end();
    }

    // Need to get image and buffer barriers in here too
    template <typename T>
    SyncRelationToBarrier AccessRelationToBarrier(const MemoryAccess& access, const T& barrier, VkPipelineStageFlags src_stage_mask,
                                                  VkPipelineStageFlags dst_stage_mask,
                                                  SubmitRelationToBarrier submit_relation) const {
        if (!barrier->AffectsResource(access.Handle()))
            return SyncRelationToBarrier::NONE;
        else {
            if (submit_relation == SubmitRelationToBarrier::BEFORE &&
                SrcAccessIsBeforeBarrier(access.pipeline_stage, src_stage_mask))
                return SyncRelationToBarrier::BEFORE;
            else if (submit_relation == SubmitRelationToBarrier::AFTER &&
                     DstAccessIsAfterBarrier(access.pipeline_stage, dst_stage_mask))
                return SyncRelationToBarrier::AFTER;
        }
    }

    template <typename T>
    SyncRelationToBarrier ResourceAccessRelationToBarrier(const MemoryAccess& access, const T& barrier,
                                                          VkPipelineStageFlags src_stage_mask, VkPipelineStageFlags dst_stage_mask,
                                                          void* resource, SubmitRelationToBarrier submit_relation) const {
        // This is an abuse of our enum names. There could be a sync relation between this access and the barrier, it's just one not
        // involving the given resource.
        if (access.Handle() != resource) return SyncRelationToBarrier::NONE;

        return AccessRelationToBarrier(access, barrier, src_stage_mask, dst_stage_mask, submit_relation);
    }
};
SERIALIZE5(SyncTracker, resource_references, resource_types, hazards, resources_with_hazards, command_buffer_submit_order);

#endif  // SYNCHRONIZATION_H
