
#include "pages.h"
#include "ui/theme.h"
#include "ui/widgets.h"
#include "../settings/settings.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "../features/esp/esp_shared.h"
#include "../features/aim/aim.h"

#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace pages {

using namespace ui;

struct Row { bool on; ImVec4 col; };
static ImVec4 Rgb(int r, int g, int b) { return ImColor(r, g, b); }

namespace players {
    static Row enable{ true, {} }, box{ false, Rgb(255, 255, 255) }, corner{ false, {} }, filled{ false, Rgb(108, 149, 95) }, box3d{ false, {} };
    static Row skeleton{ true, Rgb(255, 255, 255) }, snap{ false, Rgb(255, 255, 255) }, outline{ true, {} };
    static Row enemies{ true, {} }, team{ false, {} }, lobby{ true, {} }, hide_walls{ false, {} };
    static float render_distance = 300.f;
    static Row health{ true, Rgb(94, 216, 73) }, shield{ false, Rgb(86, 140, 251) }, distance{ true, {} }, arrows{ true, Rgb(192, 36, 20) };
}
namespace info {
    static Row name{ false, Rgb(255, 255, 255) }, platform{ true, Rgb(86, 140, 251) }, distance{ true, Rgb(255, 255, 255) }, weapon{ true, Rgb(255, 255, 255) }, ammo{ true, {} }, rank{ false, Rgb(255, 196, 64) }, level{ false, {} };
    static float font_size = 13.f, background = 35.f;
    static Row text_outline{ true, {} }, shorten{ false, {} };
    static int align = 1;
    static Row visible{ true, Rgb(94, 216, 73) }, knocked{ true, Rgb(255, 196, 64) }, reloading{ false, {} }, bot{ false, {} };
}
namespace world {
    static Row chests{ true, Rgb(255, 196, 64) }, ammo{ false, Rgb(120, 180, 120) }, drops{ true, Rgb(86, 140, 251) }, floor_loot{ false, {} }, vehicles{ false, Rgb(255, 255, 255) }, llamas{ true, Rgb(190, 120, 255) };
    static int rarity = 2;
    static float loot_distance = 300.f;
}
namespace style {
    static int box_type = 0;
    static float thick = 1.5f, corner = 25.f, fill = 20.f;
    static Row glow{ false, {} }, fade{ true, {} };
    static float skel_thick = 1.f;
    static Row head{ true, {} }, color_by_vis{ true, {} };
    static ImVec4 c_visible = Rgb(94, 216, 73), c_hidden = Rgb(192, 36, 20), c_knocked = Rgb(255, 196, 64), c_team = Rgb(86, 140, 251), c_text = Rgb(255, 255, 255);
}
namespace aim {
    static Row enable{ false, {} }, draw_fov{ true, Rgb(255, 255, 255) }, sticky{ false, {} };
    static int key = ImGuiKey_MouseRight, bone = 0;
    static float fov = 120.f, smooth = 6.f;
    static Row visible{ true, {} }, knocked{ true, {} }, bots{ false, {} }, team{ false, {} };
    static float max_dist = 250.f;
    static Row predict{ true, {} }, drop{ true, {} };
    static float strength = 60.f;
    static Row trigger{ false, {} }, trigger_head{ false, {} };
    static int trigger_key = ImGuiKey_MouseX1;
    static float delay = 40.f, hold = 80.f, chance = 90.f;
    static Row w_ar{ true, {} }, w_smg{ true, {} }, w_shotgun{ true, {} }, w_sniper{ false, {} }, w_pistol{ false, {} };
}
namespace misc {
    static Row stream{ false, {} }, tray{ true, {} };
    static int menu_key = ImGuiKey_Insert, panic_key = ImGuiKey_End;
    static Row r_enable{ true, Rgb(192, 36, 20) }, r_names{ false, {} }, r_lines{ true, {} };
    static float r_size = 230.f, r_zoom = 1.5f, r_alpha = 80.f;
    static Row c_enable{ false, Rgb(255, 255, 255) }, c_dot{ true, {} };
    static float c_size = 8.f, c_gap = 3.f;
}

struct Config { std::string name; };
static std::vector<Config> g_configs;
static int  g_active = 0;
static char g_new_name[32] = "";
static float g_unload_until = -1.f;
static float g_saved_until = -1.f;
static int g_saved_row = -1;

void Init(State& s)
{
    g_configs.clear();
    g_configs.push_back({ "nice" });
    g_configs.push_back({ "default" });
    g_active = 0;
    (void)s;
}

static void ApplyTheme(State& s)
{
    theme.pal = (s.theme == 0) ? PaletteDark() : PaletteBlack();
    if (s.ref == 1) { theme.accent = theme.accent_green; return; }
    if (s.rainbow_accent)
    {
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(fmodf(static_cast<float>(ImGui::GetTime()) * theme.rainbow_speed, 1.f), theme.rainbow_sat, theme.rainbow_val, r, g, b);
        theme.accent = ImVec4(r, g, b, 1.f);
    }
    else if (s.custom_accent)
        theme.accent = s.custom_color;
    else
        theme.accent = theme.accent_red;
}

