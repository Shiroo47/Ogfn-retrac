#include <Windows.h>
#include <timeapi.h>
#include "core/log.h"
#include "mem/process.h"
#include "overlay/overlay.h"
#include "cache/cache.h"
#include "features/esp/esp.h"
#include "features/aim/aim.h"
#include "menu/menu.h"
#include "menu/pages.h"

static bool key_edge(int vk) {
    static bool prev[256]{};
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool edge = down && !prev[vk];
    prev[vk] = down;
    return edge;
}

int main() {
    SetConsoleTitleW(L"fanta.lol");
    logx::open();
    timeBeginPeriod(1);

    logx::print("[fanta] attaching\n");
    while (!process::attach(L"FortniteClient-Win64-Shipping.exe")) Sleep(1000);
    logx::print("[fanta] attached\n");

    if (!overlay::create(process::target_hwnd)) {
        logx::print("[fanta] overlay init failed\n");
        return 1;
    }
    if (!menu::init()) {
        logx::print("[fanta] menu init failed\n");
        return 1;
    }

    cache::start();
    loot::start();
    logx::print("[fanta] running. Insert toggles the menu.\n");

    while (overlay::running) {
        overlay::pump();
        if (key_edge(VK_INSERT)) menu::toggle(process::target_hwnd);

        if (!overlay::sync()) { Sleep(50); continue; }

        cache::snapshot snap = cache::get();
        bool show_esp = GetForegroundWindow() == process::target_hwnd || menu::visible();

        overlay::begin_frame();
        if (show_esp) esp::run(snap);
        pages::SyncAimRuntime();
        aim::run(snap);
        if (show_esp) aim::draw_fov_ring(overlay::draw::argb(200, 255, 255, 255));
        overlay::end_frame();

        menu::frame(process::target_hwnd);
        if (menu::quit_requested()) break;
    }

    loot::stop_worker();
    cache::stop_worker();
    menu::shutdown();
    overlay::shutdown();
    process::detach();
    timeEndPeriod(1);
    return 0;
}
