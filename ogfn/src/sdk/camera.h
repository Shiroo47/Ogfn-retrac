#pragma once
#include <cstdint>
#include <cmath>
#include "../mem/process.h"
#include "offsets.h"
#include "pawn.h"
#include "types.h"

namespace sdk {
    struct ctx {
        uint64_t world = 0;
        uint64_t gi    = 0;
        uint64_t lp    = 0;
        uint64_t pc    = 0;
        uint64_t pcm   = 0;
        uint64_t pawn  = 0;
        bool     lobby = false;
    };

    struct view_info {
        fvector  location;
        frotator rotation;
        float    fov = 80.f;
        bool     valid = false;
    };

    constexpr float match_fov = 80.f;

    inline uint64_t camera_object(uint64_t lp) {
        if (!lp) return 0;
        uint64_t a1 = process::rpm<uint64_t>(lp + offsets::CameraPointerA);
        uint64_t a2 = a1 ? process::rpm<uint64_t>(a1 + offsets::CameraPointerAInner) : 0;
        return a2 ? process::rpm<uint64_t>(a2 + offsets::CameraPointerAInner2) : 0;
    }

    inline float read_fov(const ctx& c) {
        if (!c.lobby || !c.pcm) return match_fov;
        float fov = process::rpm<float>(c.pcm + offsets::CameraCachePrivate + offsets::CameraCachePovOffset + offsets::MinimalViewInfoFOV);
        return (std::isfinite(fov) && fov > 5.f && fov < 170.f) ? fov : match_fov;
    }

    inline view_info read_view(const ctx& c) {
        view_info v{};
        uint64_t cam = camera_object(c.lp);
        if (!cam) return v;

        ftransform t{};
        if (!process::rpm_into(cam + offsets::CameraTransform, t)) return v;
        if (!finite3(t.translation) || (t.translation.x == 0.f && t.translation.y == 0.f && t.translation.z == 0.f)) return v;

        fquat q = t.rotation;
        if (!std::isfinite(q.x) || !std::isfinite(q.y) || !std::isfinite(q.z) || !std::isfinite(q.w)) return v;
        float norm = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (fabsf(norm - 1.f) > 0.01f) return v;
        q = { q.x / norm, q.y / norm, q.z / norm, q.w / norm };

        v.location = t.translation;
        v.rotation = quat_to_rotator(q);
        v.fov = read_fov(c);
        v.valid = true;
        return v;
    }
}