static void DrawSidebar(State& s, const Textures& tex)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    TextAt(dl, fonts::f15, ImVec2(P(L.logo_pos.x), P(L.title_pos.y)), U32(theme.accent), "fanta");
    const float brand_w = TextSize(fonts::f15, "fanta").x;
    TextAt(dl, fonts::f15, ImVec2(P(L.logo_pos.x) + brand_w, P(L.title_pos.y)), U32(C.title), ".lol");
    TextAt(dl, fonts::m12, P(L.version_pos), U32(C.faint), "v1.0");

    TextAt(dl, fonts::i12, P(L.search_icon), U32(C.placeholder), icon::search);
    (void)tex;
    if (SearchInput(s.search, IM_ARRAYSIZE(s.search))) {}
    SetFilter(s.search);
    FillAA(dl, ImVec2(P(L.underline_x0), P(L.underline_y)), ImVec2(P(L.underline_x1), P(L.underline_y) + 1.f), U32(C.line));

    static const struct { const char* name; const char** ic; } items[4] = {
        { "Visuals", &icon::eye }, { "Aimbot", &icon::crosshair }, { "Misc", &icon::sliders }, { "Settings", &icon::gear } };

    const float pitch = (Snapped() ? floorf(PF(L.nav_pitch) + 0.5f) : PF(L.nav_pitch));
    const float nav_y0 = P(L.nav_y0), nav_h = P(L.nav_h);
    AnimState& pill = GetAnim(ImGui::GetID("##navpill"));
    if (!pill.initialized) { pill.f[0] = static_cast<float>(s.page); pill.initialized = true; }
    Approach(pill.f[0], static_cast<float>(s.page), theme.motion.nav);
    const float pill_y = nav_y0 + pill.f[0] * pitch;
    dl->AddRectFilled(ImVec2(P(L.nav_x0), pill_y), ImVec2(P(L.nav_x1), pill_y + nav_h), U32(C.nav_active), PF(L.nav_rounding));

    for (int i = 0; i < 4; ++i)
    {
        const float y = nav_y0 + static_cast<float>(i) * pitch;
        ImGui::PushID(i);
        bool hov = false;
        if (Hotspot("##nav", ImVec2(P(L.nav_x0), y), ImVec2(P(L.nav_x1), y + nav_h), &hov))
            s.page = i;
        AnimState& st = GetAnim(ImGui::GetID("##navanim"));
        if (!st.initialized) { st.f[0] = (s.page == i) ? 1.f : 0.f; st.initialized = true; }
        Approach(st.f[0], (s.page == i) ? 1.f : 0.f, theme.motion.speed);
        Approach(st.f[1], hov ? 1.f : 0.f, theme.motion.speed);
        if (s.page != i && st.f[1] > 0.f)
            dl->AddRectFilled(ImVec2(P(L.nav_x0), y), ImVec2(P(L.nav_x1), y + nav_h), U32(C.nav_hover, st.f[1]), PF(L.nav_rounding));
        const ImVec4 tc = Mix(Mix(C.text_dim, C.text, st.f[1] * 0.45f), C.text, st.f[0]);
        const ImVec4 ic = Mix(Mix(C.text_dim, C.text, st.f[1] * 0.45f), theme.accent, st.f[0]);
        TextAt(dl, fonts::i15, ImVec2(P(L.nav_icon_x), y + P(i == 0 ? L.nav_eye_dy : L.nav_icon_dy)), U32(ic), *items[i].ic);
        TextAt(dl, fonts::f14, ImVec2(P(L.nav_text_x), y + P(L.nav_text_dy)), U32(tc), items[i].name);
        ImGui::PopID();
    }

    dl->AddLine(ImVec2(P(L.divider_x), P(L.divider_y0)), ImVec2(P(L.divider_x), P(L.divider_y1)), U32(C.border), 1.f);
}

static void DrawHeader(State& s, ImVec2 win)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    static const char* const page_names[4] = { "Visuals", "Aimbot", "Misc", "Settings" };
    static const char* const subs[4][4] = {
        { "Players", "Info", "World", "Style" }, { "General", "Trigger", nullptr, nullptr },
        { "General", "Radar", nullptr, nullptr }, { "Config Manager", nullptr, nullptr, nullptr } };

    ImFont* f = fonts::f14;
    const float y = P(L.head_y);
    float x = P(L.head_x);
    TextAt(dl, f, ImVec2(x, y), U32(C.text), page_names[s.page]);
    x += TextSize(f, page_names[s.page]).x + PF(L.head_sep_before);
    const float sx = TextX(x);
    FillAA(dl, ImVec2(sx, P(L.head_sep_y0)), ImVec2(sx + 1.f, P(L.head_sep_y1)), U32(C.line));
    x = TextX(sx + PF(L.head_sep_after));

    int& sub = s.sub[s.page];
    for (int i = 0; i < 4 && subs[s.page][i]; ++i)
    {
        const char* name = subs[s.page][i];
        const ImVec2 ts = TextSize(f, name);
        ImGui::PushID(100 + i);
        bool hov = false;
        const float slop = P(L.hit_slop);
        if (Hotspot("##sub", ImVec2(x - slop, y - slop), ImVec2(x + ts.x + slop, y + ts.y + slop), &hov))
            sub = i;
        AnimState& st = GetAnim(ImGui::GetID("##subanim"));
        if (!st.initialized) { st.f[0] = (sub == i) ? 1.f : 0.f; st.initialized = true; }
        Approach(st.f[0], (sub == i) ? 1.f : 0.f, theme.motion.speed);
        Approach(st.f[1], hov ? 1.f : 0.f, theme.motion.speed);
        TextAt(dl, f, ImVec2(x, y), U32(Mix(Mix(C.text_dim, C.text, st.f[1] * 0.45f), C.text, st.f[0])), name);
        ImGui::PopID();
        x = TextX(x + ts.x + PF(L.subtab_gap));
    }

    const float hit = P(L.winbtn_hit), mid = P(L.winbtn_center);
    {
        const ImVec2 p = P(L.min_pos), c = p + ImVec2(mid, mid);
        bool hov = false;
        if (Hotspot("##min", c - ImVec2(hit, hit), c + ImVec2(hit, hit), &hov)) s.request_minimize = true;
        TextAt(dl, fonts::i09, p, U32(hov ? C.text : C.win_button), icon::minus);
    }
    {
        const ImVec2 p = P(L.close_pos), c = p + ImVec2(mid, mid);
        bool hov = false;
        if (Hotspot("##close", c - ImVec2(hit, hit), c + ImVec2(hit, hit), &hov)) s.request_close = true;
        TextAt(dl, fonts::i09, p, U32(hov ? C.text : C.win_button), icon::close);
    }
    (void)win;
}

