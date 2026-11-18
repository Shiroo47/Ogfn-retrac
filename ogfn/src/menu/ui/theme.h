
#pragma once
#include "imgui.h"

namespace ui {

struct Palette
{
    const char* name;
    ImVec4 window, border, panel, panel_border, strip_line;
    ImVec4 field, field_border, field_hover, line, nav_active, nav_hover, track;
    ImVec4 text, text_dim, value, faint, placeholder, win_button, title;
    float  pill_alpha;
};

inline Palette PaletteDark()
{
    Palette p{};
    p.name         = "Dark";
    p.window       = ImColor(24, 23, 21);
    p.border       = ImColor(40, 39, 38);
    p.panel        = ImColor(29, 28, 26);
    p.panel_border = ImColor(45, 44, 42);
    p.strip_line   = ImColor(45, 44, 42);
    p.field        = ImColor(34, 32, 30);
    p.field_border = ImColor(53, 50, 46);
    p.field_hover  = ImColor(40, 38, 35);
    p.line         = ImColor(52, 49, 45);
    p.nav_active   = ImColor(33, 32, 30);
    p.nav_hover    = ImColor(29, 28, 26);
    p.track        = ImColor(46, 43, 40);
    p.text         = ImColor(236, 235, 232);
    p.text_dim     = ImColor(156, 151, 143);
    p.value        = ImColor(176, 172, 166);
    p.faint        = ImColor(95, 91, 85);
    p.placeholder  = ImColor(142, 138, 131);
    p.win_button   = ImColor(138, 134, 127);
    p.title        = p.text;
    p.pill_alpha   = 0.88f;
    return p;
}

inline Palette PaletteBlack()
{
    Palette p{};
    p.name         = "Black";
    p.window       = ImColor(10, 10, 10);
    p.border       = ImColor(17, 17, 17);
    p.panel        = ImColor(10, 10, 10);
    p.panel_border = ImColor(17, 17, 17);
    p.strip_line   = ImColor(23, 23, 23);
    p.field        = ImColor(18, 18, 18);
    p.field_border = ImColor(30, 30, 30);
    p.field_hover  = ImColor(24, 24, 24);
    p.line         = ImColor(22, 22, 22);
    p.nav_active   = ImColor(18, 18, 18);
    p.nav_hover    = ImColor(14, 14, 14);
    p.track        = ImColor(30, 30, 30);
    p.text         = ImColor(236, 236, 236);
    p.text_dim     = ImColor(157, 157, 157);
    p.value        = ImColor(176, 176, 176);
    p.faint        = ImColor(90, 90, 90);
    p.placeholder  = ImColor(130, 130, 130);
    p.win_button   = ImColor(150, 150, 150);
    p.title        = p.text;
    p.pill_alpha   = 0.88f;
    return p;
}

struct Layout
{
    float win_base_w = 822.f, win_base_h = 538.f;
    float win_rounding = 13.2f;

    float divider_x = 208.f, divider_y0 = 18.f, divider_y1 = 574.f;
    ImVec2 logo_pos = ImVec2(17.f, 18.f); float logo_size = 24.f;
    ImVec2 title_pos = ImVec2(51.f, 22.f), version_pos = ImVec2(150.f, 24.f);
    ImVec2 search_icon = ImVec2(21.f, 73.f), search_text = ImVec2(46.f, 72.f);
    float search_x1 = 196.f;
    float underline_y = 94.f, underline_x0 = 11.f, underline_x1 = 197.f;
    float nav_x0 = 11.f, nav_x1 = 197.f, nav_y0 = 108.f, nav_h = 33.f, nav_pitch = 35.f, nav_rounding = 8.8f;
    float nav_icon_x = 19.f, nav_text_x = 46.f, nav_text_dy = 9.f, nav_icon_dy = 8.f, nav_eye_dy = 7.f;
    ImVec2 avatar_pos = ImVec2(18.f, 543.f); float avatar_size = 31.f;
    ImVec2 user_pos = ImVec2(60.f, 543.f), sub_pos = ImVec2(60.f, 559.f);
    ImVec2 avatar_b_pos = ImVec2(16.5f, 543.125f), avatar_b_size = ImVec2(33.f, 31.625f);

