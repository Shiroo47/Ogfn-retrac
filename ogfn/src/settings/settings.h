#pragma once

namespace settings {
    struct Color {
        float r = 1.f, g = 1.f, b = 1.f;
    };

    struct Config {
        bool  esp_enable     = true;

        bool  box            = true;
        bool  box_corner     = false;
        bool  box_filled     = false;
        bool  box_3d         = false;
        bool  skeleton       = true;
        bool  snaplines      = false;
        bool  head_circle    = false;
        bool  distance       = true;
        bool  names          = false;
        Color name_color{ 1.f, 1.f, 1.f };

        bool  weapon         = true;
        Color weapon_color{ 1.f, 1.f, 1.f };

        bool  knocked_flag   = true;
        Color knocked_color{ 1.f, 0.77f, 0.25f };
        Color knocked_text_color{ 1.f, 0.77f, 0.25f };

        bool  offscreen      = true;
        Color offscreen_color{ 0.75f, 0.14f, 0.08f };

        bool  radar          = true;
        bool  radar_lines    = true;
        float radar_size     = 230.f;
        float radar_zoom     = 1.5f;
        float radar_alpha    = 0.8f;
        Color radar_color{ 0.75f, 0.14f, 0.08f };

        float radar_screen_x = 17.f;
        float radar_screen_y = 60.f;

        bool  enemy_counter  = true;

        bool  fade           = true;

        float font_size      = 13.f;
        float text_bg        = 0.35f;
        int   text_align     = 1;
        bool  text_outline   = true;
        bool  shorten        = false;

        bool  loot_chests    = true;
        bool  loot_ammo      = false;
        bool  loot_supply    = true;
        bool  loot_floor     = false;
        bool  loot_vehicles  = false;
        bool  loot_llamas    = true;
        int   loot_min_rarity = 2;
        float loot_distance  = 300.f;
        Color chest_color{ 1.f, 0.77f, 0.25f };
        Color ammo_color{ 0.47f, 0.71f, 0.47f };
        Color supply_color{ 0.34f, 0.55f, 0.98f };
        Color vehicle_color{ 1.f, 1.f, 1.f };
        Color llama_color{ 0.75f, 0.47f, 1.f };

        bool  visible_check  = true;
        bool  hide_hidden    = false;
        bool  show_enemies   = true;
        bool  show_teammates = false;
        bool  lobby_esp      = true;

        Color box_color{ 0.16f, 0.86f, 0.31f };
        Color skel_color{ 1.f, 1.f, 1.f };
        Color fill_color{ 0.16f, 0.86f, 0.31f };
        Color visible_color{ 0.16f, 0.86f, 0.31f };
        Color hidden_color{ 0.86f, 0.18f, 0.12f };
        Color team_color{ 0.34f, 0.55f, 0.98f };
        Color text_color{ 1.f, 1.f, 1.f };

        float max_distance   = 300.f;
        float box_thickness  = 1.5f;
        float skel_thickness = 1.5f;
        float fill_opacity   = 0.25f;
        float corner_pct     = 0.25f;

        bool  crosshair       = false;
        bool  crosshair_dot   = true;
        float crosshair_size  = 8.f;
        float crosshair_gap   = 3.f;
        Color crosshair_color{ 1.f, 1.f, 1.f };
    };

    inline Config cfg;

    struct Stats {
        int enemies = 0;
        int nearby  = 0;
    };

    inline Stats stats;
}