static void Check(const char* label, Row& r, bool swatch = false) { CheckRow(label, &r.on, swatch ? &r.col : nullptr); }

static void PagePlayers(State& s)
{
    using namespace players;
    const Layout& L = theme.lay;
    BeginPanel({ "Player ESP", icon::eye, true, false, L.eye_title_dx }, L.left_x0, L.left_x1, P(L.panel_top), 9);
    Check("Enable ESP", enable);
    Check("Box", box, true);
    Check("Corner Box", corner);
    Check("Filled Box", filled, true);
    Check("3D Box", box3d);
    Check("Skeleton", skeleton, true);
    Check("Snaplines", snap);
    Check("Outline", outline);
    CheckRow("ESP Preview", &s.preview);
    EndPanel();

    BeginPanel({ "Filters", icon::user, false, false }, L.right_x0, L.right_x1, P(L.panel_top), 5);
    Check("Enemies", enemies);
    Check("Teammates", team);
    Check("Hide Behind Walls", hide_walls);
    Check("Lobby ESP", lobby);
    SliderRow("Render Distance", &render_distance, 50.f, 500.f, "%.0fm");
    const float y1 = EndPanel();

    BeginPanel({ "Bars", icon::sliders, false, false }, L.right_x0, L.right_x1, y1 + P(L.panel_gap), 4);
    Check("Health Bar", health, true);
    Check("Shield Bar", shield, true);
    Check("Distance", distance);
    Check("Off-Screen Arrows", arrows, true);
    EndPanel();
}

static void PageInfo()
{
    using namespace info;
    const Layout& L = theme.lay;
    BeginPanel({ "Player Info", icon::idcard, true, false }, L.left_x0, L.left_x1, P(L.panel_top), 7);
    Check("Username", name, true);
    Check("Platform", platform, true);
    Check("Distance", distance, true);
    Check("Weapon", weapon, true);
    Check("Ammo Type", ammo);
    Check("Rank", rank, true);
    Check("Level", level);
    EndPanel();

    static const char* const aligns[] = { "Left", "Center", "Right" };
    BeginPanel({ "Text", icon::text_aa, false, false }, L.right_x0, L.right_x1, P(L.panel_top), 5);
    SliderRow("Font Size", &font_size, 9.f, 20.f, "%.0f");
    SliderRow("Background", &background, 0.f, 100.f, "%.0f%%");
    ComboRow("Alignment", &align, aligns, 3);
    Check("Text Outline", text_outline);
    Check("Shorten Names", shorten);
    const float y1 = EndPanel();

    BeginPanel({ "Flags", icon::tag, false, false }, L.right_x0, L.right_x1, y1 + P(L.panel_gap), 4);
    Check("Visible", visible, true);
    Check("Knocked", knocked, true);
    Check("Reloading", reloading);
    Check("Bot", bot);
    EndPanel();
}

static void PageWorld()
{
    using namespace world;
    const Layout& L = theme.lay;
    static const char* const rarities[] = { "All", "Uncommon", "Rare", "Epic", "Legendary" };
    BeginPanel({ "Loot", icon::package, true, false }, L.left_x0, L.left_x1, P(L.panel_top), 8);
    Check("Chests", chests, true);
    Check("Ammo Boxes", ammo, true);
    Check("Supply Drops", drops, true);
    Check("Floor Loot", floor_loot);
    Check("Vehicles", vehicles, true);
    Check("Llamas", llamas, true);
    ComboRow("Min. Rarity", &rarity, rarities, 5);
    SliderRow("Loot Distance", &loot_distance, 50.f, 500.f, "%.0fm");
    EndPanel();
}

static void PageStyle()
{
    using namespace style;
    const Layout& L = theme.lay;
    static const char* const types[] = { "Full", "Cornered", "Curved", "3D" };
    BeginPanel({ "Box Style", icon::box, false, false }, L.left_x0, L.left_x1, P(L.panel_top), 6);
    ComboRow("Box Type", &box_type, types, 4);
    SliderRow("Thickness", &thick, 1.f, 4.f, "%.1f");
    SliderRow("Corner Length", &corner, 5.f, 50.f, "%.0f%%");
    SliderRow("Fill Opacity", &fill, 0.f, 100.f, "%.0f%%");
    Check("Glow", glow);
    Check("Fade By Distance", fade);
    const float y1 = EndPanel();

    BeginPanel({ "Skeleton", icon::person, false, false }, L.left_x0, L.left_x1, y1 + P(L.panel_gap), 2);
    SliderRow("Thickness", &skel_thick, 1.f, 3.f, "%.1f");
    Check("Head Circle", head);
    EndPanel();

    BeginPanel({ "Colors", icon::palette, true, false }, L.right_x0, L.right_x1, P(L.panel_top), 6);
    Check("Visible Check", color_by_vis);
    SwatchRow("Visible", &c_visible);
    SwatchRow("Hidden", &c_hidden);
    SwatchRow("Knocked", &c_knocked);
    SwatchRow("Team", &c_team);
    SwatchRow("Text", &c_text);
    EndPanel();
}