    float head_x = 240.f, head_y = 22.f, head_sep_before = 13.2f, head_sep_after = 14.3f, head_sep_y0 = 23.f, head_sep_y1 = 36.f, subtab_gap = 17.6f;
    ImVec2 min_pos = ImVec2(839.f, 24.f), close_pos = ImVec2(867.f, 25.f);
    float winbtn_hit = 11.f, winbtn_center = 5.f;

    ImVec2 footer_right = ImVec2(868.5f, 562.f);

    float left_x0 = 226.f, left_x1 = 547.f, right_x0 = 565.f, right_x1 = 885.f;
    float panel_top = 59.f, panel_gap = 18.f, panel_rounding = 11.f, panel_pad = 14.f;
    float plain_title_dy = 12.f, plain_row0 = 41.f;
    float strip_h = 43.f, strip_row0 = 61.f, strip_icon_x = 15.f, strip_title_x = 34.f, strip_title_dy = 14.3f, strip_icon_dy = 15.5f, strip_tint = 0.0714f;
    float info_gap = 6.f, info_dy = 1.f, eye_title_dx = 2.5f;
    float panel_end = 27.f, strip_end = 27.6f;
    float row_base = 26.f;
    float row_text_dy = 0.f;

    float check_size = 15.f, check_rounding = 4.4f, swatch_gap = 26.f, swatch_rounding = 4.4f;
    float check_inset = 14.f, check_glyph = 2.f;
    float frame_h = 22.f, frame_dy = 3.f, frame_rounding = 6.6f;
    float track_h = 2.f, track_dy = 7.f, track_w = 132.f, track_inset = 19.f, value_gap = 11.f;
    float combo_w = 130.f, combo_text_x = 9.f, combo_text_dy = 0.f, chevron_x = 114.f, chevron_dy = 2.f;
    float input_w = 165.f, button_w = 92.f, config_button_w = 57.f;
    float config_x[3] = { 688.f, 751.f, 815.f };
    float active_gap = 8.f;
    float popup_item_h = 24.f, popup_pad = 4.f, popup_gap = 3.f, hit_slop = 4.f;
    float picker_pad = 8.f, picker_w = 150.f, picker_gap = 6.f;

    float pill_w = 256.f, pill_h = 32.f, pill_rounding = 9.f;
    ImVec2 pill_screen = ImVec2(17.f, 17.f);
    float pill_logo_x = 11.f, pill_logo_y = 9.f, pill_logo_size = 15.f;
    float pill_name_x = 40.f, pill_sep1 = 86.f, pill_graph_x0 = 97.6f, pill_graph_x1 = 134.6f, pill_graph_y = 18.5f;
    float pill_fps_x = 141.f, pill_unit_x = 167.f, pill_sep2 = 196.f, pill_clock_x = 208.f, pill_text_y = 10.f, pill_sep_y0 = 10.f, pill_sep_y1 = 23.f;
    float pill_sep3 = 252.f, pill_enemy_x = 264.f, pill_enemy_w = 92.f, radar_gap = 8.f;
};

inline Layout LayoutB()
{
    Layout l;
    l.title_pos = ImVec2(33.55f, 18.7f);
    l.version_pos = ImVec2(151.3f, 24.2f);
    l.divider_x = 207.6f;
    l.search_icon = ImVec2(21.f, 74.8f);
    l.search_text = ImVec2(46.6f, 74.4f);
    l.underline_y = 95.5f;
    l.nav_y0 = 110.f; l.nav_pitch = 35.2f; l.nav_eye_dy = 8.f;
    l.left_x0 = 226.48f; l.left_x1 = 547.9f; l.right_x0 = 565.1f; l.right_x1 = 886.9f;
    l.panel_top = 59.1f;
    l.strip_h = 42.9f; l.strip_row0 = 60.6f;
    l.panel_gap = 17.3f;
    l.nav_text_dy = 8.6f;
    l.head_x = 240.9f;
    l.user_pos = ImVec2(59.6f, 542.4f); l.sub_pos = ImVec2(59.6f, 558.6f);
    l.check_size = 15.4f; l.check_inset = 14.44f; l.swatch_gap = 26.4f; l.panel_pad = 14.5f;
    l.strip_row0 = 60.82f; l.row_text_dy = -0.4f; l.strip_title_dy = 14.5f; l.strip_icon_dy = 16.9f; l.info_dy = 1.4f;
    l.track_dy = 6.6f; l.track_h = 2.2f;
    l.footer_right = ImVec2(869.f, 561.2f);
    l.min_pos = ImVec2(840.5f, 24.f); l.close_pos = ImVec2(868.f, 25.f);
    return l;
}

struct Theme
{
    ImVec4  accent_red   = ImColor(192, 36, 20);
    ImVec4  accent_green = ImColor(46, 255, 154);
    float   rainbow_sat = 0.82f, rainbow_val = 1.f, rainbow_speed = 0.08f;
    ImVec4  accent = accent_red;
    ImVec4  clear = ImVec4(0.f, 0.f, 0.f, 0.f);
    Palette pal = PaletteBlack();
    Layout  lay;
    ImVec4  check_mark = ImColor(10, 10, 10);

