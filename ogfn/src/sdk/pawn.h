#pragma once
#include <cstddef>
#include <cstdint>
#include "../mem/process.h"
#include "offsets.h"
#include "types.h"

namespace sdk {
    inline uint64_t pawn_root(uint64_t pawn) {
        return process::rpm<uint64_t>(pawn + offsets::RootComponent);
    }

    inline uint64_t pawn_mesh(uint64_t pawn) {
        return process::rpm<uint64_t>(pawn + offsets::Mesh);
    }

    inline fvector root_component_to_world_translation(uint64_t root) {
        if (!root) return {};
        return process::rpm<fvector>(root + offsets::ComponentToWorld + offsetof(ftransform, translation));
    }

    inline fvector root_relative_location(uint64_t root) {
        if (!root) return {};
        return process::rpm<fvector>(root + offsets::RelativeLocation);
    }

    inline fvector pawn_location(uint64_t pawn) {
        return root_component_to_world_translation(pawn_mesh(pawn));
    }

    inline bool pawn_is_dying(uint64_t pawn) {
        uint8_t bits = process::rpm<uint8_t>(pawn + offsets::bIsDying);
        return (bits >> 3) & 1;
    }

    inline bool pawn_is_dbno(uint64_t pawn) {
        uint8_t bits = process::rpm<uint8_t>(pawn + offsets::bIsDBNO);
        return (bits >> 3) & 1;
    }

    struct fstring_header {
        uint64_t data;
        int32_t  count;
        int32_t  max;
    };

    inline int pawn_name(uint64_t pawn, wchar_t* out, int cap) {
        out[0] = 0;
        uint64_t ps = pawn ? process::rpm<uint64_t>(pawn + offsets::PlayerState) : 0;
        if (!ps) return 0;
        fstring_header h = process::rpm<fstring_header>(ps + offsets::PlayerNamePrivate);
        int len = h.count - 1;
        if (!h.data || len <= 0 || len > 64 || h.count > h.max) return 0;
        if (len > cap - 1) len = cap - 1;
        if (!process::read_raw(h.data, out, (size_t)len * sizeof(wchar_t))) { out[0] = 0; return 0; }
        int shift = (h.count - 1) & 3;
        for (int k = 0; k < len; k++) {
            shift += 3;
            out[k] = (wchar_t)(uint16_t)(out[k] + (shift & 7));
        }
        out[len] = 0;
        return len;
    }

    inline int pawn_team(uint64_t pawn) {
        uint64_t ps = pawn ? process::rpm<uint64_t>(pawn + offsets::PlayerState) : 0;
        if (!ps) return -1;
        return (int)process::rpm<uint8_t>(ps + offsets::TeamIndex);
    }

    inline bool finite3(const fvector& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
}
