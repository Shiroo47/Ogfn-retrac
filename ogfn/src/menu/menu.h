#pragma once
#include <Windows.h>

namespace menu {
    bool init();
    void shutdown();
    void toggle(HWND game);
    bool visible();
    bool quit_requested();
    void frame(HWND game);
}
