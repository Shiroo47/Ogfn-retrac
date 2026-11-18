#pragma once
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <chrono>
#include <vector>
#include "../../cache/cache.h"
#include "../../sdk/camera.h"
#include "../../sdk/pawn.h"
#include "../../sdk/w2s.h"
#include "../../sdk/mesh.h"
#include "../../overlay/overlay.h"
#include "../../settings/settings.h"
#include "../../cache/loot_cache.h"
#include "esp_shared.h"

namespace esp {
    inline sdk::view_info                        last_view;
    inline std::chrono::steady_clock::time_point last_view_time{};

    constexpr float head_top_offset = 15.f;
    constexpr float fallback_height = 180.f;

    inline uint32_t to_argb(const settings::Color& c, float a = 1.f) {
        auto ch = [](float v) -> uint8_t {
            int i = (int)(v * 255.f + 0.5f);
            return (uint8_t)(i < 0 ? 0 : (i > 255 ? 255 : i));
        };
        return overlay::draw::argb(ch(a), ch(c.r), ch(c.g), ch(c.b));
    }

    constexpr int shorten_len = 14;

    inline uint32_t with_alpha(uint32_t c, float a) {
        uint32_t base = (c >> 24) & 0xFF;
        uint32_t na = (uint32_t)(base * fminf(fmaxf(a, 0.f), 1.f));
        return (na << 24) | (c & 0x00FFFFFF);
    }

    inline float fade_alpha(float dist_m, float max_m) {
        if (max_m <= 1.f) return 1.f;
        return fminf(fmaxf(1.f - (dist_m / max_m) * 0.75f, 0.25f), 1.f);
    }

    inline const wchar_t* shortened(const wchar_t* s, bool on, wchar_t* buf, int cap) {
        if (!on || (int)wcslen(s) <= shorten_len || cap <= shorten_len + 2) return s;
        wcsncpy_s(buf, cap, s, shorten_len);
        buf[shorten_len] = L'\x2026';
        buf[shorten_len + 1] = 0;
        return buf;
    }

    inline void box_label(const box_rect& br, float y, const wchar_t* s, uint32_t col, const settings::Config& cfg) {
        wchar_t buf[48];
        const wchar_t* t = shortened(s, cfg.shorten, buf, 48);
        if (cfg.text_align == 0)      overlay::draw::label(br.x, y, t, col, overlay::draw::align::left);
        else if (cfg.text_align == 2) overlay::draw::label(br.x + br.w, y, t, col, overlay::draw::align::right);
        else                          overlay::draw::label(br.x + br.w * 0.5f, y, t, col, overlay::draw::align::center);
    }

    inline bool loot_enabled(const settings::Config& cfg, sdk::loot_kind k, uint32_t& col) {
        switch (k) {
            case sdk::loot_kind::chest:   col = to_argb(cfg.chest_color);   return cfg.loot_chests;
            case sdk::loot_kind::ammo:    col = to_argb(cfg.ammo_color);    return cfg.loot_ammo;
            case sdk::loot_kind::supply:  col = to_argb(cfg.supply_color);  return cfg.loot_supply;
            case sdk::loot_kind::llama:   col = to_argb(cfg.llama_color);   return cfg.loot_llamas;
            case sdk::loot_kind::vehicle: col = to_argb(cfg.vehicle_color); return cfg.loot_vehicles;
            case sdk::loot_kind::pickup:  col = to_argb(cfg.text_color);    return cfg.loot_floor;
            default: return false;
        }
    }

    inline uint32_t rarity_argb(uint8_t rarity, uint32_t fallback);

    inline void draw_loot(const settings::Config& cfg, const sdk::projector& proj, const sdk::view_info& view) {
        std::vector<loot::item> items = loot::get();
        for (const loot::item& it : items) {
            uint32_t col;
            if (!loot_enabled(cfg, it.kind, col)) continue;
            float dist_m = it.pos.distance(view.location) * 0.01f;
            if (dist_m > cfg.loot_distance) continue;
            if (it.kind == sdk::loot_kind::pickup) {
                int r = it.rarity == sdk::no_rarity ? 0 : it.rarity;
                if (r < cfg.loot_min_rarity) continue;
                col = rarity_argb(it.rarity, col);
            }
            fvector2d s;
            if (!sdk::project(proj, it.pos, s) || !sdk::on_screen(proj, s)) continue;
            wchar_t buf[48];
            const wchar_t* name = shortened(it.label, cfg.shorten, buf, 48);
            wchar_t text[64];
            swprintf(text, 64, L"%ls %.0fm", name, dist_m);
            overlay::draw::text_centered(s.x, s.y, text, col);
        }
    }

