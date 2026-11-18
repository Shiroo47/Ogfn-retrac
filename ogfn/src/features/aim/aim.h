#pragma once
#include <cmath>
#include <cfloat>
#include <chrono>
#include <unordered_map>
#include <windows.h>
#include "../../cache/cache.h"
#include "../../sdk/camera.h"
#include "../../sdk/mesh.h"
#include "../../sdk/pawn.h"
#include "../../sdk/w2s.h"
#include "../../overlay/overlay.h"

// External aim module for fanta.lol / ogfn.
// Reads bone positions of the closest target to the crosshair inside a FOV,
// computes a screen-space delta, and sends it via SendInput MOUSEEVENTF_MOVE.
//
// This is an *external* aim: no hooks, no injection, just RPM + SendInput.
// It assists the player's real mouse, it does not teleport the view (no silent aim).

namespace aim {

    // Runtime configuration. The menu page in pages.cpp writes to these.
    // Defaults mirror the UI defaults (namespace aim in pages.cpp).
    struct Config {
        bool  enabled           = true;        // enabled by default so you can test right after launch
        int   hotkey_vk         = VK_XBUTTON2; // X2 mouse. change to VK_RBUTTON / VK_XBUTTON1 / 'C' / etc.
        float fov_px            = 120.f;       // screen radius around crosshair
        float smooth            = 6.f;         // divider on delta (higher = smoother/slower)
        float strength          = 60.f;        // percentage multiplier on final delta (0..100)
        float max_distance_m    = 250.f;       // skip targets beyond this
        int   bone              = sdk::bone::head;  // 0 slot in UI = head
        bool  visible_only      = true;        // require recently_rendered
        bool  include_knocked   = true;
        bool  include_teammates = false;
        bool  sticky            = false;       // lock to last target while held

        // Prediction (lead target by its velocity). Velocity is derived from position deltas.
        bool  predict           = true;
        bool  bullet_drop       = true;
        float bullet_speed      = 25000.f;     // cm/s, hitscan-ish. Lower = more lead. (AR ~24000, sniper ~40000)
        float gravity_cms2      = 980.f;       // cm/s^2 projectile drop
    };
    inline Config cfg;

    namespace detail {
        inline uint64_t g_sticky_addr = 0;

        struct vel_sample {
            fvector pos;
            std::chrono::steady_clock::time_point t;
        };
        inline std::unordered_map<uint64_t, vel_sample> g_prev;

        // Estimate velocity for a pawn by diffing the current head/anchor world position
        // against the previous frame's reading. Returns zero velocity the first frame a
        // pawn is seen or if dt is too small.
        inline fvector estimate_velocity(uint64_t addr, const fvector& cur) {
            auto now = std::chrono::steady_clock::now();
            auto it = g_prev.find(addr);
            fvector v{};
            if (it != g_prev.end()) {
                float dt = std::chrono::duration<float>(now - it->second.t).count();
                if (dt > 0.002f && dt < 0.5f) {
                    v.x = (cur.x - it->second.pos.x) / dt;
                    v.y = (cur.y - it->second.pos.y) / dt;
                    v.z = (cur.z - it->second.pos.z) / dt;
                }
                it->second.pos = cur;
                it->second.t = now;
            } else {
                g_prev[addr] = { cur, now };
            }
            // Clamp absurd deltas (teleport / respawn).
            float sp = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
            if (sp > 3000.f) v = {};
            return v;
        }

        inline void send_mouse_move(int dx, int dy) {
            if (dx == 0 && dy == 0) return;
            INPUT in{};
            in.type = INPUT_MOUSE;
            in.mi.dx = dx;
            in.mi.dy = dy;
            in.mi.dwFlags = MOUSEEVENTF_MOVE;
            SendInput(1, &in, sizeof(INPUT));
        }

        inline bool hotkey_held(int vk) {
            if (vk == 0) return false;
            return (GetAsyncKeyState(vk) & 0x8000) != 0;
        }

        inline fvector bone_world(const sdk::mesh_state& st, const ftransform* bones, int b) {
            fmatrix c2w = transform_to_matrix_with_scale(st.c2w);
            return sdk::bone_to_world(bones[b], c2w);
        }
    }

