#pragma once
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include "../mem/process.h"
#include "offsets.h"
#include "pawn.h"
#include "names.h"

namespace sdk {
    constexpr int weapon_cap = 32;
    constexpr uint8_t no_rarity = 0xFF;

    struct weapon_info {
        wchar_t label[weapon_cap];
        uint8_t rarity;
    };

    inline uint64_t pawn_weapon_def(uint64_t pawn) {
        uint64_t weapon = pawn ? process::rpm<uint64_t>(pawn + offsets::CurrentWeapon) : 0;
        return weapon ? process::rpm<uint64_t>(weapon + offsets::WeaponData) : 0;
    }

    inline bool read_ftext(uint64_t ftext, wchar_t* out, int cap) {
        out[0] = 0;
        uint64_t data = process::rpm<uint64_t>(ftext);
        if (data < 0x10000) return false;
        fstring_header h = process::rpm<fstring_header>(data + offsets::TextDataSourceString);
        int len = h.count - 1;
        if (!h.data || len <= 0 || len > 128 || h.count > h.max) return false;
        if (len > cap - 1) len = cap - 1;
        if (!process::read_raw(h.data, out, (size_t)len * sizeof(wchar_t))) { out[0] = 0; return false; }
        for (int i = 0; i < len; i++) {
            if (out[i] < 0x20 || out[i] == 0xFFFF) { out[0] = 0; return false; }
        }
        out[len] = 0;
        return true;
    }

    inline void label_from_fname(const char* fname, wchar_t* out, int cap) {
        const char* s = fname;
        if (!strncmp(s, "WID_", 4)) s += 4;
        const char* end = strstr(s, "_Athena");
        int n = end ? (int)(end - s) : (int)strlen(s);
        if (n > cap - 1) n = cap - 1;
        for (int i = 0; i < n; i++) out[i] = s[i] == '_' ? L' ' : (wchar_t)(unsigned char)s[i];
        out[n] = 0;
    }

    inline void trim(wchar_t* s) {
        wchar_t* p = s;
        while (*p == L' ') p++;
        if (p != s) memmove(s, p, (wcslen(p) + 1) * sizeof(wchar_t));
        size_t n = wcslen(s);
        while (n > 0 && s[n - 1] == L' ') s[--n] = 0;
    }

    inline weapon_info resolve_weapon(uint64_t def) {
        weapon_info w{};
        w.rarity = no_rarity;
        char fname[128] = {};
        bool have_fname = fname::name_of(def, fname, sizeof(fname));
        if (have_fname && !strncmp(fname, "WID_Harvest_", 12)) {
            wcscpy_s(w.label, L"Pickaxe");
        } else if (have_fname && !strncmp(fname, "BuildingItemData_", 17)) {
            wcscpy_s(w.label, L"Building");
        } else if (have_fname && !strcmp(fname, "EditTool")) {
            wcscpy_s(w.label, L"Editing");
        } else {
            uint8_t r = process::rpm<uint8_t>(def + offsets::ItemRarity);
            w.rarity = r <= 6 ? r : no_rarity;
            bool have_text = read_ftext(def + offsets::ItemDisplayName, w.label, weapon_cap);
            if (!have_text && have_fname) label_from_fname(fname, w.label, weapon_cap);
            trim(w.label);
        }
        return w;
    }

    inline const weapon_info* cached_weapon(std::unordered_map<uint64_t, weapon_info>& cache, uint64_t def) {
        if (!def) return nullptr;
        auto it = cache.find(def);
        if (it == cache.end()) it = cache.emplace(def, resolve_weapon(def)).first;
        return &it->second;
    }
}
