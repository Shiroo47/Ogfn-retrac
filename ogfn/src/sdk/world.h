#pragma once
#include <cstdint>
#include "../mem/process.h"
#include "offsets.h"
#include "types.h"

namespace sdk {
    inline uint64_t gworld() {
        return process::rpm<uint64_t>(process::base + globals::GWorld);
    }

    inline uint64_t persistent_level(uint64_t world) {
        return process::rpm<uint64_t>(world + offsets::PersistentLevel);
    }

    inline tarray<uint64_t> level_actors(uint64_t level) {
        return process::rpm<tarray<uint64_t>>(level + offsets::Actors);
    }

    inline uint64_t game_instance(uint64_t world) {
        return process::rpm<uint64_t>(world + offsets::GameInstance);
    }

    inline uint64_t local_player(uint64_t gi) {
        uint64_t arr = process::rpm<uint64_t>(gi + offsets::LocalPlayers);
        if (!arr) return 0;
        return process::rpm<uint64_t>(arr);
    }

    inline uint64_t player_controller(uint64_t lp) {
        return process::rpm<uint64_t>(lp + offsets::PlayerController);
    }

    inline uint64_t acknowledged_pawn(uint64_t pc) {
        return process::rpm<uint64_t>(pc + offsets::AcknowledgedPawn);
    }

    inline uint64_t object_class(uint64_t obj) {
        return process::rpm<uint64_t>(obj + guobject_array_offsets::UObject_Class);
    }

    inline bool is_player_pawn_class(uint64_t cls) {
        for (int32_t want : class_sizes::player_pawn_chain) {
            if (!cls || process::rpm<int32_t>(cls + guobject_array_offsets::UStruct_Size) != want) return false;
            cls = process::rpm<uint64_t>(cls + guobject_array_offsets::UStruct_SuperStruct);
        }
        return true;
    }

    inline bool is_frontend_controller(uint64_t pc) {
        uint64_t cls = pc ? object_class(pc) : 0;
        return cls && process::rpm<int32_t>(cls + guobject_array_offsets::UStruct_Size) == class_sizes::FortPlayerControllerFrontEnd;
    }
}