    struct Motion {
        float speed      = 12.f;
        float check      = 18.f;
        float nav        = 16.f;
        float page_fade  = 15.f;
    } motion;

    struct Atlas {
        float gamma       = 0.65f;
        int   oversample_h = 3;
        int   oversample_v = 1;
    } atlas;
};

inline Theme theme;

namespace fonts {
    inline ImFont* f12 = nullptr;
    inline ImFont* f13 = nullptr;
    inline ImFont* f14 = nullptr;
    inline ImFont* f15 = nullptr;
    inline ImFont* f18 = nullptr;
    inline ImFont* m12 = nullptr;
    inline ImFont* m13 = nullptr;
    inline ImFont* m14 = nullptr;
    inline ImFont* i09 = nullptr;
    inline ImFont* i12 = nullptr;
    inline ImFont* i15 = nullptr;
}

namespace icon {
    inline const char* eye        = "\xEE\x88\xA0";
    inline const char* crosshair  = "\xEE\x87\x96";
    inline const char* sliders    = "\xEE\x90\xB4";
    inline const char* gear       = "\xEE\x89\xB2";
    inline const char* search     = "\xEE\x8C\x8C";
    inline const char* info       = "\xEE\x8B\x8E";
    inline const char* user       = "\xEE\x93\x82";
    inline const char* users      = "\xEE\x93\x96";
    inline const char* idcard     = "\xEE\x8B\x88";
    inline const char* text_aa    = "\xEE\x9B\xAE";
    inline const char* tag        = "\xEE\x91\xB8";
    inline const char* package    = "\xEE\x8E\x90";
    inline const char* globe      = "\xEE\x8A\x8C";
    inline const char* box        = "\xEE\x9B\x8E";
    inline const char* person     = "\xEE\x8E\xA8";
    inline const char* palette    = "\xEE\x9B\x88";
    inline const char* lightning  = "\xEE\x8B\x9E";
    inline const char* click      = "\xEE\x9F\x88";
    inline const char* sword      = "\xEE\x96\xBA";
    inline const char* monitor    = "\xEE\x8C\xAE";
    inline const char* keyboard   = "\xEE\x8B\x98";
    inline const char* compass    = "\xEE\x87\x88";
    inline const char* minus      = "\xEE\x8C\xAA";
    inline const char* close      = "\xEE\x93\xB6";
    inline const char* caret_down = "\xEE\x84\xB6";
    inline const char* check      = "\xEE\x86\x82";
}

}
