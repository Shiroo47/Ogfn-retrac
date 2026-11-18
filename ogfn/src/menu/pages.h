
#pragma once
#include "imgui.h"
#include "ui/theme.h"

namespace pages {

enum Page { Visuals = 0, Aimbot = 1, Misc = 2, Settings = 3 };

struct Textures
{
    ImTextureID preview = 0;
    int preview_w = 0, preview_h = 0;
};

struct State
{
    int page = Visuals;
    int sub[4] = { 0, 0, 0, 0 };
    char search[64] = "";

    float ui_scale = 110.f;
    float applied_scale = 110.f;
    int   theme = 1;
    bool  custom_accent = true;
    bool  rainbow_accent = false;
    ImVec4 custom_color = ui::theme.accent_red;

    bool watermark = true;
    bool pill_fps = true, pill_clock = true, pill_enemies = true;
    bool preview = true;

    int  ref = 0;
    bool no_footer = false;

    bool request_close = false, request_minimize = false;
};

void Init(State& s);
void Draw(State& s, const Textures& tex, ImVec2 window_size);

// Copy the current Aimbot UI state (hotkey, FOV, smoothing, bone, filters, etc.)
// into the runtime aim::cfg used by the aim loop. Call every frame from main.
void SyncAimRuntime();

constexpr float preview_gap = 12.f;
constexpr float preview_w   = 300.f;
constexpr float drag_h      = 52.f;

bool PreviewVisible(const State& s);
void DrawPreview(const Textures& tex, const ImVec2& origin, const ImVec2& size);

void DrawPill(ImDrawList* dl, const ImVec2& origin, const State& s, const Textures& tex, float time, bool frozen);
float PillWidth(const State& s);

}
