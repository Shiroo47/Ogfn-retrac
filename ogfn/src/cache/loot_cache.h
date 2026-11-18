#pragma once
#include <cstdint>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "../mem/process.h"
#include "../sdk/loot.h"
#include "../sdk/weapon.h"
#include "../sdk/camera.h"
#include "../settings/settings.h"
#include "cache.h"

namespace loot {
    constexpr int label_cap = 40;
    constexpr float range_margin_m = 50.f;

    struct item {
        fvector        pos;
        sdk::loot_kind kind;
        uint8_t        rarity;
        wchar_t        label[label_cap];
    };

    struct tracked {
        uint64_t       cls;
        sdk::loot_kind kind;
        const char*    vehicle_label;
        fvector        fixed_pos;
        bool           has_fixed_pos;
    };

    inline std::vector<item> latest;
    inline std::mutex        mtx;
    inline std::atomic<bool> stop{ false };
    inline std::thread       worker_thread;

    struct class_entry { sdk::loot_kind kind; const char* vehicle_label; };

    inline const char* kind_name(sdk::loot_kind k) {
        switch (k) {
            case sdk::loot_kind::chest:   return "chest";
            case sdk::loot_kind::ammo:    return "ammo";
            case sdk::loot_kind::supply:  return "supply";
            case sdk::loot_kind::llama:   return "llama";
            case sdk::loot_kind::pickup:  return "pickup";
            case sdk::loot_kind::vehicle: return "vehicle";
            default:                      return "none";
        }
    }

    inline bool loot_like(const char* n) {
        static const char* keys[] = { "Chest", "Ammo", "Llama", "SupplyDrop", "Vehicle", "Pickup", "Container", "Loot" };
        for (const char* k : keys) if (strstr(n, k)) return true;
        return false;
    }

    inline void log_class(uint64_t cls, sdk::loot_kind k, const char* vl) {
        char nm[128] = {};
        if (!sdk::fname::name_of(cls, nm, sizeof(nm))) strcpy_s(nm, "?");
        if (k != sdk::loot_kind::none)
            logx::print("[loot] class %s -> %s%s%s\n", nm, kind_name(k), vl ? " label=" : "", vl ? vl : "");
        else if (loot_like(nm))
            logx::print("[loot] class %s -> none (loot-like name, not matched)\n", nm);
    }

    inline bool fill_item(uint64_t a, tracked& t, const fvector& origin, float range_cm,
                          std::unordered_map<uint64_t, sdk::weapon_info>& defs, item& out, bool& searched) {
        searched = false;
        const bool is_static = t.kind == sdk::loot_kind::chest || t.kind == sdk::loot_kind::ammo;
        fvector pos;
        if (is_static && t.has_fixed_pos) pos = t.fixed_pos;
        else {
            pos = sdk::actor_location(a);
            if (!sdk::finite3(pos) || (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f)) return false;
            if (is_static) { t.fixed_pos = pos; t.has_fixed_pos = true; }
        }
        if (pos.distance(origin) > range_cm) return false;

        out.pos = pos;
        out.kind = t.kind;
        out.rarity = sdk::no_rarity;
        out.label[0] = 0;
        switch (t.kind) {
            case sdk::loot_kind::chest:
                if (sdk::container_searched(a)) { searched = true; return false; }
                wcscpy_s(out.label, L"Chest");
                break;
            case sdk::loot_kind::ammo:
                if (sdk::container_searched(a)) { searched = true; return false; }
                wcscpy_s(out.label, L"Ammo Box");
                break;
            case sdk::loot_kind::supply:  wcscpy_s(out.label, L"Supply Drop"); break;
            case sdk::loot_kind::llama:   wcscpy_s(out.label, L"Llama"); break;
            case sdk::loot_kind::vehicle:
                swprintf(out.label, label_cap, L"%hs", t.vehicle_label ? t.vehicle_label : "Vehicle");
                break;
            case sdk::loot_kind::pickup: {
                int32_t count = 0;
                uint64_t def = sdk::pickup_def(a, count);
                size_t before = defs.size();
                const sdk::weapon_info* w = sdk::cached_weapon(defs, def);
                if (w && defs.size() != before) {
                    char nm[128] = {};
                    if (!sdk::fname::name_of(def, nm, sizeof(nm))) strcpy_s(nm, "?");
                    logx::print("[loot] pickup def %s rarity=%d label=%ls count=%d\n", nm, (int)w->rarity, w->label[0] ? w->label : L"?", count);
                }
                if (!w || !w->label[0]) return false;
                out.rarity = w->rarity;
                if (count > 1) swprintf(out.label, label_cap, L"%ls x%d", w->label, count);
                else wcscpy_s(out.label, w->label);
                break;
            }
            default: return false;
        }
        return true;
    }