static void PageAimGeneral()
{
    using namespace aim;
    const Layout& L = theme.lay;
    static const char* const bones[] = { "Head", "Neck", "Chest", "Nearest" };
    BeginPanel({ "Aimbot", icon::crosshair, true, false }, L.left_x0, L.left_x1, P(L.panel_top), 7);
    Check("Enable Aimbot", enable);
    KeyRow("Aim Key", &key);
    ComboRow("Target Bone", &bone, bones, 4);
    SliderRow("FOV", &fov, 10.f, 360.f, "%.0f");
    SliderRow("Smoothing", &smooth, 1.f, 20.f, "%.1f");
    Check("Draw FOV", draw_fov, true);
    Check("Sticky Target", sticky);
    EndPanel();

    BeginPanel({ "Targets", icon::users, false, false }, L.right_x0, L.right_x1, P(L.panel_top), 5);
    Check("Visible Only", visible);
    Check("Skip Knocked", knocked);
    Check("Skip Bots", bots);
    Check("Teammates", team);
    SliderRow("Max Distance", &max_dist, 25.f, 500.f, "%.0fm");
    const float y1 = EndPanel();

    BeginPanel({ "Prediction", icon::lightning, false, false }, L.right_x0, L.right_x1, y1 + P(L.panel_gap), 3);
    Check("Prediction", predict);
    Check("Bullet Drop", drop);
    SliderRow("Strength", &strength, 0.f, 100.f, "%.0f%%");
    EndPanel();
}

static void PageAimTrigger()
{
    using namespace aim;
    const Layout& L = theme.lay;
    BeginPanel({ "Triggerbot", icon::click, true, false }, L.left_x0, L.left_x1, P(L.panel_top), 6);
    Check("Enable Trigger", trigger);
    KeyRow("Trigger Key", &trigger_key);
    SliderRow("First Delay", &delay, 0.f, 300.f, "%.0fms");
    SliderRow("Hold Time", &hold, 10.f, 300.f, "%.0fms");
    SliderRow("Hit Chance", &chance, 0.f, 100.f, "%.0f%%");
    Check("Head Only", trigger_head);
    EndPanel();

    BeginPanel({ "Weapons", icon::sword, false, false }, L.right_x0, L.right_x1, P(L.panel_top), 5);
    Check("Assault Rifles", w_ar);
    Check("SMGs", w_smg);
    Check("Shotguns", w_shotgun);
    Check("Snipers", w_sniper);
    Check("Pistols", w_pistol);
    EndPanel();
}

static void PageMiscGeneral(State& s)
{
    using namespace misc;
    const Layout& L = theme.lay;
    BeginPanel({ "Overlay", icon::monitor, true, false }, L.left_x0, L.left_x1, P(L.panel_top), 6);
    CheckRow("Watermark", &s.watermark);
    CheckRow("FPS Counter", &s.pill_fps);
    CheckRow("Clock", &s.pill_clock);
    CheckRow("Enemy Counter", &s.pill_enemies);
    Check("Stream Proof", stream);
    Check("Minimise To Tray", tray);
    EndPanel();

    BeginPanel({ "Hotkeys", icon::keyboard, false, false }, L.right_x0, L.right_x1, P(L.panel_top), 2);
    KeyRow("Menu Key", &menu_key);
    KeyRow("Panic Key", &panic_key);
    EndPanel();
}

static void PageMiscRadar()
{
    using namespace misc;
    const Layout& L = theme.lay;
    BeginPanel({ "Radar", icon::compass, true, false }, L.left_x0, L.left_x1, P(L.panel_top), 6);
    Check("Enable Radar", r_enable, true);
    Check("Player Names", r_names);
    Check("Look Lines", r_lines);
    SliderRow("Size", &r_size, 120.f, 400.f, "%.0f");
    SliderRow("Zoom", &r_zoom, 0.5f, 4.f, "%.1f");
    SliderRow("Opacity", &r_alpha, 10.f, 100.f, "%.0f%%");
    EndPanel();

    BeginPanel({ "Crosshair", icon::crosshair, false, false }, L.right_x0, L.right_x1, P(L.panel_top), 4);
    Check("Enable Crosshair", c_enable, true);
    Check("Center Dot", c_dot);
    SliderRow("Size", &c_size, 2.f, 24.f, "%.0f");
    SliderRow("Gap", &c_gap, 0.f, 12.f, "%.0f");
    EndPanel();
}

