#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>

namespace process {
    inline HANDLE handle = nullptr;
    inline DWORD  pid    = 0;
    inline uint64_t base = 0;
    inline uint64_t size = 0;

    inline bool enable_debug_priv() {
        HANDLE t = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &t)) return false;
        LUID luid{};
        if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) { CloseHandle(t); return false; }
        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        BOOL ok = AdjustTokenPrivileges(t, FALSE, &tp, sizeof(tp), nullptr, nullptr);
        DWORD err = GetLastError();
        CloseHandle(t);
        return ok && err == ERROR_SUCCESS;
    }

    struct _find_ctx { const wchar_t* exe; DWORD pid; HWND hwnd; };

    inline BOOL CALLBACK _find_cb(HWND hwnd, LPARAM lp) {
        auto* ctx = (_find_ctx*)lp;
        if (!IsWindowVisible(hwnd)) return TRUE;
        DWORD p = 0;
        GetWindowThreadProcessId(hwnd, &p);
        if (!p) return TRUE;
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p);
        if (!h) return TRUE;
        wchar_t path[MAX_PATH]{};
        DWORD sz = MAX_PATH;
        BOOL ok = QueryFullProcessImageNameW(h, 0, path, &sz);
        CloseHandle(h);
        if (!ok) return TRUE;
        const wchar_t* base = wcsrchr(path, L'\\');
        base = base ? base + 1 : path;
        if (_wcsicmp(base, ctx->exe) == 0) { ctx->pid = p; ctx->hwnd = hwnd; return FALSE; }
        return TRUE;
    }

    inline HWND target_hwnd = nullptr;

    inline DWORD find_pid_by_window(const wchar_t* exe) {
        _find_ctx ctx{ exe, 0, nullptr };
        EnumWindows(_find_cb, (LPARAM)&ctx);
        target_hwnd = ctx.hwnd;
        return ctx.pid;
    }

    inline DWORD find_pid_by_toolhelp(const wchar_t* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        PROCESSENTRY32W pe{ sizeof(pe) };
        DWORD found = 0;
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, name) == 0) { found = pe.th32ProcessID; break; }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
        return found;
    }

    inline DWORD find_pid(const wchar_t* name) {
        DWORD p = find_pid_by_window(name);
        if (p) return p;
        return find_pid_by_toolhelp(name);
    }

    inline uint64_t find_base_via_modules(const wchar_t* exe) {
        HMODULE mods[1024];
        DWORD needed = 0;
        if (!EnumProcessModulesEx(handle, mods, sizeof(mods), &needed, LIST_MODULES_64BIT)) return 0;
        DWORD count = needed / sizeof(HMODULE);
        for (DWORD i = 0; i < count; i++) {
            wchar_t name[MAX_PATH]{};
            if (GetModuleBaseNameW(handle, mods[i], name, MAX_PATH)) {
                if (_wcsicmp(name, exe) == 0) {
                    MODULEINFO mi{};
                    if (GetModuleInformation(handle, mods[i], &mi, sizeof(mi))) {
                        size = mi.SizeOfImage;
                        return (uint64_t)mi.lpBaseOfDll;
                    }
                }
            }
        }
        return 0;
    }

    inline uint64_t find_fortnite_image() {
        uint64_t addr = 0x10000;
        uint64_t last_alloc = 0;
        MEMORY_BASIC_INFORMATION mbi{};
        while (VirtualQueryEx(handle, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.AllocationBase == mbi.BaseAddress && (uint64_t)mbi.BaseAddress != last_alloc) {
                last_alloc = (uint64_t)mbi.BaseAddress;
                uint16_t dos = 0;
                if (ReadProcessMemory(handle, mbi.BaseAddress, &dos, 2, nullptr) && dos == 0x5A4D) {
                    int32_t e_lfanew = 0;
                    ReadProcessMemory(handle, (char*)mbi.BaseAddress + 0x3C, &e_lfanew, 4, nullptr);
                    if (e_lfanew > 0 && e_lfanew < 0x400) {
                        uint32_t sig = 0;
                        ReadProcessMemory(handle, (char*)mbi.BaseAddress + e_lfanew, &sig, 4, nullptr);
                        if (sig == 0x4550) {
                            uint16_t chars = 0; uint64_t pref_base = 0; uint32_t img_size = 0;
                            ReadProcessMemory(handle, (char*)mbi.BaseAddress + e_lfanew + 4 + 18, &chars, 2, nullptr);
                            ReadProcessMemory(handle, (char*)mbi.BaseAddress + e_lfanew + 4 + 20 + 24, &pref_base, 8, nullptr);
                            ReadProcessMemory(handle, (char*)mbi.BaseAddress + e_lfanew + 4 + 20 + 56, &img_size, 4, nullptr);
                            bool is_exe = (chars & 0x2000) == 0;
                            bool fortnite_sized = img_size > 0x2000000;
                            bool self_referencing = pref_base == (uint64_t)mbi.BaseAddress;
                            if (is_exe && fortnite_sized && self_referencing) {
                                size = img_size;
                                return (uint64_t)mbi.BaseAddress;
                            }
                        }
                    }
                }
            }
            uint64_t next = (uint64_t)mbi.BaseAddress + mbi.RegionSize;
            if (next <= addr) break;
            addr = next;
            if (addr > 0x7FFFFFFFFFFF) break;
        }
        return 0;
    }

    inline bool attach(const wchar_t* exe) {
        static bool priv_done = false;
        if (!priv_done) { enable_debug_priv(); priv_done = true; }

        pid = find_pid(exe);
        if (!pid) return false;
        handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!handle) return false;

        base = find_fortnite_image();
        return base != 0;
    }

    inline void detach() {
        if (handle) { CloseHandle(handle); handle = nullptr; }
        pid = 0; base = 0; size = 0;
    }

    inline bool read_raw(uint64_t addr, void* dst, size_t bytes) {
        if (!handle || !addr) return false;
        SIZE_T r = 0;
        return ReadProcessMemory(handle, (LPCVOID)addr, dst, bytes, &r) && r == bytes;
    }

    inline bool write_raw(uint64_t addr, const void* src, size_t bytes) {
        if (!handle || !addr) return false;
        SIZE_T w = 0;
        return WriteProcessMemory(handle, (LPVOID)addr, src, bytes, &w) && w == bytes;
    }

    template <class T>
    T rpm(uint64_t addr) {
        T v{};
        read_raw(addr, &v, sizeof(T));
        return v;
    }

    template <class T>
    bool rpm_into(uint64_t addr, T& dst) {
        return read_raw(addr, &dst, sizeof(T));
    }
}
