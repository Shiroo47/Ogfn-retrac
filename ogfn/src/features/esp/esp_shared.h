#pragma once
#include <cmath>
#include <cfloat>
#include "../../sdk/mesh.h"
#include "../../sdk/types.h"

namespace esp {
    struct skel_pair { int from; int to; };
    inline const skel_pair skeleton_lines[] = {
        { sdk::bone::neck,       sdk::bone::head       },
        { sdk::bone::neck,       sdk::bone::spine_03   },
        { sdk::bone::spine_03,   sdk::bone::spine_02   },
        { sdk::bone::spine_02,   sdk::bone::spine_01   },
        { sdk::bone::spine_01,   sdk::bone::pelvis     },
        { sdk::bone::pelvis,     sdk::bone::thigh_l    },
        { sdk::bone::thigh_l,    sdk::bone::calf_l     },
        { sdk::bone::calf_l,     sdk::bone::foot_l     },
        { sdk::bone::pelvis,     sdk::bone::thigh_r    },
        { sdk::bone::thigh_r,    sdk::bone::calf_r     },
        { sdk::bone::calf_r,     sdk::bone::foot_r     },
        { sdk::bone::clavicle_l, sdk::bone::neck       },
        { sdk::bone::clavicle_l, sdk::bone::upperarm_l },
        { sdk::bone::upperarm_l, sdk::bone::lowerarm_l },
        { sdk::bone::lowerarm_l, sdk::bone::hand_l     },
        { sdk::bone::clavicle_r, sdk::bone::neck       },
        { sdk::bone::clavicle_r, sdk::bone::upperarm_r },
        { sdk::bone::upperarm_r, sdk::bone::lowerarm_r },
        { sdk::bone::lowerarm_r, sdk::bone::hand_r     },
    };

    inline const int used_bones[] = {
        sdk::bone::root, sdk::bone::pelvis, sdk::bone::spine_01, sdk::bone::spine_02, sdk::bone::spine_03,
        sdk::bone::clavicle_l, sdk::bone::upperarm_l, sdk::bone::lowerarm_l, sdk::bone::hand_l,
        sdk::bone::clavicle_r, sdk::bone::upperarm_r, sdk::bone::lowerarm_r, sdk::bone::hand_r,
        sdk::bone::neck, sdk::bone::head,
        sdk::bone::thigh_l, sdk::bone::calf_l, sdk::bone::foot_l,
        sdk::bone::thigh_r, sdk::bone::calf_r, sdk::bone::foot_r,
    };

    struct screen_bounds {
        float min_x = FLT_MAX, min_y = FLT_MAX, max_x = -FLT_MAX, max_y = -FLT_MAX;
        int   points = 0;
        void add(const fvector2d& s) {
            min_x = fminf(min_x, s.x);
            min_y = fminf(min_y, s.y);
            max_x = fmaxf(max_x, s.x);
            max_y = fmaxf(max_y, s.y);
            points++;
        }
    };

    struct box_rect { float x, y, w, h; };

    inline box_rect box_from_bounds(const screen_bounds& bb) {
        float bw = bb.max_x - bb.min_x;
        float bh = bb.max_y - bb.min_y;
        float pad = 0.06f * fmaxf(bw, bh) + 1.f;
        return { bb.min_x - pad, bb.min_y, bw + pad * 2.f, bh };
    }

    struct seg2d { float x0, y0, x1, y1; };

    inline int corner_segments(const box_rect& r, float corner_pct, seg2d* out) {
        float L = corner_pct * fminf(r.w, r.h);
        if (L < 2.f) L = 2.f;
        if (L > r.w * 0.5f) L = r.w * 0.5f;
        if (L > r.h * 0.5f) L = r.h * 0.5f;
        const float x = r.x, y = r.y, w = r.w, h = r.h;
        out[0] = { x, y, x + L, y };         out[1] = { x, y, x, y + L };
        out[2] = { x + w, y, x + w - L, y };   out[3] = { x + w, y, x + w, y + L };
        out[4] = { x, y + h, x + L, y + h };   out[5] = { x, y + h, x, y + h - L };
        out[6] = { x + w, y + h, x + w - L, y + h }; out[7] = { x + w, y + h, x + w, y + h - L };
        return 8;
    }
}