static void PageSettings(State& s)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;

    BeginPanel({ "Display", nullptr, true, true }, L.left_x0, L.left_x1, P(L.panel_top), 4);
    SliderRow("UI Scale", &s.ui_scale, 60.f, 150.f, "%.0f%%");
    if (!ImGui::IsItemActive() && !ImGui::IsMouseDown(0) && s.ref == 0)
    {
        const float snapped = floorf(s.ui_scale + 0.5f);
        if (snapped != s.applied_scale) { s.ui_scale = snapped; s.applied_scale = snapped; }
    }
    static const char* const themes[] = { "Dark", "Black" };
    ComboRow("Theme", &s.theme, themes, 2);
    if (CheckRow("Custom Accent", &s.custom_accent, s.custom_accent ? &s.custom_color : nullptr) && s.custom_accent) s.rainbow_accent = false;
    if (CheckRow("Rainbow Accent", &s.rainbow_accent) && s.rainbow_accent) s.custom_accent = false;
    float y1 = EndPanel();

    BeginPanel({ "Manage", nullptr, false, true }, L.left_x0, L.left_x1, y1 + P(L.panel_gap), 4);
    InputRow("Name", g_new_name, IM_ARRAYSIZE(g_new_name));
    if (ButtonRow("New from current", "Create") && g_new_name[0])
    {
        g_configs.push_back({ g_new_name });
        g_active = static_cast<int>(g_configs.size()) - 1;
        g_new_name[0] = 0;
    }
    if (ButtonRow("Rename active", "Rename") && g_new_name[0] && !g_configs.empty())
    {
        g_configs[g_active].name = g_new_name;
        g_new_name[0] = 0;
    }
    const float now = static_cast<float>(ImGui::GetTime());
    if (ButtonRow("Unload Cheat", now < g_unload_until ? "Unloaded###ul" : "Unload###ul"))
        g_unload_until = now + 1.5f;
    EndPanel();

    const int rows = ImMax(1, static_cast<int>(g_configs.size()));
    BeginPanel({ "Configs", nullptr, false, true }, L.right_x0, L.right_x1, P(L.panel_top), rows);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float row0 = P(L.panel_top) + P(L.plain_row0);
    int erase = -1, copy = -1;
    for (int i = 0; i < static_cast<int>(g_configs.size()); ++i)
    {
        const float y = RowY(row0, i);
        const bool active = (i == g_active);
        const float x = P(L.right_x0) + P(L.panel_pad);
        const char* name = g_configs[i].name.c_str();
        TextAt(dl, fonts::f14, ImVec2(x, y), U32(active ? C.text : C.text_dim), name);
        if (active)
            TextAt(dl, fonts::f13, ImVec2(x + TextSize(fonts::f14, name).x + P(L.active_gap), y), U32(theme.accent), "active");
        ImGui::PushID(i);
        const float fy = y - P(L.frame_dy), bw = P(L.config_button_w), bh = P(L.frame_h);
        const bool saved = (g_saved_row == i && now < g_saved_until);
        if (Button("##a", active ? (saved ? "Saved###a" : "Save###a") : "Load###a", P(L.config_x[0]), fy, bw, bh))
        {
            if (active) { g_saved_row = i; g_saved_until = now + 1.2f; }
            else g_active = i;
        }
        if (Button("##c", "Copy", P(L.config_x[1]), fy, bw, bh)) copy = i;
        if (Button("##d", "Delete", P(L.config_x[2]), fy, bw, bh)) erase = i;
        ImGui::PopID();
    }
    if (copy >= 0)
    {
        std::string n = g_configs[copy].name + " copy";
        g_configs.insert(g_configs.begin() + copy + 1, { n });
        if (g_active > copy) ++g_active;
    }
    if (erase >= 0)
    {
        g_configs.erase(g_configs.begin() + erase);
        if (g_active >= erase && g_active > 0) --g_active;
    }
    EndPanel();
}

static settings::Color ToColor(const ImVec4& c) { return { c.x, c.y, c.z }; }

static void SyncToConfig(const State& s)
{
    settings::Config& o = settings::cfg;
    o.esp_enable     = players::enable.on;
    o.visible_check  = style::color_by_vis.on;
    o.box            = players::box.on;
    o.box_color      = ToColor(players::box.col);
    o.skel_color     = ToColor(players::skeleton.col);
    o.fill_color     = ToColor(players::filled.col);
    o.box_corner     = players::corner.on;
    o.box_filled     = players::filled.on;
    o.box_3d         = players::box3d.on;
    o.skeleton       = players::skeleton.on;
    o.snaplines      = players::snap.on;
    o.head_circle    = style::head.on;
    o.distance       = players::distance.on;
    o.names          = info::name.on;
    o.weapon         = info::weapon.on;
    o.weapon_color   = ToColor(info::weapon.col);
    o.name_color     = ToColor(info::name.col);
    o.hide_hidden    = players::hide_walls.on;
    o.show_enemies   = players::enemies.on;
    o.show_teammates = players::team.on;
    o.lobby_esp      = players::lobby.on;
    o.visible_color  = ToColor(style::c_visible);
    o.hidden_color   = ToColor(style::c_hidden);
    o.team_color     = ToColor(style::c_team);
    o.text_color     = ToColor(style::c_text);
    o.max_distance   = players::render_distance;
    o.box_thickness  = style::thick;
    o.skel_thickness = style::skel_thick;
    o.fill_opacity   = style::fill / 100.f;
    o.corner_pct     = style::corner / 100.f;
    o.crosshair       = misc::c_enable.on;
    o.crosshair_dot   = misc::c_dot.on;
    o.crosshair_size  = misc::c_size;
    o.crosshair_gap   = misc::c_gap;
    o.crosshair_color = ToColor(misc::c_enable.col);
    o.knocked_flag       = info::knocked.on;
    o.knocked_color      = ToColor(style::c_knocked);
    o.knocked_text_color = ToColor(info::knocked.col);
    o.offscreen          = players::arrows.on;
    o.offscreen_color    = ToColor(players::arrows.col);
    o.radar              = misc::r_enable.on;
    o.radar_color        = ToColor(misc::r_enable.col);
    o.radar_lines        = misc::r_lines.on;
    o.radar_size         = misc::r_size;
    o.radar_zoom         = misc::r_zoom;
    o.radar_alpha        = misc::r_alpha / 100.f;
    o.enemy_counter      = s.watermark && s.pill_enemies;
    o.fade               = style::fade.on;
    o.font_size          = info::font_size;
    o.text_bg            = info::background / 100.f;
    o.text_align         = info::align;
    o.text_outline       = info::text_outline.on;
    o.shorten            = info::shorten.on;
    o.loot_chests        = world::chests.on;
    o.loot_ammo          = world::ammo.on;
    o.loot_supply        = world::drops.on;
    o.loot_floor         = world::floor_loot.on;
    o.loot_vehicles      = world::vehicles.on;
    o.loot_llamas        = world::llamas.on;
    o.loot_min_rarity    = world::rarity;
    o.loot_distance      = world::loot_distance;
    o.chest_color        = ToColor(world::chests.col);
    o.ammo_color         = ToColor(world::ammo.col);
    o.supply_color       = ToColor(world::drops.col);
    o.vehicle_color      = ToColor(world::vehicles.col);
    o.llama_color        = ToColor(world::llamas.col);
    o.radar_screen_x     = theme.lay.pill_screen.x;
    o.radar_screen_y     = theme.lay.pill_screen.y + (s.watermark ? P(theme.lay.pill_h) + P(theme.lay.radar_gap) : 0.f);
}