    inline void worker() {
        std::unordered_map<uint64_t, class_entry>      classes;
        std::unordered_map<uint64_t, tracked>          actors;
        std::unordered_set<uint64_t>                   ignored;
        std::unordered_map<uint64_t, sdk::weapon_info> defs;
        std::vector<uint64_t>                          levels, buffer;
        uint64_t last_world = 0;
        auto t_flush = std::chrono::steady_clock::now();
        auto t_log = t_flush;

        while (!stop.load()) {
            auto tick = std::chrono::steady_clock::now();
            const settings::Config& cfg = settings::cfg;
            const bool any = cfg.loot_chests || cfg.loot_ammo || cfg.loot_supply || cfg.loot_floor
                          || cfg.loot_vehicles || cfg.loot_llamas;

            uint64_t world = sdk::gworld();
            if (world != last_world) {
                last_world = world;
                classes.clear(); actors.clear(); ignored.clear(); defs.clear();
            }
            if (tick - t_flush > std::chrono::seconds(15)) {
                t_flush = tick;
                ignored.clear();
            }

            std::vector<item> next;
            int n_levels = 0, n_actors = 0;
            int n_seen[7] = {}, n_shown[7] = {}, n_searched[7] = {};
            sdk::ctx ctx = cache::get().ctx;
            sdk::view_info view = (any && world && sdk::fname::ready() && !ctx.lobby) ? sdk::read_view(ctx) : sdk::view_info{};
            if (view.valid) {
                const float range_cm = (cfg.loot_distance + range_margin_m) * 100.f;
                tarray<uint64_t> lv = sdk::world_levels(world);
                if (lv.data && lv.count > 0 && lv.count < 4096) {
                    levels.resize(lv.count);
                    if (process::read_raw(lv.data, levels.data(), (size_t)lv.count * sizeof(uint64_t))) {
                        for (uint64_t level : levels) {
                            if (stop.load()) break;
                            if (!level) continue;
                            tarray<uint64_t> acts = sdk::level_actors(level);
                            if (!acts.data || acts.count <= 0 || acts.count > 200000) continue;
                            buffer.resize(acts.count);
                            if (!process::read_raw(acts.data, buffer.data(), (size_t)acts.count * sizeof(uint64_t))) continue;
                            n_levels++;
                            n_actors += acts.count;
                            for (uint64_t a : buffer) {
                                if (!a || ignored.count(a)) continue;
                                uint64_t cls = sdk::object_class(a);
                                if (!cls) continue;
                                auto it = actors.find(a);
                                if (it == actors.end() || it->second.cls != cls) {
                                    auto ci = classes.find(cls);
                                    if (ci == classes.end()) {
                                        const char* vl = nullptr;
                                        sdk::loot_kind k = sdk::classify_class(cls, vl);
                                        ci = classes.emplace(cls, class_entry{ k, vl }).first;
                                        log_class(cls, k, vl);
                                    }
                                    if (ci->second.kind == sdk::loot_kind::none) {
                                        if (it != actors.end()) actors.erase(it);
                                        ignored.insert(a);
                                        continue;
                                    }
                                    it = actors.insert_or_assign(a, tracked{ cls, ci->second.kind, ci->second.vehicle_label, {}, false }).first;
                                }
                                item out;
                                bool searched = false;
                                const int ki = (int)it->second.kind;
                                n_seen[ki]++;
                                if (fill_item(a, it->second, view.location, range_cm, defs, out, searched)) {
                                    next.push_back(out);
                                    n_shown[ki]++;
                                }
                                if (searched) n_searched[ki]++;
                            }
                        }
                    }
                }
            }

            if (view.valid && tick - t_log > std::chrono::seconds(5)) {
                t_log = tick;
                long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick).count();
                logx::print("[loot] scan %lldms levels=%d actors=%d classes=%zu ignored=%zu\n",
                            ms, n_levels, n_actors, classes.size(), ignored.size());
                for (int k = 1; k < 7; k++)
                    logx::print("[loot]   %-7s total=%d in_range=%d opened=%d\n",
                                kind_name((sdk::loot_kind)k), n_seen[k], n_shown[k], n_searched[k]);
            }

            {
                std::lock_guard<std::mutex> lk(mtx);
                latest = std::move(next);
            }

            auto spent = std::chrono::steady_clock::now() - tick;
            auto budget = std::chrono::milliseconds(1000);
            if (spent < budget) std::this_thread::sleep_for(budget - spent);
        }
    }

    inline std::vector<item> get() {
        std::lock_guard<std::mutex> lk(mtx);
        return latest;
    }

    inline void start() {
        stop.store(false);
        worker_thread = std::thread(worker);
    }

    inline void stop_worker() {
        stop.store(true);
        if (worker_thread.joinable()) worker_thread.join();
    }
}
