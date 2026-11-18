#pragma once
#include <cmath>
#include "types.h"
#include "camera.h"

namespace sdk {
    struct projector {
        fvector location;
        fvector axis_x;
        fvector axis_y;
        fvector axis_z;
        float   fov = 80.f;
        float   cx = 0.f;
        float   cy = 0.f;
        int     w = 0;
        int     h = 0;
    };

    inline projector make_projector(const view_info& v, int screen_w, int screen_h) {
        projector p;
        p.location = v.location;
        p.fov = v.fov;
        p.w = screen_w;
        p.h = screen_h;
        p.cx = screen_w * 0.5f;
        p.cy = screen_h * 0.5f;
        fmatrix m = rotator_to_matrix(v.rotation);
        p.axis_x = { m.m[0][0], m.m[0][1], m.m[0][2] };
        p.axis_y = { m.m[1][0], m.m[1][1], m.m[1][2] };
        p.axis_z = { m.m[2][0], m.m[2][1], m.m[2][2] };
        return p;
    }

    inline bool project(const projector& p, const fvector& world, fvector2d& out) {
        fvector d = world - p.location;
        fvector t{ d.dot(p.axis_y), d.dot(p.axis_z), d.dot(p.axis_x) };
        if (t.z < 1.f) return false;
        constexpr float pi = 3.14159265358979323846f;
        float fov_coef = p.cx / tanf(p.fov * pi / 360.f);
        out.x = p.cx + t.x * fov_coef / t.z;
        out.y = p.cy - t.y * fov_coef / t.z;
        return true;
    }

    inline bool on_screen(const projector& p, const fvector2d& s) {
        return s.x > 0.f && s.x < p.w && s.y > 0.f && s.y < p.h;
    }
}