void Draw(State& s, const Textures& tex, ImVec2 win)
{
    ApplyTheme(s);
    SyncToConfig(s);
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* bg = ImGui::GetBackgroundDrawList();
    const float r = PF(L.win_rounding);
    bg->AddRectFilled(ImVec2(0, 0), win, U32(C.window), r);
    bg->AddRect(ImVec2(0, 0), win, U32(C.border), r, 0, 1.f);

    DrawSidebar(s, tex);
    DrawHeader(s, win);

    static int shown_page = -1, shown_sub = -1;
    static float fade = 1.f;
    if (shown_page < 0) { shown_page = s.page; shown_sub = s.sub[s.page]; }
    const bool same = (shown_page == s.page && shown_sub == s.sub[s.page]);
    Approach(fade, same ? 1.f : 0.f, theme.motion.page_fade);
    if (!same && fade < 0.02f) { shown_page = s.page; shown_sub = s.sub[s.page]; }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
    switch (shown_page)
    {
    case Visuals:
        switch (shown_sub) { case 0: PagePlayers(s); break; case 1: PageInfo(); break; case 2: PageWorld(); break; default: PageStyle(); break; }
        break;
    case Aimbot:
        if (shown_sub == 0) PageAimGeneral(); else PageAimTrigger();
        break;
    case Misc:
        if (shown_sub == 0) PageMiscGeneral(s); else PageMiscRadar();
        break;
    default:
        PageSettings(s);
        break;
    }
    ImGui::PopStyleVar();
}

namespace preview_rig {
    struct Joint { int bone; float x, y; };
    static const Joint joints[] = {
        { sdk::bone::root,       472.f, 828.f },
        { sdk::bone::pelvis,     452.f, 382.f },
        { sdk::bone::spine_01,   449.f, 330.f },
        { sdk::bone::spine_02,   444.f, 282.f },
        { sdk::bone::spine_03,   441.f, 235.f },
        { sdk::bone::neck,       439.f, 182.f },
        { sdk::bone::head,       439.f, 125.f },
        { sdk::bone::clavicle_l, 468.f, 200.f },
        { sdk::bone::upperarm_l, 505.f, 218.f },
        { sdk::bone::lowerarm_l, 555.f, 302.f },
        { sdk::bone::hand_l,     548.f, 350.f },
        { sdk::bone::clavicle_r, 412.f, 200.f },
        { sdk::bone::upperarm_r, 362.f, 218.f },
        { sdk::bone::lowerarm_r, 293.f, 292.f },
        { sdk::bone::hand_r,     335.f, 337.f },
        { sdk::bone::thigh_l,    488.f, 405.f },
        { sdk::bone::calf_l,     526.f, 565.f },
        { sdk::bone::foot_l,     550.f, 752.f },
        { sdk::bone::thigh_r,    418.f, 405.f },
        { sdk::bone::calf_r,     407.f, 578.f },
        { sdk::bone::foot_r,     396.f, 770.f },
    };
    constexpr float head_top_x = 439.f, head_top_y = 66.f;
    constexpr float crop_x0 = 230.f, crop_y0 = 40.f, crop_x1 = 670.f, crop_y1 = 860.f;
    constexpr float distance_m = 12.f;
}

bool PreviewVisible(const State& s)
{
    return s.preview && s.page == Visuals;
}

static ImU32 CfgColor(const settings::Color& c)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, 1.f));
}

static ImU32 CfgColorA(const settings::Color& c, float a)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, a));
}

