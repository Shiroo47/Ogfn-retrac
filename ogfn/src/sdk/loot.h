#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "../mem/process.h"
#include "offsets.h"
#include "types.h"
#include "names.h"
#include "world.h"

namespace sdk {
    enum class loot_kind : uint8_t { none, chest, ammo, supply, llama, pickup, vehicle };

    inline tarray<uint64_t> world_levels(uint64_t world) {
        return process::rpm<tarray<uint64_t>>(world + offsets::Levels);
    }

    inline bool ends_with(const char* s, const char* suffix) {
        size_t a = strlen(s), b = strlen(suffix);
        return a >= b && !strcmp(s + a - b, suffix);
    }

    inline loot_kind classify_class(uint64_t cls, const char*& vehicle_label) {
        vehicle_label = nullptr;
        bool container = false, supply = false, pickup = false, vehicle = false;
        char first[128] = {};
        uint64_t c = cls;
        for (int depth = 0; c && depth < 16; depth++) {
            char nm[128];
            if (fname::name_of(c, nm, sizeof(nm))) {
                if (depth == 0) strcpy_s(first, nm);
                if (!strcmp(nm, "Actor")) break;
                if (!strcmp(nm, "BuildingContainer")) container = true;
                else if (!strcmp(nm, "FortAthenaSupplyDrop")) supply = true;
                else if (!strcmp(nm, "FortPickup")) pickup = true;
                else if (!strncmp(nm, "Fort", 4) && ends_with(nm, "Vehicle")) {
                    vehicle = true;
                    if (strstr(nm, "Hoagie")) vehicle_label = "Choppa";
                    else if (strstr(nm, "Meatball")) vehicle_label = "Boat";
                }
            }
            c = process::rpm<uint64_t>(c + guobject_array_offsets::UStruct_SuperStruct);
        }
        if (pickup) return loot_kind::pickup;
        if (vehicle) return loot_kind::vehicle;
        if (strstr(first, "Llama")) return loot_kind::llama;
        if (supply) return strstr(first, "Balloon") ? loot_kind::none : loot_kind::supply;
        if (container) {
            if (strstr(first, "Chest")) return loot_kind::chest;
            if (strstr(first, "Ammo")) return loot_kind::ammo;
        }
        return loot_kind::none;
    }

    inline bool container_searched(uint64_t actor) {
        uint8_t b = process::rpm<uint8_t>(actor + offsets::bAlreadySearched);
        return (b >> offsets::bAlreadySearchedBit) & 1;
    }

    inline uint64_t pickup_def(uint64_t actor, int32_t& count) {
        count = process::rpm<int32_t>(actor + offsets::PrimaryPickupItemEntry + offsets::ItemEntryCount);
        return process::rpm<uint64_t>(actor + offsets::PrimaryPickupItemEntry + offsets::ItemEntryDefinition);
    }

    inline fvector actor_location(uint64_t actor) {
        uint64_t root = process::rpm<uint64_t>(actor + offsets::RootComponent);
        if (!root) return {};
        return process::rpm<fvector>(root + offsets::ComponentToWorld + offsetof(ftransform, translation));
    }
}
