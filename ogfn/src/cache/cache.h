#pragma once
#include <cstdint>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_set>
#include "../mem/process.h"
#include "../core/log.h"
#include "../sdk/world.h"
#include "../sdk/pawn.h"
#include "../sdk/camera.h"
#include "../sdk/names.h"
#include "../sdk/weapon.h"
#include <unordered_map>

namespace cache {
    constexpr int name_cap = 32;

    struct pawn_entry {
        uint64_t addr;
        uint64_t mesh;
        wchar_t  name[name_cap];
        bool     knocked;
        wchar_t  weapon[sdk::weapon_cap];
        uint8_t  rarity;
    };

    struct snapshot {
        sdk::ctx                ctx;
        uint64_t                pawn_class = 0;
        std::vector<pawn_entry> pawns;
    };

    inline snapshot          latest;
    inline std::mutex        mtx;
    inline std::atomic<bool> stop{ false };
    inline std::thread       worker_thread;

    using clk = std::chrono::steady_clock;
    inline long long ms_since(clk::time_point t) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(clk::now() - t).count();
    }

    inline sdk::ctx resolve_context(uint64_t world) {
        sdk::ctx c;
        c.world = world;
        c.gi    = sdk::game_instance(world);
        if (!c.gi) return c;
        c.lp    = sdk::local_player(c.gi);
        if (!c.lp) return c;
        c.pc    = sdk::player_controller(c.lp);
        if (!c.pc) return c;
        c.pcm   = process::rpm<uint64_t>(c.pc + offsets::PlayerCameraManager);
        c.pawn  = sdk::acknowledged_pawn(c.pc);
        c.lobby = sdk::is_frontend_controller(c.pc);
        return c;
    }

    inline void worker() {
        std::vector<uint64_t>        buffer;
        std::vector<pawn_entry>      list;
        std::unordered_set<uint64_t> rejected;
        std::unordered_map<uint64_t, sdk::weapon_info> weapons;
        sdk::ctx                     ctx;
        uint64_t                     pawn_class = 0;
        uint64_t                     last_world = 0;
        bool                         names_tried = false;
        auto                         t_ctx    = clk::now() - std::chrono::seconds(10);
        auto                         t_rescan = t_ctx;
        auto                         t_reject = clk::now();

        while (!stop.load()) {
            auto tick_start = clk::now();

            uint64_t world = sdk::gworld();
            if (!world) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); continue; }

            if (world != last_world) {
                last_world = world;
                list.clear();
                rejected.clear();
                weapons.clear();
                pawn_class = 0;
                ctx = sdk::ctx{};
                names_tried = false;
                t_ctx    = clk::now() - std::chrono::seconds(10);
                t_rescan = t_ctx;
            }

            if (pawn_class && !names_tried) {
                names_tried = true;
                sdk::fname::build_keystream(pawn_class);
            }

            bool force_rescan = false;
            bool changed = false;
            if (ms_since(t_ctx) > 150) {
                t_ctx = clk::now();
                sdk::ctx fresh = resolve_context(world);
                if (fresh.pawn) {
                    uint64_t cls = sdk::object_class(fresh.pawn);
                    if (cls && cls != pawn_class) {
                        pawn_class = cls;
                        list.clear();
                        rejected.clear();
                        force_rescan = true;
                    }
                }
                ctx = fresh;
                changed = true;
            }

            if (ms_since(t_reject) > 15000) {
                t_reject = clk::now();
                rejected.clear();
            }

            if (force_rescan || ms_since(t_rescan) > 500) {
                t_rescan = clk::now();
                uint64_t level = sdk::persistent_level(world);
                tarray<uint64_t> actors = level ? sdk::level_actors(level) : tarray<uint64_t>{};
                if (actors.data && actors.count > 0 && actors.count < 100000) {
                    buffer.resize(actors.count);
                    if (process::read_raw(actors.data, buffer.data(), (size_t)actors.count * sizeof(uint64_t))) {
                        std::vector<pawn_entry> next;
                        next.reserve(64);
                        for (uint64_t a : buffer) {
                            if (!a || (a == ctx.pawn && !ctx.lobby)) continue;
                            if (rejected.count(a)) continue;
                            uint64_t cls = sdk::object_class(a);
                            if (!pawn_class && sdk::is_player_pawn_class(cls)) pawn_class = cls;
                            if (!cls || cls != pawn_class) { rejected.insert(a); continue; }
                            if (sdk::pawn_is_dying(a)) continue;
                            uint64_t mesh = sdk::pawn_mesh(a);
                            if (!mesh) continue;
                            pawn_entry e{ a, mesh, {}, false, {}, sdk::no_rarity };
                            sdk::pawn_name(a, e.name, name_cap);
                            e.knocked = sdk::pawn_is_dbno(a);
                            if (sdk::fname::ready()) {
                                if (const sdk::weapon_info* w = sdk::cached_weapon(weapons, sdk::pawn_weapon_def(a))) {
                                    wcscpy_s(e.weapon, w->label);
                                    e.rarity = w->rarity;
                                }
                            }
                            next.push_back(e);
                        }
                        list = std::move(next);
                    }
                }
                changed = true;
            }

            if (changed) {
                std::lock_guard<std::mutex> lk(mtx);
                latest.ctx = ctx;
                latest.pawn_class = pawn_class;
                latest.pawns = list;
            }

            auto spent = clk::now() - tick_start;
            auto budget = std::chrono::milliseconds(8);
            if (spent < budget) std::this_thread::sleep_for(budget - spent);
        }
    }

    inline snapshot get() {
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