void DrawPreview(const Textures& tex, const ImVec2& o, const ImVec2& sz)
{
    using namespace preview_rig;
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float r = PF(L.win_rounding);
    dl->AddRectFilled(o, o + sz, U32(C.window), r);
    dl->AddRect(o, o + sz, U32(C.border), r, 0, 1.f);
    TextAt(dl, fonts::i15, o + ImVec2(P(L.nav_icon_x), P(L.head_y) - P(1.f)), U32(theme.accent), icon::person);
    TextAt(dl, fonts::f14, o + ImVec2(P(L.nav_text_x), P(L.head_y)), U32(C.text), "ESP Preview");

    const ImVec2 p0 = o + ImVec2(P(L.panel_pad), P(L.panel_top));
    const ImVec2 p1 = o + sz - ImVec2(P(L.panel_pad), P(L.panel_gap));
    const float pr = PF(L.panel_rounding);
    dl->AddRectFilled(p0, p1, U32(C.panel), pr);
    dl->AddRect(p0, p1, U32(C.panel_border), pr, 0, 1.f);

    const float inset = P(12.f);
    const ImVec2 a0 = p0 + ImVec2(inset, inset), a1 = p1 - ImVec2(inset, inset);
    const float cw = crop_x1 - crop_x0, ch = crop_y1 - crop_y0;
    const float k = ImMin((a1.x - a0.x) / cw, (a1.y - a0.y) / ch);
    const ImVec2 isz(cw * k, ch * k);
    const ImVec2 i0(floorf((a0.x + a1.x - isz.x) * 0.5f), floorf((a0.y + a1.y - isz.y) * 0.5f));
    if (tex.preview && tex.preview_w > 0 && tex.preview_h > 0)
    {
        const float tw = static_cast<float>(tex.preview_w), th = static_cast<float>(tex.preview_h);
        dl->AddImage(tex.preview, i0, i0 + isz, ImVec2(crop_x0 / tw, crop_y0 / th), ImVec2(crop_x1 / tw, crop_y1 / th));
    }

    const settings::Config& cfg = settings::cfg;
    if (!cfg.esp_enable) return;

    auto map = [&](float x, float y) { return fvector2d(i0.x + (x - crop_x0) * k, i0.y + (y - crop_y0) * k); };
    fvector2d scr[sdk::bone::count];
    bool vis[sdk::bone::count] = {};
    esp::screen_bounds bb;
    for (const Joint& j : joints)
    {
        scr[j.bone] = map(j.x, j.y);
        vis[j.bone] = true;
        bb.add(scr[j.bone]);
    }
    bb.add(map(head_top_x, head_top_y));
    const esp::box_rect br = esp::box_from_bounds(bb);
    const settings::Color& box_base  = cfg.visible_check ? cfg.visible_color : cfg.box_color;
    const settings::Color& skel_base = cfg.visible_check ? cfg.visible_color : cfg.skel_color;
    const ImU32 box_c  = CfgColor(box_base);
    const ImU32 skel_c = CfgColor(skel_base);

    if (cfg.box)
    {
        const ImVec2 mn(br.x, br.y), mx(br.x + br.w, br.y + br.h);
        if (cfg.box_filled)
            dl->AddRectFilled(mn, mx, CfgColorA(cfg.fill_color, cfg.fill_opacity));
        if (cfg.box_corner && !cfg.box_3d)
        {
            esp::seg2d segs[8];
            int nseg = esp::corner_segments(br, cfg.corner_pct, segs);
            for (int i = 0; i < nseg; i++)
                dl->AddLine(ImVec2(segs[i].x0, segs[i].y0), ImVec2(segs[i].x1, segs[i].y1), box_c, cfg.box_thickness);
        }
        else
            dl->AddRect(mn, mx, box_c, 0.f, 0, cfg.box_thickness);
    }

    if (cfg.skeleton)
    {
        for (const esp::skel_pair& line : esp::skeleton_lines)
        {
            if (!vis[line.from] || !vis[line.to]) continue;
            dl->AddLine(ImVec2(scr[line.from].x, scr[line.from].y), ImVec2(scr[line.to].x, scr[line.to].y), skel_c, cfg.skel_thickness);
        }
    }

    if (cfg.head_circle && vis[sdk::bone::head])
    {
        float radius = fmaxf(4.f, (scr[sdk::bone::head] - scr[sdk::bone::neck]).length() * 0.6f);
        dl->AddCircle(ImVec2(scr[sdk::bone::head].x, scr[sdk::bone::head].y), radius, skel_c, 0, cfg.skel_thickness);
    }

    if (cfg.distance)
    {
        char label[16];
        ImFormatString(label, IM_ARRAYSIZE(label), "%.0fm", distance_m);
        const ImVec2 tp(br.x + br.w + 4.f, br.y);
        dl->AddText(fonts::f13, fonts::f13->FontSize, tp + ImVec2(1.f, 1.f), IM_COL32(0, 0, 0, 220), label);
        dl->AddText(fonts::f13, fonts::f13->FontSize, tp, CfgColor(cfg.text_color), label);
    }

    if (cfg.names)
    {
        const char* name = "Player";
        const ImVec2 ts = TextSize(fonts::f13, name);
        const ImVec2 np(floorf(br.x + (br.w - ts.x) * 0.5f), floorf(br.y - ts.y - 3.f));
        dl->AddText(fonts::f13, fonts::f13->FontSize, np + ImVec2(1.f, 1.f), IM_COL32(0, 0, 0, 220), name);
        dl->AddText(fonts::f13, fonts::f13->FontSize, np, CfgColor(cfg.name_color), name);
    }

    if (cfg.weapon)
    {
        const char* weapon = "Pump Shotgun";
        const ImVec2 ts = TextSize(fonts::f13, weapon);
        const ImVec2 wp(floorf(br.x + (br.w - ts.x) * 0.5f), floorf(br.y + br.h + 2.f));
        dl->AddText(fonts::f13, fonts::f13->FontSize, wp + ImVec2(1.f, 1.f), IM_COL32(0, 0, 0, 220), weapon);
        dl->AddText(fonts::f13, fonts::f13->FontSize, wp, IM_COL32(70, 160, 255, 255), weapon);
    }
}