    inline uint32_t rarity_argb(uint8_t rarity, uint32_t fallback) {
        switch (rarity) {
            case 0: return overlay::draw::argb(255, 190, 190, 190);
            case 1: return overlay::draw::argb(255, 96, 200, 60);
            case 2: return overlay::draw::argb(255, 70, 160, 255);
            case 3: return overlay::draw::argb(255, 190, 90, 245);
            case 4: return overlay::draw::argb(255, 245, 160, 50);
            case 5: return overlay::draw::argb(255, 255, 215, 80);
            default: return fallback;
        }
    }

    inline bool current_view(const sdk::ctx& c, sdk::view_info& out) {
        auto now = std::chrono::steady_clock::now();
        sdk::view_info v = sdk::read_view(c);
        if (v.valid) {
            last_view = v;
            last_view_time = now;
            out = v;
            return true;
        }
        if (last_view.valid && now - last_view_time < std::chrono::milliseconds(300)) {
            out = last_view;
            return true;
        }
        return false;
    }

    inline void draw_crosshair() {
        const settings::Config& cfg = settings::cfg;
        if (!cfg.crosshair) return;
        const float cx = overlay::width * 0.5f;
        const float cy = overlay::height * 0.5f;
        const uint32_t col = to_argb(cfg.crosshair_color);
        const float g = cfg.crosshair_gap;
        const float s = cfg.crosshair_size;
        overlay::draw::line(cx - g - s, cy, cx - g, cy, col, 1.5f);
        overlay::draw::line(cx + g, cy, cx + g + s, cy, col, 1.5f);
        overlay::draw::line(cx, cy - g - s, cx, cy - g, col, 1.5f);
        overlay::draw::line(cx, cy + g, cx, cy + g + s, col, 1.5f);
        if (cfg.crosshair_dot) overlay::draw::filled_box(cx - 1.f, cy - 1.f, 2.f, 2.f, col);
    }