    inline void run(const cache::snapshot& snap) {
        if (!cfg.enabled) { detail::g_sticky_addr = 0; return; }
        if (!detail::hotkey_held(cfg.hotkey_vk)) { detail::g_sticky_addr = 0; return; }
        if (snap.ctx.lobby || !snap.ctx.pawn) return;

        sdk::view_info view = sdk::read_view(snap.ctx);
        if (!view.valid) return;

        sdk::projector proj = sdk::make_projector(view, overlay::width, overlay::height);
        const float cx = overlay::width  * 0.5f;
        const float cy = overlay::height * 0.5f;
        const float fov_sq = cfg.fov_px * cfg.fov_px;

        int   local_team = sdk::pawn_team(snap.ctx.pawn);
        float best_d2    = FLT_MAX;
        fvector2d best_scr{};
        uint64_t best_addr = 0;
        ftransform bones[sdk::bone::count];

        // Sticky: if we already have a locked target, try it first and keep it.
        auto try_pawn = [&](const cache::pawn_entry& p) {
            if (!p.addr || !p.mesh) return;
            if (!cfg.include_knocked && p.knocked) return;
            if (!cfg.include_teammates) {
                int team = sdk::pawn_team(p.addr);
                if (local_team >= 0 && team >= 0 && team == local_team) return;
            }
            sdk::mesh_state st;
            if (!sdk::read_mesh_state(p.mesh, st)) return;
            if (cfg.visible_only && !st.recently_rendered()) return;
            if (!sdk::read_bones(st, bones)) return;

            fvector origin_w = detail::bone_world(st, bones, cfg.bone);
            if (!sdk::finite3(origin_w)) return;

            float dist_cm = origin_w.distance(view.location);
            float dist_m = dist_cm * 0.01f;
            if (dist_m > cfg.max_distance_m) return;

            // --- Prediction: lead the target by its velocity and compensate for gravity ---
            fvector aim_w = origin_w;
            if (cfg.predict || cfg.bullet_drop) {
                fvector v = detail::estimate_velocity(p.addr, origin_w);
                float bs = cfg.bullet_speed < 500.f ? 500.f : cfg.bullet_speed;
                float tof = dist_cm / bs;
                if (cfg.predict) {
                    aim_w.x += v.x * tof;
                    aim_w.y += v.y * tof;
                    aim_w.z += v.z * tof;
                }
                if (cfg.bullet_drop) {
                    aim_w.z += 0.5f * cfg.gravity_cms2 * tof * tof;
                }
            }

            fvector2d scr;
            if (!sdk::project(proj, aim_w, scr)) return;
            float dx = scr.x - cx, dy = scr.y - cy;
            float d2 = dx * dx + dy * dy;
            if (d2 > fov_sq) return;

            if (d2 < best_d2) {
                best_d2 = d2;
                best_scr = scr;
                best_addr = p.addr;
            }
        };

        if (cfg.sticky && detail::g_sticky_addr) {
            for (const auto& p : snap.pawns) {
                if (p.addr == detail::g_sticky_addr) { try_pawn(p); break; }
            }
        }
        if (best_addr == 0) {
            for (const auto& p : snap.pawns) try_pawn(p);
        }

        if (best_addr == 0) { detail::g_sticky_addr = 0; return; }
        detail::g_sticky_addr = cfg.sticky ? best_addr : 0;

        // Compute mouse delta from screen-space offset.
        float dx = best_scr.x - cx;
        float dy = best_scr.y - cy;

        float s = (cfg.smooth < 1.f) ? 1.f : cfg.smooth;
        float strength = cfg.strength / 100.f;

        float fdx = (dx / s) * strength;
        float fdy = (dy / s) * strength;

        int idx = (int)(fdx >= 0.f ? fdx + 0.5f : fdx - 0.5f);
        int idy = (int)(fdy >= 0.f ? fdy + 0.5f : fdy - 0.5f);
        detail::send_mouse_move(idx, idy);
    }

    // Draw the FOV ring. Call from the overlay frame (after esp::run) if you want it visible.
    inline void draw_fov_ring(uint32_t argb_color) {
        if (!cfg.enabled) return;
        const float cx = overlay::width  * 0.5f;
        const float cy = overlay::height * 0.5f;
        overlay::draw::circle(cx, cy, cfg.fov_px, argb_color, 1.2f);
    }
}
