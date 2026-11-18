#pragma once
#include <cstdint>
#include <cstring>
#include "../mem/process.h"
#include "offsets.h"
#include "types.h"

namespace sdk {
    namespace bone {
        constexpr int root      = 0;
        constexpr int pelvis    = 2;
        constexpr int spine_01  = 3;
        constexpr int spine_02  = 4;
        constexpr int spine_03  = 5;
        constexpr int clavicle_l = 8;
        constexpr int upperarm_l = 9;
        constexpr int lowerarm_l = 10;
        constexpr int hand_l     = 11;
        constexpr int clavicle_r = 36;
        constexpr int upperarm_r = 37;
        constexpr int lowerarm_r = 38;
        constexpr int hand_r     = 39;
        constexpr int neck      = 64;
        constexpr int head      = 66;
        constexpr int thigh_l   = 67;
        constexpr int calf_l    = 68;
        constexpr int foot_l    = 71;
        constexpr int thigh_r   = 74;
        constexpr int calf_r    = 75;
        constexpr int foot_r    = 78;
        constexpr int count     = 79;
    }

    struct mesh_state {
        ftransform c2w;
        uint64_t   bone_data = 0;
        int32_t    bone_count = 0;
        uint8_t    render_byte = 0;
        bool recently_rendered() const { return (render_byte >> offsets::RecentlyRenderedBit) & 1; }
    };

    inline bool read_mesh_state(uint64_t mesh, mesh_state& out) {
        constexpr size_t first = offsets::ComponentToWorld;
        constexpr size_t last = offsets::RecentlyRendered + 1;
        uint8_t buf[last - first];
        if (!mesh || !process::read_raw(mesh + first, buf, sizeof(buf))) return false;
        memcpy(&out.c2w, buf, sizeof(ftransform));
        tarray<ftransform> a0{}, a1{};
        memcpy(&a0, buf + (offsets::BoneArray - first), sizeof(a0));
        memcpy(&a1, buf + (offsets::BoneArray + offsets::BoneArrayFallback - first), sizeof(a1));
        const tarray<ftransform>& a = a0.data ? a0 : a1;
        out.bone_data = a.data;
        out.bone_count = a.count;
        out.render_byte = buf[offsets::RecentlyRendered - first];
        return true;
    }

    inline bool read_bones(const mesh_state& m, ftransform* out) {
        if (!m.bone_data || m.bone_count < bone::count) return false;
        return process::read_raw(m.bone_data, out, sizeof(ftransform) * bone::count);
    }

    inline fvector bone_to_world(const ftransform& b, const fmatrix& c2w) {
        fmatrix m = mat_mul(transform_to_matrix_with_scale(b), c2w);
        return { m.m[3][0], m.m[3][1], m.m[3][2] };
    }
}