    inline void draw_box_3d(const sdk::projector& proj, const fvector& wmin, const fvector& wmax, uint32_t col, float thick) {
        fvector c[8] = {
            { wmin.x, wmin.y, wmin.z }, { wmax.x, wmin.y, wmin.z }, { wmax.x, wmax.y, wmin.z }, { wmin.x, wmax.y, wmin.z },
            { wmin.x, wmin.y, wmax.z }, { wmax.x, wmin.y, wmax.z }, { wmax.x, wmax.y, wmax.z }, { wmin.x, wmax.y, wmax.z },
        };
        fvector2d s[8];
        bool ok[8];
        for (int i = 0; i < 8; i++) ok[i] = sdk::project(proj, c[i], s[i]);
        static const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
        };
        for (auto& e : edges)
            if (ok[e[0]] && ok[e[1]])
                overlay::draw::line(s[e[0]].x, s[e[0]].y, s[e[1]].x, s[e[1]].y, col, thick);
    }

    constexpr float deg2rad          = 3.14159265358979323846f / 180.f;
    constexpr float radar_base_range = 15000.f;
    constexpr float nearby_range_m   = 50.f;

    struct yaw_frame {
        fvector origin;
        float   yaw;
        float   c, s;
    };

    inline yaw_frame make_yaw_frame(const sdk::view_info& v) {
        float r = v.rotation.yaw * deg2rad;
        return { v.location, v.rotation.yaw, cosf(r), sinf(r) };
    }

    inline void to_local(const yaw_frame& f, const fvector& w, float& fwd, float& right) {
        float dx = w.x - f.origin.x, dy = w.y - f.origin.y;
        fwd   = dx * f.c + dy * f.s;
        right = -dx * f.s + dy * f.c;
    }

    struct radar_frame {
        float x, y, size, cx, cy, scale;
    };

    inline radar_frame begin_radar(const settings::Config& cfg) {
        radar_frame r;
        r.x = cfg.radar_screen_x - (float)overlay::screen_x;
        r.y = cfg.radar_screen_y - (float)overlay::screen_y;
        r.size = cfg.radar_size;
        r.cx = r.x + r.size * 0.5f;
        r.cy = r.y + r.size * 0.5f;
        r.scale = (r.size * 0.5f) / (radar_base_range / fmaxf(cfg.radar_zoom, 0.1f));
        const uint8_t a = (uint8_t)(fminf(fmaxf(cfg.radar_alpha, 0.f), 1.f) * 255.f);
        overlay::draw::filled_box(r.x, r.y, r.size, r.size, overlay::draw::argb(a, 14, 14, 18));
        overlay::draw::box(r.x, r.y, r.size, r.size, overlay::draw::argb(a, 70, 70, 82), 1.f);
        const uint32_t grid = overlay::draw::argb((uint8_t)(a * 0.5f), 90, 90, 100);
        overlay::draw::line(r.cx, r.y, r.cx, r.y + r.size, grid, 1.f);
        overlay::draw::line(r.x, r.cy, r.x + r.size, r.cy, grid, 1.f);
        overlay::draw::filled_triangle(r.cx, r.cy - 6.f, r.cx - 4.5f, r.cy + 4.f, r.cx + 4.5f, r.cy + 4.f,
                                       overlay::draw::argb(255, 255, 255, 255));
        return r;
    }

    inline void radar_dot(const radar_frame& r, const yaw_frame& f, const fvector& loc, float enemy_yaw, bool lines, uint32_t col) {
        float fwd, right;
        to_local(f, loc, fwd, right);
        const float lo = 3.f, hi = r.size - 3.f;
        float px = fminf(fmaxf(r.cx + right * r.scale, r.x + lo), r.x + hi);
        float py = fminf(fmaxf(r.cy - fwd * r.scale, r.y + lo), r.y + hi);
        if (lines) {
            float rel = (enemy_yaw - f.yaw) * deg2rad;
            overlay::draw::line(px, py, px + sinf(rel) * 10.f, py - cosf(rel) * 10.f, col, 1.5f);
        }
        overlay::draw::filled_circle(px, py, 3.f, col);
    }

    inline void offscreen_arrow(const yaw_frame& f, const fvector& loc, uint32_t col) {
        float fwd, right;
        to_local(f, loc, fwd, right);
        float a = atan2f(right, fwd);
        float sx = sinf(a), sy = -cosf(a);
        float cx = overlay::width * 0.5f, cy = overlay::height * 0.5f;
        float R = fminf((float)overlay::width, (float)overlay::height) * 0.28f;
        float tx = cx + sx * (R + 14.f), ty = cy + sy * (R + 14.f);
        float bx = cx + sx * R, by = cy + sy * R;
        float px = -sy * 7.f, py = sx * 7.f;
        overlay::draw::filled_triangle(tx, ty, bx + px, by + py, bx - px, by - py, col);
    }

    inline void run(const cache::snapshot& snap) {
        draw_crosshair();

        const settings::Config& cfg = settings::cfg;
        const bool in_match     = !snap.ctx.lobby && snap.ctx.pawn;
        const bool want_radar   = cfg.radar;
        const bool want_arrows  = cfg.esp_enable && cfg.offscreen && in_match;
        const bool want_counter = cfg.enemy_counter && in_match;
        const bool want_loot    = !snap.ctx.lobby && (cfg.loot_chests || cfg.loot_ammo || cfg.loot_supply
                                                      || cfg.loot_floor || cfg.loot_vehicles || cfg.loot_llamas);

        overlay::set_font_size(cfg.font_size);
        overlay::draw::text_bg = cfg.text_bg;
        overlay::draw::text_outline = cfg.text_outline;

        settings::stats = {};
        radar_frame rf{};
        if (want_radar) rf = begin_radar(cfg);
        if (!cfg.esp_enable && !want_radar && !want_counter && !want_loot) return;

        const uint32_t text_col = to_argb(cfg.text_color);
        const uint32_t name_col = to_argb(cfg.name_color);
        const uint32_t knock_col = to_argb(cfg.knocked_text_color);
        const uint32_t arrow_col = to_argb(cfg.offscreen_color);
        const uint32_t radar_col = to_argb(cfg.radar_color);
        const uint32_t weapon_col = to_argb(cfg.weapon_color);

        static std::vector<sdk::mesh_state> states;
        static std::vector<uint8_t>         state_ok;
        states.resize(snap.pawns.size());
        state_ok.assign(snap.pawns.size(), 0);
        for (size_t i = 0; i < snap.pawns.size(); i++)
            state_ok[i] = sdk::read_mesh_state(snap.pawns[i].mesh, states[i]) ? 1 : 0;

        sdk::view_info view;
        if (!current_view(snap.ctx, view)) return;

        const int local_team = (!snap.ctx.lobby && snap.ctx.pawn) ? sdk::pawn_team(snap.ctx.pawn) : -1;

        sdk::projector proj = sdk::make_projector(view, overlay::width, overlay::height);
        const yaw_frame yf = make_yaw_frame(view);
        int enemy_count = 0, nearby_count = 0;

        if (want_loot) draw_loot(cfg, proj, view);

        ftransform bones[sdk::bone::count];
        for (size_t i = 0; i < snap.pawns.size(); i++) {
            if (!state_ok[i]) continue;
            const sdk::mesh_state& st = states[i];
            fvector loc = st.c2w.translation;
            if (!sdk::finite3(loc) || (loc.x == 0.f && loc.y == 0.f && loc.z == 0.f)) continue;

            float dist_m = loc.distance(view.location) * 0.01f;

            const int team = (!snap.ctx.lobby) ? sdk::pawn_team(snap.pawns[i].addr) : -1;
            const bool teammate = (local_team >= 0 && team >= 0 && team == local_team);
            const bool knocked = cfg.knocked_flag && snap.pawns[i].knocked;
            if (want_counter && !teammate) {
                enemy_count++;
                if (dist_m <= nearby_range_m) nearby_count++;
            }
            if (teammate ? !cfg.show_teammates : !cfg.show_enemies) continue;

            if (want_radar) {
                uint32_t c = knocked ? to_argb(cfg.knocked_color) : (teammate ? to_argb(cfg.team_color) : radar_col);
                float enemy_yaw = quat_to_rotator(st.c2w.rotation).yaw + 90.f;
                radar_dot(rf, yf, loc, enemy_yaw, cfg.radar_lines, c);
            }

            if (!cfg.esp_enable) continue;
            if (dist_m > cfg.max_distance) continue;

            if (want_arrows) {
                fvector2d mid;
                bool on = sdk::project(proj, { loc.x, loc.y, loc.z + 90.f }, mid) && sdk::on_screen(proj, mid);
                if (!on) offscreen_arrow(yf, loc, knocked ? to_argb(cfg.knocked_color) : arrow_col);
            }

            if ((loc - proj.location).dot(proj.axis_x) < -300.f) continue;

            const bool player_visible = st.recently_rendered();
            if (cfg.hide_hidden && !player_visible) continue;

            settings::Color base_box, base_skel;
            if (knocked) {
                base_box = base_skel = cfg.knocked_color;
            } else if (teammate) {
                base_box = base_skel = cfg.team_color;
            } else if (cfg.visible_check) {
                base_box = base_skel = player_visible ? cfg.visible_color : cfg.hidden_color;
            } else {
                base_box = cfg.box_color;
                base_skel = cfg.skel_color;
            }
            const float fa = cfg.fade ? fade_alpha(dist_m, cfg.max_distance) : 1.f;
            const uint32_t box_col  = to_argb(base_box, fa);
            const uint32_t skel_col = to_argb(base_skel, fa);

            fvector2d scr[sdk::bone::count];
            bool      vis[sdk::bone::count] = {};
            screen_bounds bb;
            fvector wmin{ FLT_MAX, FLT_MAX, FLT_MAX }, wmax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

            bool have_bones = sdk::read_bones(st, bones);
            if (have_bones) {
                fmatrix c2w = transform_to_matrix_with_scale(st.c2w);
                fvector world_head{}, world_neck{};
                for (int b : used_bones) {
                    fvector w = sdk::bone_to_world(bones[b], c2w);
                    if (!sdk::finite3(w)) continue;
                    if (b == sdk::bone::head) world_head = w;
                    if (b == sdk::bone::neck) world_neck = w;
                    wmin.x = fminf(wmin.x, w.x); wmin.y = fminf(wmin.y, w.y); wmin.z = fminf(wmin.z, w.z);
                    wmax.x = fmaxf(wmax.x, w.x); wmax.y = fmaxf(wmax.y, w.y); wmax.z = fmaxf(wmax.z, w.z);
                    fvector2d s;
                    if (!sdk::project(proj, w, s)) continue;
                    scr[b] = s;
                    vis[b] = true;
                    bb.add(s);
                }
                if (vis[sdk::bone::head]) {
                    fvector up = world_head - world_neck;
                    float len = up.length();
                    fvector top = len > 1.f ? world_head + up * (head_top_offset / len)
                                            : world_head + fvector{ 0.f, 0.f, head_top_offset };
                    fvector2d s;
                    if (sdk::project(proj, top, s)) bb.add(s);
                }
            }

            if (bb.points < 2) {
                bb = screen_bounds{};
                fvector2d foot_s, head_s;
                if (!sdk::project(proj, loc, foot_s)) continue;
                if (!sdk::project(proj, { loc.x, loc.y, loc.z + fallback_height }, head_s)) continue;
                float fh = fabsf(foot_s.y - head_s.y);
                float fw = fh * 0.48f;
                float cx = (foot_s.x + head_s.x) * 0.5f;
                bb.add({ cx - fw * 0.5f, head_s.y });
                bb.add({ cx + fw * 0.5f, foot_s.y });
            }

            const box_rect br = box_from_bounds(bb);
            if (br.w < 2.f && br.h < 4.f) continue;
            if (br.h > overlay::height * 3.f || br.w > overlay::width * 3.f) continue;

            if (cfg.box) {
                if (cfg.box_filled)
                    overlay::draw::filled_box(br.x, br.y, br.w, br.h, to_argb(cfg.fill_color, cfg.fill_opacity * fa));
                if (cfg.box_3d && have_bones && wmin.x < wmax.x) {
                    fvector mn = { wmin.x - 18.f, wmin.y - 18.f, wmin.z - 4.f };
                    fvector mx = { wmax.x + 18.f, wmax.y + 18.f, wmax.z + 10.f };
                    draw_box_3d(proj, mn, mx, box_col, cfg.box_thickness);
                } else if (cfg.box_corner) {
                    seg2d segs[8];
                    int n = corner_segments(br, cfg.corner_pct, segs);
                    for (int k = 0; k < n; k++)
                        overlay::draw::line(segs[k].x0, segs[k].y0, segs[k].x1, segs[k].y1, box_col, cfg.box_thickness);
                } else {
                    overlay::draw::box(br.x, br.y, br.w, br.h, box_col, cfg.box_thickness);
                }
            }

            if (cfg.snaplines) {
                float fx = br.x + br.w * 0.5f;
                float fy = br.y + br.h;
                overlay::draw::line(overlay::width * 0.5f, (float)overlay::height, fx, fy, box_col, 1.f);
            }

            if (cfg.skeleton && have_bones) {
                for (const auto& line : skeleton_lines) {
                    if (!vis[line.from] || !vis[line.to]) continue;
                    overlay::draw::line(scr[line.from].x, scr[line.from].y, scr[line.to].x, scr[line.to].y, skel_col, cfg.skel_thickness);
                }
            }

            if (cfg.head_circle && vis[sdk::bone::head]) {
                float radius = vis[sdk::bone::neck]
                    ? fmaxf(4.f, (scr[sdk::bone::head] - scr[sdk::bone::neck]).length() * 0.6f)
                    : fmaxf(4.f, br.w * 0.18f);
                overlay::draw::circle(scr[sdk::bone::head].x, scr[sdk::bone::head].y, radius, skel_col, cfg.skel_thickness);
            }

            if (cfg.distance) {
                wchar_t label[32];
                swprintf(label, 32, L"%.0fm", dist_m);
                overlay::draw::text(br.x + br.w + 4.f, br.y, label, with_alpha(text_col, fa));
            }

            const float line_h = overlay::font_size + 2.f;
            if (cfg.names && snap.pawns[i].name[0])
                box_label(br, br.y - line_h - 3.f, snap.pawns[i].name, with_alpha(name_col, fa), cfg);

            float below = br.y + br.h + 2.f;
            if (cfg.weapon && snap.pawns[i].weapon[0]) {
                box_label(br, below, snap.pawns[i].weapon, with_alpha(rarity_argb(snap.pawns[i].rarity, weapon_col), fa), cfg);
                below += line_h;
            }

            if (knocked)
                box_label(br, below, L"KNOCKED", with_alpha(knock_col, fa), cfg);
        }

        settings::stats = { enemy_count, nearby_count };
    }
}
