#pragma once
#include <Windows.h>
#undef DrawText
#include <dwmapi.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dxgi.h>
#include <cstdint>
#include <cwchar>
#include <cmath>
#include "../core/log.h"

namespace overlay {
    inline HWND hwnd     = nullptr;
    inline HWND target   = nullptr;
    inline int  width    = 0;
    inline int  height   = 0;
    inline int  screen_x = 0;
    inline int  screen_y = 0;
    inline bool running  = true;

    inline ID3D11Device*         device  = nullptr;
    inline ID3D11DeviceContext*  context = nullptr;
    inline IDXGISwapChain*       swap    = nullptr;
    inline ID2D1Factory*         d2d     = nullptr;
    inline IDWriteFactory*       dwrite  = nullptr;
    inline IDWriteTextFormat*    format  = nullptr;
    inline IDWriteTextFormat*    format_center = nullptr;
    inline IDWriteTextFormat*    format_right  = nullptr;
    inline float                 font_size     = 13.f;
    inline ID2D1RenderTarget*    rt      = nullptr;
    inline ID2D1SolidColorBrush* brush   = nullptr;

    template <class T>
    inline void release(T*& p) {
        if (p) { p->Release(); p = nullptr; }
    }

    inline LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
        if (m == WM_DESTROY || m == WM_CLOSE) { running = false; PostQuitMessage(0); return 0; }
        return DefWindowProcW(h, m, w, l);
    }

    inline bool create_target() {
        IDXGISurface* surface = nullptr;
        if (FAILED(swap->GetBuffer(0, __uuidof(IDXGISurface), (void**)&surface))) return false;
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.f, 96.f);
        HRESULT hr = d2d->CreateDxgiSurfaceRenderTarget(surface, &props, &rt);
        surface->Release();
        if (FAILED(hr)) return false;
        if (FAILED(rt->CreateSolidColorBrush(D2D1::ColorF(1.f, 1.f, 1.f, 1.f), &brush))) return false;
        rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        return true;
    }

    inline void destroy_target() {
        release(brush);
        release(rt);
    }

    inline bool make_format(float size, DWRITE_TEXT_ALIGNMENT align, IDWriteTextFormat*& out) {
        if (FAILED(dwrite->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_SEMI_BOLD,
                DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &out))) return false;
        out->SetTextAlignment(align);
        out->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        return true;
    }

    inline bool create_formats(float size) {
        IDWriteTextFormat *l = nullptr, *c = nullptr, *r = nullptr;
        if (!make_format(size, DWRITE_TEXT_ALIGNMENT_LEADING, l) ||
            !make_format(size, DWRITE_TEXT_ALIGNMENT_CENTER, c) ||
            !make_format(size, DWRITE_TEXT_ALIGNMENT_TRAILING, r)) {
            release(l); release(c); release(r);
            return false;
        }
        release(format); release(format_center); release(format_right);
        format = l; format_center = c; format_right = r;
        font_size = size;
        return true;
    }

    inline void set_font_size(float size) {
        if (size < 6.f) size = 6.f;
        if (fabsf(size - font_size) < 0.5f) return;
        create_formats(size);
    }

    inline bool measure_target(int& x, int& y, int& w, int& h) {
        if (!IsWindow(target)) return false;
        RECT r{};
        if (!GetClientRect(target, &r)) return false;
        POINT tl{ 0, 0 };
        if (!ClientToScreen(target, &tl)) return false;
        x = tl.x;
        y = tl.y;
        w = r.right - r.left;
        h = r.bottom - r.top;
        return w > 0 && h > 0;
    }

    inline bool create(HWND tgt) {
        target = tgt;

        int x = 0, y = 0, w = 0, h = 0;
        if (!measure_target(x, y, w, h)) {
            x = 0;
            y = 0;
            w = GetSystemMetrics(SM_CXSCREEN);
            h = GetSystemMetrics(SM_CYSCREEN);
        }

        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = wndproc;
        wc.hInstance     = GetModuleHandleW(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"ogfn_overlay";
        RegisterClassExW(&wc);

        hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            wc.lpszClassName, L"ogfn", WS_POPUP,
            x, y, w, h, nullptr, nullptr, wc.hInstance, nullptr);
        if (!hwnd) { logx::print("[overlay] CreateWindowEx failed\n"); return false; }

        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
        MARGINS margins{ -1, -1, -1, -1 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);

        screen_x = x;
        screen_y = y;
        width    = w;
        height   = h;

        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount        = 1;
        sd.BufferDesc.Width   = (UINT)w;
        sd.BufferDesc.Height  = (UINT)h;
        sd.BufferDesc.Format  = DXGI_FORMAT_B8G8R8A8_UNORM;
        sd.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow       = hwnd;
        sd.SampleDesc.Count   = 1;
        sd.Windowed           = TRUE;
        sd.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL level{};
        HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
            &sd, &swap, &device, &level, &context);
        if (FAILED(hr)) { logx::print("[overlay] D3D11CreateDeviceAndSwapChain failed 0x%X\n", (unsigned)hr); return false; }

        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d);
        if (FAILED(hr)) { logx::print("[overlay] D2D1CreateFactory failed 0x%X\n", (unsigned)hr); return false; }

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwrite);
        if (FAILED(hr)) { logx::print("[overlay] DWriteCreateFactory failed 0x%X\n", (unsigned)hr); return false; }

        if (!create_formats(font_size)) { logx::print("[overlay] CreateTextFormat failed\n"); return false; }

        if (!create_target()) { logx::print("[overlay] create_target failed\n"); return false; }
        return true;
    }

    inline void shutdown() {
        destroy_target();
        release(format_right);
        release(format_center);
        release(format);
        release(dwrite);
        release(d2d);
        release(swap);
        release(context);
        release(device);
        if (hwnd) { DestroyWindow(hwnd); hwnd = nullptr; }
    }

    inline bool sync() {
        int x = 0, y = 0, w = 0, h = 0;
        if (!IsWindow(target)) { running = false; return false; }
        if (!measure_target(x, y, w, h)) return false;
        bool moved   = x != screen_x || y != screen_y;
        bool resized = w != width || h != height;
        if (!moved && !resized) return true;

        screen_x = x;
        screen_y = y;
        SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);

        if (resized) {
            width  = w;
            height = h;
            destroy_target();
            HRESULT hr = swap->ResizeBuffers(0, (UINT)w, (UINT)h, DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr) || !create_target()) {
                logx::print("[overlay] resize failed 0x%X\n", (unsigned)hr);
                return false;
            }
        }
        return true;
    }

    inline void pump() {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    inline void begin_frame() {
        rt->BeginDraw();
        rt->Clear(D2D1::ColorF(0.f, 0.f, 0.f, 0.f));
    }

    inline void end_frame() {
        HRESULT hr = rt->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            destroy_target();
            create_target();
        }
        swap->Present(1, 0);
    }

    namespace draw {
        inline uint32_t argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
            return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }

        inline void set_color(uint32_t c) {
            brush->SetColor(D2D1::ColorF(
                ((c >> 16) & 0xFF) / 255.f,
                ((c >> 8) & 0xFF) / 255.f,
                (c & 0xFF) / 255.f,
                ((c >> 24) & 0xFF) / 255.f));
        }

        inline void line(float x0, float y0, float x1, float y1, uint32_t c, float thickness = 1.f) {
            set_color(c);
            rt->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, thickness);
        }

        inline void box(float x, float y, float w, float h, uint32_t c, float thickness = 1.f) {
            set_color(c);
            rt->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), brush, thickness);
        }

        inline void filled_box(float x, float y, float w, float h, uint32_t c) {
            set_color(c);
            rt->FillRectangle(D2D1::RectF(x, y, x + w, y + h), brush);
        }

        inline void circle(float cx, float cy, float r, uint32_t c, float thickness = 1.f) {
            set_color(c);
            rt->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), brush, thickness);
        }

        inline void filled_circle(float cx, float cy, float r, uint32_t c) {
            set_color(c);
            rt->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), r, r), brush);
        }

        inline void filled_triangle(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c) {
            ID2D1PathGeometry* geo = nullptr;
            if (FAILED(d2d->CreatePathGeometry(&geo))) return;
            ID2D1GeometrySink* sink = nullptr;
            if (SUCCEEDED(geo->Open(&sink))) {
                sink->BeginFigure(D2D1::Point2F(x0, y0), D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(D2D1::Point2F(x1, y1));
                sink->AddLine(D2D1::Point2F(x2, y2));
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                sink->Release();
                set_color(c);
                rt->FillGeometry(geo, brush);
            }
            geo->Release();
        }

        enum class align { left, center, right };

        inline float text_bg      = 0.f;
        inline bool  text_outline = true;

        inline void label(float x, float y, const wchar_t* s, uint32_t c, align a) {
            if (!s || !s[0]) return;
            UINT32 len = (UINT32)wcslen(s);
            IDWriteTextFormat* f = a == align::left ? format : (a == align::center ? format_center : format_right);
            D2D1_RECT_F r = a == align::left   ? D2D1::RectF(x, y, x + 1200.f, y + 80.f)
                          : a == align::center ? D2D1::RectF(x - 600.f, y, x + 600.f, y + 80.f)
                                               : D2D1::RectF(x - 1200.f, y, x, y + 80.f);
            const uint8_t alpha = (uint8_t)((c >> 24) & 0xFF);
            if (text_bg > 0.f) {
                IDWriteTextLayout* lay = nullptr;
                if (SUCCEEDED(dwrite->CreateTextLayout(s, len, f, r.right - r.left, r.bottom - r.top, &lay))) {
                    DWRITE_TEXT_METRICS m{};
                    lay->GetMetrics(&m);
                    float bx = r.left + m.left - 3.f, by = r.top + m.top;
                    set_color(argb((uint8_t)(text_bg * alpha), 0, 0, 0));
                    rt->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(bx, by, bx + m.width + 6.f, by + m.height), 3.f, 3.f), brush);
                    lay->Release();
                }
            }
            if (text_outline) {
                set_color(argb((uint8_t)(alpha * 0.86f), 0, 0, 0));
                static const float off[4][2] = { { -1.f, 0.f }, { 1.f, 0.f }, { 0.f, -1.f }, { 0.f, 1.f } };
                for (auto& o : off)
                    rt->DrawText(s, len, f, D2D1::RectF(r.left + o[0], r.top + o[1], r.right + o[0], r.bottom + o[1]), brush);
            }
            set_color(c);
            rt->DrawText(s, len, f, r, brush);
        }

        inline void text(float x, float y, const wchar_t* s, uint32_t c) {
            label(x, y, s, c, align::left);
        }

        inline void text_centered(float cx, float y, const wchar_t* s, uint32_t c) {
            label(cx, y, s, c, align::center);
        }
    }
}
