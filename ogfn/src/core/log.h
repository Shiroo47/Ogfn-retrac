#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>

namespace logx {
    inline FILE*      file = nullptr;
    inline std::mutex mtx;

    inline void open() {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        wchar_t* slash = wcsrchr(path, L'\\');
        if (slash) *(slash + 1) = 0;
        wcscat_s(path, L"ogfn_log.txt");
        _wfopen_s(&file, path, L"w");
    }

    inline void print(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        std::lock_guard<std::mutex> lk(mtx);
        fputs(buf, stdout);
        if (file) {
            fputs(buf, file);
            fflush(file);
        }
    }
}