void DrawPill(ImDrawList* dl, const ImVec2& o, const State& s, const Textures& tex, float time, bool frozen)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    const float w = PillWidth(s), h = P(L.pill_h), r = PF(L.pill_rounding);
    dl->AddRectFilled(o, o + ImVec2(w, h), U32(C.window, C.pill_alpha), r);
    dl->AddRect(o, o + ImVec2(w, h), U32(C.border, C.pill_alpha), r, 0, 1.f);

    const float ty = o.y + P(L.pill_text_y);
    TextAt(dl, fonts::m14, ImVec2(o.x + P(L.pill_logo_x), ty), U32(theme.accent), "fanta");
    (void)tex;
    const ImU32 sep = U32(C.line);
    dl->AddRectFilled(o + ImVec2(P(L.pill_sep1), P(L.pill_sep_y0)), o + ImVec2(P(L.pill_sep1) + 1.f, P(L.pill_sep_y1)), sep);
    dl->AddRectFilled(o + ImVec2(P(L.pill_sep2), P(L.pill_sep_y0)), o + ImVec2(P(L.pill_sep2) + 1.f, P(L.pill_sep_y1)), sep);

    static const float beat[][2] = {
        {0.f,1.f},{2.f,.5f},{5.f,.5f},{7.f,.4f},{9.f,0.f},{13.f,0.f},{14.2f,-3.0f},{15.5f,-.6f},{20.f,.4f},{26.f,.8f},{27.2f,12.f},{28.3f,-14.5f},{29.6f,-1.5f},{31.f,-.2f},{35.f,.8f},{37.f,1.f} };
    const int n = IM_ARRAYSIZE(beat);
    const float gx0 = o.x + P(L.pill_graph_x0), gw = P(L.pill_graph_x1) - P(L.pill_graph_x0), gy = o.y + P(L.pill_graph_y);
    const float k = gw / 37.f;
    ImVec2 pts[IM_ARRAYSIZE(beat)];
    if (frozen)
    {
        for (int i = 0; i < n; ++i) pts[i] = ImVec2(gx0 + beat[i][0] * k, gy + beat[i][1] * k);
        dl->AddPolyline(pts, n, U32(theme.accent), 0, PF(1.1f));
    }
    else
    {

        const float phase = fmodf(time * 0.6f, 1.f) * 37.f;
        dl->PushClipRect(ImVec2(gx0, o.y), ImVec2(gx0 + gw, o.y + h), true);
        for (int rep = 0; rep < 2; ++rep)
        {
            for (int i = 0; i < n; ++i) pts[i] = ImVec2(gx0 + (beat[i][0] - phase + 37.f * rep) * k, gy + beat[i][1] * k);
            dl->AddPolyline(pts, n, U32(theme.accent), 0, PF(1.1f));
        }
        dl->PopClipRect();
    }

    char fps[16], clock[16];
    if (frozen) { ImStrncpy(fps, "180", 16); ImStrncpy(clock, "22:47", 16); }
    else
    {
        ImFormatString(fps, 16, "%.0f", ImGui::GetIO().Framerate);
        SYSTEMTIME st; ::GetLocalTime(&st);
        ImFormatString(clock, 16, "%02d:%02d", st.wHour, st.wMinute);
    }
    if (s.pill_fps)
    {
        TextAt(dl, fonts::m14, ImVec2(o.x + P(L.pill_fps_x), ty), U32(C.text), fps);
        TextAt(dl, fonts::f14, ImVec2(o.x + P(L.pill_unit_x), ty), U32(C.text_dim), "fps");
    }
    if (s.pill_clock)
        TextAt(dl, fonts::m14, ImVec2(o.x + P(L.pill_clock_x), ty), U32(C.text), clock);

    if (s.pill_enemies)
    {
        dl->AddRectFilled(o + ImVec2(P(L.pill_sep3), P(L.pill_sep_y0)), o + ImVec2(P(L.pill_sep3) + 1.f, P(L.pill_sep_y1)), sep);
        char count[16];
        ImFormatString(count, 16, "%d", frozen ? 0 : settings::stats.enemies);
        const bool any_near = !frozen && settings::stats.nearby > 0;
        const float x = o.x + P(L.pill_enemy_x);
        TextAt(dl, fonts::m14, ImVec2(x, ty), any_near ? U32(theme.accent) : U32(C.text), count);
        TextAt(dl, fonts::f14, ImVec2(x + TextSize(fonts::m14, count).x + P(4.f), ty), U32(C.text_dim), "enemies");
    }
}

float PillWidth(const State& s)
{
    const Layout& L = theme.lay;
    return P(L.pill_w) + (s.pill_enemies ? P(L.pill_enemy_w) : 0.f);
}

// Map an ImGuiKey (as stored by KeyRow) to a Windows virtual-key code usable
// with GetAsyncKeyState. Covers mouse buttons, modifiers, letters, digits,
// function keys and a handful of common navigation keys.
static int ImGuiKeyToVK(int k)
{
    switch (k)
    {
        case ImGuiKey_MouseLeft:   return VK_LBUTTON;
        case ImGuiKey_MouseRight:  return VK_RBUTTON;
        case ImGuiKey_MouseMiddle: return VK_MBUTTON;
        case ImGuiKey_MouseX1:     return VK_XBUTTON1;
        case ImGuiKey_MouseX2:     return VK_XBUTTON2;
        case ImGuiKey_LeftShift:
        case ImGuiKey_RightShift:  return VK_SHIFT;
        case ImGuiKey_LeftCtrl:
        case ImGuiKey_RightCtrl:   return VK_CONTROL;
        case ImGuiKey_LeftAlt:
        case ImGuiKey_RightAlt:    return VK_MENU;
        case ImGuiKey_Space:       return VK_SPACE;
        case ImGuiKey_Tab:         return VK_TAB;
        case ImGuiKey_Enter:       return VK_RETURN;
        case ImGuiKey_Escape:      return VK_ESCAPE;
        case ImGuiKey_Insert:      return VK_INSERT;
        case ImGuiKey_Delete:      return VK_DELETE;
        case ImGuiKey_Home:        return VK_HOME;
        case ImGuiKey_End:         return VK_END;
        default: break;
    }
    if (k >= ImGuiKey_A && k <= ImGuiKey_Z) return 'A' + (k - ImGuiKey_A);
    if (k >= ImGuiKey_0 && k <= ImGuiKey_9) return '0' + (k - ImGuiKey_0);
    if (k >= ImGuiKey_F1 && k <= ImGuiKey_F12) return VK_F1 + (k - ImGuiKey_F1);
    return 0;
}

void SyncAimRuntime()
{
    ::aim::cfg.enabled           = aim::enable.on;
    ::aim::cfg.hotkey_vk         = ImGuiKeyToVK(aim::key);
    ::aim::cfg.fov_px            = aim::fov;
    ::aim::cfg.smooth            = aim::smooth;
    ::aim::cfg.strength          = aim::strength;
    ::aim::cfg.max_distance_m    = aim::max_dist;
    ::aim::cfg.visible_only      = aim::visible.on;
    ::aim::cfg.include_knocked   = !aim::knocked.on;   // UI says "Skip Knocked"
    ::aim::cfg.include_teammates = aim::team.on;
    ::aim::cfg.sticky            = aim::sticky.on;
    ::aim::cfg.predict           = aim::predict.on;
    ::aim::cfg.bullet_drop       = aim::drop.on;
    switch (aim::bone)
    {
        case 0: ::aim::cfg.bone = sdk::bone::head; break;
        case 1: ::aim::cfg.bone = sdk::bone::neck; break;
        case 2: ::aim::cfg.bone = sdk::bone::spine_02; break;
        default: ::aim::cfg.bone = sdk::bone::head; break;  // "Nearest" falls back to head
    }
}

}
