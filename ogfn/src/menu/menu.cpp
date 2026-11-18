#include "menu.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "ui/theme.h"
#include "ui/widgets.h"
#include "pages.h"
#include "../core/log.h"

#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <wincodec.h>
#include <string>
#include <vector>
#include <cmath>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace menu {

    static ID3D11Device*           g_device    = nullptr;
    static ID3D11DeviceContext*    g_context   = nullptr;
    static IDXGISwapChain*         g_swapchain = nullptr;
    static ID3D11RenderTargetView* g_rtv       = nullptr;
    static HWND                    g_hwnd      = nullptr;

    static HWND                    g_pill_hwnd = nullptr;
    static IDXGISwapChain*         g_pill_sc   = nullptr;
    static ID3D11RenderTargetView* g_pill_rtv  = nullptr;
    static int                     g_pill_w = 0, g_pill_h = 0;

    static pages::State    g_app;
    static pages::Textures g_tex;
    static std::vector<ID3D11ShaderResourceView*> g_textures;
    static bool            g_com = false;
    static bool            g_visible = false;
    static bool            g_positioned = false;
    static bool            g_quit = false;
    static float           g_percent = 110.f;
    static ImVec2          g_menu;
    static ImVec2          g_win;

    static std::string ExeDir() {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (wchar_t* slash = wcsrchr(path, L'\\')) *(slash + 1) = 0;
        char out[MAX_PATH]{};
        WideCharToMultiByte(CP_UTF8, 0, path, -1, out, MAX_PATH, nullptr, nullptr);
        return std::string(out);
    }

    static std::string Asset(const char* rel) { return ExeDir() + "assets\\" + rel; }

    static void CreateRenderTarget() {
        ID3D11Texture2D* back = nullptr;
        g_swapchain->GetBuffer(0, IID_PPV_ARGS(&back));
        if (!back) return;
        g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
        back->Release();
    }

    static void CleanupRenderTarget() {
        if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    }

    static bool CreateDeviceD3D(HWND hwnd) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount       = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow      = hwnd;
        sd.SampleDesc.Count  = 1;
        sd.Windowed          = TRUE;
        sd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;
        const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL got;
        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                          levels, 2, D3D11_SDK_VERSION, &sd,
                                          &g_swapchain, &g_device, &got, &g_context) != S_OK)
            return false;
        CreateRenderTarget();
        return true;
    }

    static void KeyCheckerBackground(std::vector<unsigned char>& px, int w, int h) {
        const int n = w * h;
        auto is_bg = [&](int i) {
            const unsigned char* p = &px[(size_t)i * 4];
            int mn = p[0] < p[1] ? p[0] : p[1]; mn = mn < p[2] ? mn : p[2];
            int mx = p[0] > p[1] ? p[0] : p[1]; mx = mx > p[2] ? mx : p[2];
            return mn >= 226 && mx - mn <= 10;
        };
        std::vector<uint8_t> state((size_t)n, 0);
        std::vector<int> stack, comp;
        for (int start = 0; start < n; start++) {
            if (state[start] || !is_bg(start)) continue;
            stack.clear(); comp.clear();
            stack.push_back(start); state[start] = 1;
            bool edge = false;
            while (!stack.empty()) {
                int i = stack.back(); stack.pop_back();
                comp.push_back(i);
                int x = i % w, y = i / w;
                if (x == 0 || y == 0 || x == w - 1 || y == h - 1) edge = true;
                const int nb[4] = { i - 1, i + 1, i - w, i + w };
                const bool ok[4] = { x > 0, x < w - 1, y > 0, y < h - 1 };
                for (int k = 0; k < 4; k++) {
                    if (!ok[k] || state[nb[k]] || !is_bg(nb[k])) continue;
                    state[nb[k]] = 1;
                    stack.push_back(nb[k]);
                }
            }
            if (edge || comp.size() >= 400)
                for (int i : comp) state[i] = 2;
        }
        for (int i = 0; i < n; i++) if (state[i] == 2) px[(size_t)i * 4 + 3] = 0;
        for (int y = 1; y < h - 1; y++)
            for (int x = 1; x < w - 1; x++) {
                int i = y * w + x;
                if (state[i] == 2) continue;
                bool near_bg = false;
                for (int dy = -1; dy <= 1 && !near_bg; dy++)
                    for (int dx = -1; dx <= 1 && !near_bg; dx++)
                        if (state[i + dy * w + dx] == 2) near_bg = true;
                if (!near_bg) continue;
                unsigned char* p = &px[(size_t)i * 4];
                float lum = (p[0] + p[1] + p[2]) / 3.f;
                float a = (250.f - lum) / 50.f;
                a = a < 0.f ? 0.f : (a > 1.f ? 1.f : a);
                if (a > 0.05f)
                    for (int c = 0; c < 3; c++) {
                        float v = (p[c] - (1.f - a) * 248.f) / a;
                        p[c] = (unsigned char)(v < 0.f ? 0.f : (v > 255.f ? 255.f : v));
                    }
                p[3] = (unsigned char)(a * 255.f);
            }
    }

    static ImTextureID LoadTexture(const char* path_utf8, int* out_w, int* out_h, bool key_background = false) {
        wchar_t path[MAX_PATH];
        if (!MultiByteToWideChar(CP_UTF8, 0, path_utf8, -1, path, MAX_PATH)) return 0;
        IWICImagingFactory* factory = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) return 0;
        IWICBitmapDecoder* decoder = nullptr; IWICBitmapFrameDecode* frame = nullptr; IWICFormatConverter* conv = nullptr;
        std::vector<unsigned char> px; UINT w = 0, h = 0; bool ok = false;
        if (SUCCEEDED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder)) &&
            SUCCEEDED(decoder->GetFrame(0, &frame)) &&
            SUCCEEDED(factory->CreateFormatConverter(&conv)) &&
            SUCCEEDED(conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom)) &&
            SUCCEEDED(conv->GetSize(&w, &h)) && w > 0 && h > 0) {
            px.resize((size_t)w * h * 4);
            ok = SUCCEEDED(conv->CopyPixels(nullptr, w * 4, (UINT)px.size(), px.data()));
        }
        if (conv) conv->Release();
        if (frame) frame->Release();
        if (decoder) decoder->Release();
        factory->Release();
        if (!ok) return 0;
        if (key_background) KeyCheckerBackground(px, (int)w, (int)h);
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA init{ px.data(), w * 4, 0 };
        ID3D11Texture2D* tex = nullptr;
        if (FAILED(g_device->CreateTexture2D(&desc, &init, &tex))) return 0;
        D3D11_SHADER_RESOURCE_VIEW_DESC vd{};
        vd.Format = desc.Format; vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; vd.Texture2D.MipLevels = 1;
        ID3D11ShaderResourceView* srv = nullptr;
        g_device->CreateShaderResourceView(tex, &vd, &srv);
        tex->Release();
        if (!srv) return 0;
        g_textures.push_back(srv);
        if (out_w) *out_w = (int)w;
        if (out_h) *out_h = (int)h;
        return (ImTextureID)(uintptr_t)srv;
    }

    static ImFont* AddFontExact(const char* file, float base, float percent, const ImWchar* range) {
        ImGuiIO& io = ImGui::GetIO();
        const float size = base * percent / 100.f;
        ImFontConfig cfg;
        cfg.OversampleH = ui::theme.atlas.oversample_h;
        cfg.OversampleV = ui::theme.atlas.oversample_v;
        ImFont* f = io.Fonts->AddFontFromFileTTF(file, size, &cfg, range);
        if (!f) return io.Fonts->AddFontDefault();
        io.Fonts->Sources.back().SizePixels = size;
        return f;
    }

    static void ApplyCoverageCurve() {
        ImGuiIO& io = ImGui::GetIO();
        unsigned char lut[256];
        for (int i = 0; i < 256; ++i) {
            const float o = powf(i / 255.f, ui::theme.atlas.gamma) * 255.f + 0.5f;
            lut[i] = (unsigned char)(o > 255.f ? 255.f : o);
        }
        unsigned char* px = nullptr; int w = 0, h = 0;
        io.Fonts->GetTexDataAsAlpha8(&px, &w, &h);
        for (ImFont* f : io.Fonts->Fonts)
            for (const ImFontGlyph& g : f->Glyphs) {
                const int x0 = (int)(g.U0 * w + 0.5f), x1 = (int)(g.U1 * w + 0.5f);
                const int y0 = (int)(g.V0 * h + 0.5f), y1 = (int)(g.V1 * h + 0.5f);
                for (int y = y0; y < y1; ++y)
                    for (int x = x0; x < x1; ++x)
                        px[y * w + x] = lut[px[y * w + x]];
            }
    }

    static void LoadFonts(float percent) {
        ImGuiIO& io = ImGui::GetIO();
        static const ImWchar text_range[] = { 0x0020, 0x00FF, 0x2026, 0x2026, 0 };
        static ImVector<ImWchar> icon_range;
        if (icon_range.empty()) {
            namespace I = ui::icon;
            ImFontGlyphRangesBuilder b;
            for (const char* s : { I::eye, I::crosshair, I::sliders, I::gear, I::search, I::info, I::user, I::users, I::idcard, I::text_aa, I::tag,
                                   I::package, I::globe, I::box, I::person, I::palette, I::lightning, I::click, I::sword, I::monitor, I::keyboard, I::compass,
                                   I::minus, I::close, I::caret_down, I::check })
                b.AddText(s);
            b.BuildRanges(&icon_range);
        }
        namespace F = ui::fonts;
        const std::string geist = Asset("Geist-Regular.ttf");
        const std::string mono  = Asset("GeistMono-Medium.ttf");
        const std::string ph    = Asset("Phosphor-Bold.ttf");
        F::f14 = AddFontExact(geist.c_str(), 14.f, percent, text_range);
        F::f12 = AddFontExact(geist.c_str(), 12.f, percent, text_range);
        F::f13 = AddFontExact(geist.c_str(), 13.f, percent, text_range);
        F::f15 = AddFontExact(geist.c_str(), 15.f, percent, text_range);
        F::m12 = AddFontExact(mono.c_str(), 12.f, percent, text_range);
        F::m13 = AddFontExact(mono.c_str(), 13.f, percent, text_range);
        F::m14 = AddFontExact(mono.c_str(), 14.f, percent, text_range);
        F::i09 = AddFontExact(ph.c_str(), 9.f, percent, icon_range.Data);
        F::i12 = AddFontExact(ph.c_str(), 12.f, percent, icon_range.Data);
        F::i15 = AddFontExact(ph.c_str(), 15.f, percent, icon_range.Data);
        F::f18 = F::f15;
        io.Fonts->Build();
        ApplyCoverageCurve();
    }

    static ImVec2 WindowSize(float percent) {
        const ui::Layout& L = ui::theme.lay;
        return ImVec2(floorf(L.win_base_w * percent / 100.f + 0.5f), floorf(L.win_base_h * percent / 100.f + 0.5f));
    }

    static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) return true;
        switch (msg) {
        case WM_SIZE:
            if (g_device && wp != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_swapchain->ResizeBuffers(0, LOWORD(lp), HIWORD(lp), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wp & 0xfff0) == SC_KEYMENU) return 0;
            break;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    static ImVec2 FullSize() {
        const bool pv = pages::PreviewVisible(g_app);
        const float extra = pv ? ui::P(pages::preview_gap) + ui::P(pages::preview_w) : 0.f;
        return ImVec2(g_menu.x + extra, g_menu.y);
    }

    static void CenterWindow(HWND hwnd) {
        const int x = (GetSystemMetrics(SM_CXSCREEN) - (int)g_menu.x) / 2;
        const int y = (GetSystemMetrics(SM_CYSCREEN) - (int)g_menu.y) / 2;
        SetWindowPos(hwnd, HWND_TOPMOST, x, y, (int)g_win.x, (int)g_win.y, SWP_NOACTIVATE);
    }

    static void ApplyWindowSize() {
        const ImVec2 want = FullSize();
        if (want.x == g_win.x && want.y == g_win.y) return;
        g_win = want;
        SetWindowPos(g_hwnd, nullptr, 0, 0, (int)g_win.x, (int)g_win.y, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    static void HeaderDrag() {
        static POINT start_cursor{};
        static RECT  start_rect{};
        ImGui::SetCursorScreenPos(ImVec2(0.f, 0.f));
        ImGui::InvisibleButton("##drag", ImVec2(g_win.x, ui::P(pages::drag_h)));
        if (ImGui::IsItemActivated()) {
            GetCursorPos(&start_cursor);
            GetWindowRect(g_hwnd, &start_rect);
        }
        if (ImGui::IsItemActive()) {
            POINT p{};
            GetCursorPos(&p);
            SetWindowPos(g_hwnd, nullptr, start_rect.left + p.x - start_cursor.x, start_rect.top + p.y - start_cursor.y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    static void CreatePillTarget() {
        ID3D11Texture2D* back = nullptr;
        g_pill_sc->GetBuffer(0, IID_PPV_ARGS(&back));
        if (!back) return;
        g_device->CreateRenderTargetView(back, nullptr, &g_pill_rtv);
        back->Release();
    }

    static void CreatePillWindow() {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc); wc.style = CS_CLASSDC; wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandleW(nullptr); wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.lpszClassName = L"fanta_pill";
        RegisterClassExW(&wc);
        g_pill_w = (int)pages::PillWidth(g_app) + 2;
        g_pill_h = (int)ui::P(ui::theme.lay.pill_h) + 2;
        g_pill_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED,
                                      wc.lpszClassName, L"fanta overlay", WS_POPUP,
                                      (int)ui::theme.lay.pill_screen.x, (int)ui::theme.lay.pill_screen.y,
                                      g_pill_w, g_pill_h, nullptr, nullptr, wc.hInstance, nullptr);
        if (!g_pill_hwnd) return;
        SetLayeredWindowAttributes(g_pill_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        MARGINS m{ -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(g_pill_hwnd, &m);
        IDXGIDevice* dxgi = nullptr; IDXGIAdapter* adapter = nullptr; IDXGIFactory* factory = nullptr;
        g_device->QueryInterface(IID_PPV_ARGS(&dxgi));
        if (dxgi) dxgi->GetAdapter(&adapter);
        if (adapter) adapter->GetParent(IID_PPV_ARGS(&factory));
        if (factory) {
            DXGI_SWAP_CHAIN_DESC sd{};
            sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = g_pill_hwnd;
            sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            factory->CreateSwapChain(g_device, &sd, &g_pill_sc);
            factory->Release();
        }
        if (adapter) adapter->Release();
        if (dxgi) dxgi->Release();
        if (g_pill_sc) CreatePillTarget();
        ShowWindow(g_pill_hwnd, SW_SHOWNOACTIVATE);
    }

    static void RenderPill() {
        if (!g_pill_sc || !g_pill_rtv) return;
        const bool show = g_app.watermark;
        if (show != (IsWindowVisible(g_pill_hwnd) != FALSE))
            ShowWindow(g_pill_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
        if (!show) return;
        const int w = (int)pages::PillWidth(g_app) + 2, h = (int)ui::P(ui::theme.lay.pill_h) + 2;
        if (w != g_pill_w || h != g_pill_h) {
            g_pill_rtv->Release(); g_pill_rtv = nullptr;
            SetWindowPos(g_pill_hwnd, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            g_pill_sc->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
            CreatePillTarget();
            g_pill_w = w; g_pill_h = h;
        }
        ImDrawList dl(ImGui::GetDrawListSharedData());
        dl._ResetForNewFrame();
        dl.PushTextureID(ImGui::GetIO().Fonts->TexID);
        dl.PushClipRect(ImVec2(0, 0), ImVec2((float)w, (float)h));
        pages::DrawPill(&dl, ImVec2(0.f, 0.f), g_app, g_tex, (float)ImGui::GetTime(), false);
        dl._PopUnusedDrawCmd();
        ImDrawData dd;
        dd.Valid = true;
        dd.CmdListsCount = 1;
        dd.CmdLists.push_back(&dl);
        dd.TotalVtxCount = dl.VtxBuffer.Size;
        dd.TotalIdxCount = dl.IdxBuffer.Size;
        dd.DisplayPos = ImVec2(0, 0);
        dd.DisplaySize = ImVec2((float)w, (float)h);
        dd.FramebufferScale = ImVec2(1, 1);
        const float clear[4] = { 0.f, 0.f, 0.f, 0.f };
        g_context->OMSetRenderTargets(1, &g_pill_rtv, nullptr);
        g_context->ClearRenderTargetView(g_pill_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(&dd);
        g_pill_sc->Present(0, 0);
    }

    bool init() {
        g_com = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
        g_percent = g_app.ui_scale = g_app.applied_scale = 110.f;
        g_app.theme = 1;
        g_app.custom_accent = false;
        g_app.rainbow_accent = false;
        ui::SetScale(g_percent, true);
        g_menu = WindowSize(g_percent);
        g_win = FullSize();

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc); wc.style = CS_CLASSDC; wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr); wc.hCursor = LoadCursorW(nullptr, IDC_ARROW); wc.lpszClassName = L"fanta_menu";
        RegisterClassExW(&wc);
        const int x = (GetSystemMetrics(SM_CXSCREEN) - (int)g_menu.x) / 2;
        const int y = (GetSystemMetrics(SM_CYSCREEN) - (int)g_menu.y) / 2;
        g_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, wc.lpszClassName, L"fanta.lol",
                                 WS_POPUP, x, y, (int)g_win.x, (int)g_win.y, nullptr, nullptr, wc.hInstance, nullptr);
        if (!g_hwnd) return false;
        SetLayeredWindowAttributes(g_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        MARGINS m{ -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(g_hwnd, &m);

        if (!CreateDeviceD3D(g_hwnd)) return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().ConfigDebugHighlightIdConflicts = false;
        ImGui::StyleColorsDark();
        LoadFonts(g_percent);
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_device, g_context);

        g_tex.preview = LoadTexture(Asset("preview.jpg").c_str(), &g_tex.preview_w, &g_tex.preview_h, true);
        if (!g_tex.preview) logx::print("[fanta] failed to load assets\\preview.jpg\n");
        pages::Init(g_app);

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding    = ImVec2(0.f, 0.f);
        style.WindowBorderSize = 0.f;
        style.WindowRounding   = 0.f;
        style.ItemSpacing      = ImVec2(0.f, 0.f);
        style.Colors[ImGuiCol_WindowBg] = ui::theme.clear;

        CreatePillWindow();
        return true;
    }

    static void ForceForeground(HWND hwnd) {
        HWND fg = GetForegroundWindow();
        DWORD fg_thread = GetWindowThreadProcessId(fg, nullptr);
        DWORD my_thread = GetCurrentThreadId();
        if (fg_thread != my_thread) AttachThreadInput(fg_thread, my_thread, TRUE);
        ShowWindow(hwnd, SW_SHOW);
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
        if (fg_thread != my_thread) AttachThreadInput(fg_thread, my_thread, FALSE);
    }

    void toggle(HWND game) {
        g_visible = !g_visible;
        if (g_visible) {
            if (!g_positioned) { CenterWindow(g_hwnd); g_positioned = true; }
            ForceForeground(g_hwnd);
        } else {
            ShowWindow(g_hwnd, SW_HIDE);
            if (game) SetForegroundWindow(game);
        }
    }

    bool visible() { return g_visible; }

    bool quit_requested() { return g_quit; }

    void frame(HWND game) {
        if (!g_device) return;

        if (g_app.applied_scale != g_percent) {
            g_percent = g_app.applied_scale;
            ui::SetScale(g_percent, true);
            ImGui::GetIO().Fonts->Clear();
            LoadFonts(g_percent);
            ImGui_ImplDX11_InvalidateDeviceObjects();
            g_menu = WindowSize(g_percent);
        }
        ApplyWindowSize();

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ui::NewFrame();

        if (g_visible) {
            const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(g_win);
            ImGui::Begin("##menu", nullptr, flags);
            pages::Draw(g_app, g_tex, g_menu);
            if (pages::PreviewVisible(g_app) && g_win.x > g_menu.x)
                pages::DrawPreview(g_tex, ImVec2(g_menu.x + ui::P(pages::preview_gap), 0.f), ImVec2(ui::P(pages::preview_w), g_menu.y));
            HeaderDrag();
            ImGui::End();
        }

        ImGui::Render();
        if (g_visible) {
            const float clear[4] = { 0.f, 0.f, 0.f, 0.f };
            g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_context->ClearRenderTargetView(g_rtv, clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_swapchain->Present(0, 0);
        }

        RenderPill();

        if (g_app.request_close) { g_app.request_close = false; g_quit = true; }
        if (g_app.request_minimize) { g_app.request_minimize = false; if (g_visible) toggle(game); }
    }

    void shutdown() {
        if (!g_device) return;
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        for (ID3D11ShaderResourceView* t : g_textures) t->Release();
        if (g_pill_rtv) g_pill_rtv->Release();
        if (g_pill_sc)  g_pill_sc->Release();
        if (g_pill_hwnd) DestroyWindow(g_pill_hwnd);
        CleanupRenderTarget();
        if (g_swapchain) g_swapchain->Release();
        if (g_context)   g_context->Release();
        if (g_device)    g_device->Release();
        if (g_hwnd) DestroyWindow(g_hwnd);
        g_device = nullptr;
        if (g_com) { CoUninitialize(); g_com = false; }
    }
}
