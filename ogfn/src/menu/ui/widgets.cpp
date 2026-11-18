
#include "widgets.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ui {

static float g_percent = 110.f, g_k = 1.f;
static bool  g_snap = true;

void  SetScale(float percent, bool snap) { g_percent = percent; g_k = percent / 110.f; g_snap = snap; }
float ScalePercent() { return g_percent; }
bool  Snapped() { return g_snap; }
float P(float v) { const float o = v * g_k; return g_snap ? floorf(o + 0.5f) : o; }
float PF(float v) { return v * g_k; }
float TextX(float x) { return g_snap ? floorf(x + 0.5f) : x; }

float RowY(float y0, int i)
{
    const float base = theme.lay.row_base * g_percent / 100.f;
    return y0 + static_cast<float>(i) * (g_snap ? floorf(base + 0.5f) : base);
}

ImU32 U32(const ImVec4& c, float alpha)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alpha * ImGui::GetStyle().Alpha));
}
ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

void FillAA(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col)
{
    dl->PathRect(a, b, 0.f, 0);
    dl->PathFillConvex(col);
}

void TextAt(ImDrawList* dl, ImFont* f, const ImVec2& pos, ImU32 col, const char* text, const char* end)
{
    if (g_snap)
    {
        dl->AddText(f, f->FontSize, pos, col, text, end);
        return;
    }

    float x = pos.x;
    if (!end) end = text + strlen(text);
    dl->PushTextureID(f->ContainerAtlas->TexID);
    for (const char* s = text; s < end; )
    {
        unsigned int c = static_cast<unsigned char>(*s);
        if (c < 0x80) s += 1; else s += ImTextCharFromUtf8(&c, s, end);
        const ImFontGlyph* g = f->FindGlyph(static_cast<ImWchar>(c));
        if (!g) continue;
        if (g->Visible)
        {
            dl->PrimReserve(6, 4);
            dl->PrimRectUV(ImVec2(x + g->X0, pos.y + g->Y0), ImVec2(x + g->X1, pos.y + g->Y1), ImVec2(g->U0, g->V0), ImVec2(g->U1, g->V1), col);
        }
        x += g->AdvanceX;
    }
    dl->PopTextureID();
}

ImVec2 TextSize(ImFont* f, const char* text, const char* end)
{
    return f->CalcTextSizeA(f->FontSize, FLT_MAX, 0.f, text, end);
}

static char g_filter[64] = "";

void SetFilter(const char* query)
{
    ImStrncpy(g_filter, query, IM_ARRAYSIZE(g_filter));
}

static float FilterAlpha(const char* label)
{
    if (!g_filter[0]) return 1.f;
    return ImStristr(label, nullptr, g_filter, nullptr) ? 1.f : 0.3f;
}

void NewFrame()
{
    AnimNewFrame();
}

bool Hotspot(const char* id, const ImVec2& mn, const ImVec2& mx, bool* hovered)
{
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiID gid = w->GetID(id);
    const ImRect bb(mn, mx);
    ImGui::ItemSize(ImVec2(0.f, 0.f));
    if (!ImGui::ItemAdd(bb, gid)) { if (hovered) *hovered = false; return false; }
    bool hov = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bb, gid, &hov, &held);
    if (hovered) *hovered = hov;
    return pressed;
}

static float Ramp(ImGuiID id, int slot, float target, float speed)
{
    AnimState& st = GetAnim(id);
    if (!st.initialized) { for (float& v : st.f) v = 0.f; st.f[slot] = target; st.initialized = true; }
    return Approach(st.f[slot], target, speed);
}

struct PanelState { float x0, x1, y0, y1; int row; float row0; bool plain; };
static PanelState g_panel;

float PanelHeight(bool plain, int rows)
{
    const Layout& L = theme.lay;
    const float row0 = plain ? L.plain_row0 : L.strip_row0;
    const float end = plain ? L.panel_end : L.strip_end;
    return P(row0) + (RowY(0.f, rows - 1)) + P(end);
}

void BeginPanel(const PanelDesc& d, float x0, float x1, float screen_y0, int rows)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float sx0 = P(x0), sx1 = P(x1), sy0 = screen_y0;
    const float sy1 = sy0 + PanelHeight(d.plain, rows);
    const float r = PF(L.panel_rounding);

    dl->AddRectFilled(ImVec2(sx0, sy0), ImVec2(sx1, sy1), U32(C.panel), r);
    if (!d.plain)
    {

        const float ly = sy0 + P(L.strip_h);
        ImVec4 left = Mix(C.panel, theme.accent, L.strip_tint);
        left.y = ImMax(0.f, left.y - 2.f / 255.f); left.z = ImMax(0.f, left.z - 2.f / 255.f);
        const ImU32 cl = U32(left), cr = U32(C.panel);

        dl->PushClipRect(ImVec2(sx0, sy0), ImVec2(sx1, ly), true);
        dl->PathRect(ImVec2(sx0, sy0), ImVec2(sx1, sy1), r);
        const int vtx0 = dl->VtxBuffer.Size;
        dl->PathFillConvex(cl);
        const int vtx1 = dl->VtxBuffer.Size;
        ImGui::ShadeVertsLinearColorGradientKeepAlpha(dl, vtx0, vtx1, ImVec2(sx0, sy0), ImVec2(sx1, sy0), cl, cr);
        dl->PopClipRect();
        dl->AddLine(ImVec2(sx0 + 1.f, ly), ImVec2(sx1 - 1.f, ly), U32(C.strip_line), 1.f);
    }
    dl->AddRect(ImVec2(sx0, sy0), ImVec2(sx1, sy1), U32(C.panel_border), r, 0, 1.f);

    float tx, ty;
    ImFont* tf = fonts::f14;
    if (d.plain)
    {
        tx = sx0 + P(L.panel_pad); ty = sy0 + P(L.plain_title_dy);
    }
    else
    {
        tx = sx0 + P(L.strip_title_x); ty = sy0 + P(L.strip_title_dy);
        tx += P(d.title_dx);
        if (d.icon) TextAt(dl, fonts::i12, ImVec2(sx0 + P(L.strip_icon_x), sy0 + P(L.strip_icon_dy)), U32(theme.accent), d.icon);
    }
    TextAt(dl, tf, ImVec2(tx, ty), U32(C.text), d.title);
    if (d.info)
        TextAt(dl, fonts::i12, ImVec2(TextX(tx + TextSize(tf, d.title).x + PF(L.info_gap)), ty + P(L.info_dy)), U32(C.faint), icon::info);

    g_panel.x0 = sx0; g_panel.x1 = sx1; g_panel.y0 = sy0; g_panel.y1 = sy1; g_panel.row = 0; g_panel.plain = d.plain;
    g_panel.row0 = sy0 + P(d.plain ? L.plain_row0 : L.strip_row0);
    ImGui::PushID(d.title);
}

float EndPanel()
{
    ImGui::PopID();
    return g_panel.y1;
}

static float NextRow()
{
    return RowY(g_panel.row0, g_panel.row++);
}

static float LabelX() { return g_panel.x0 + P(theme.lay.panel_pad); }
static float RightX() { return g_panel.x1 - P(theme.lay.check_inset); }

static void Label(ImDrawList* dl, const char* label, float y, const ImVec4& col, float alpha)
{
    const char* end = ImGui::FindRenderedTextEnd(label);
    TextAt(dl, fonts::f14, ImVec2(LabelX(), y + PF(theme.lay.row_text_dy)), U32(col, alpha), label, end);
}

static void DrawCheck(ImDrawList* dl, const ImVec2& mn, const ImVec2& mx, float on, float hover, float alpha)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    const float r = PF(L.check_rounding);
    if (on < 1.f)
    {
        dl->AddRectFilled(mn, mx, U32(Mix(C.field, C.field_hover, hover), alpha), r);
        dl->AddRect(mn, mx, U32(C.field_border, alpha), r, 0, 1.f);
    }
    if (on > 0.f)
    {
        dl->AddRectFilled(mn, mx, U32(theme.accent, on * alpha), r);
        TextAt(dl, fonts::i09, mn + ImVec2(PF(L.check_glyph), PF(L.check_glyph)), U32(theme.check_mark, on * alpha), icon::check);
    }
}

static void SwatchButton(ImDrawList* dl, const char* id, ImVec4* col, const ImVec2& mn, const ImVec2& mx, float alpha)
{
    bool hovered = false;
    if (Hotspot(id, mn, mx, &hovered))
        ImGui::OpenPopup(id);
    dl->AddRectFilled(mn, mx, U32(*col, alpha), PF(theme.lay.swatch_rounding));
    if (hovered)
        dl->AddRect(mn, mx, U32(theme.pal.text, 0.5f * alpha), PF(theme.lay.swatch_rounding), 0, 1.f);

    const Palette& C = theme.pal;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(P(theme.lay.picker_pad), P(theme.lay.picker_pad)));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, PF(theme.lay.frame_rounding));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C.field);
    ImGui::PushStyleColor(ImGuiCol_Border, C.field_border);
    ImGui::SetNextWindowPos(ImVec2(mx.x + P(theme.lay.picker_gap), mn.y), ImGuiCond_Appearing);
    if (ImGui::BeginPopup(id, ImGuiWindowFlags_NoMove))
    {
        ImGui::PushFont(fonts::f12);
        ImGui::SetNextItemWidth(P(theme.lay.picker_w));
        ImGui::ColorPicker3("##pick", &col->x, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
        ImGui::PopFont();
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

bool CheckRow(const char* label, bool* v, ImVec4* swatch)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    const float size = P(L.check_size);
    const float x1 = RightX();
    const float alpha = FilterAlpha(label);
    const ImVec2 mn(x1 - size, y), mx(x1, y + size);
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiID id = w->GetID(label);

    bool hovered = false;

    const float row_x1 = swatch ? (mn.x - P(L.swatch_gap) - 1.f) : x1;
    bool pressed = Hotspot(label, ImVec2(LabelX(), y - P(L.frame_dy)), ImVec2(row_x1, y + P(L.frame_h) - P(L.frame_dy)), &hovered);
    if (swatch)
    {
        ImGui::PushID(label);
        bool h2 = false;
        pressed |= Hotspot("##box", mn, mx, &h2);
        hovered |= h2;
        ImGui::PopID();
    }
    if (pressed) *v = !*v;

    const float on = Ramp(id, 0, *v ? 1.f : 0.f, theme.motion.check);
    const float hv = Ramp(id, 1, hovered ? 1.f : 0.f, theme.motion.speed);
    Label(dl, label, y, Mix(C.text_dim, C.text, ImClamp(on + hv * 0.35f, 0.f, 1.f)), alpha);
    DrawCheck(dl, mn, mx, on, hv, alpha);
    if (swatch)
    {
        const ImVec2 smn(mn.x - P(L.swatch_gap), y), smx(smn.x + size, y + size);
        ImGui::PushID(label);
        SwatchButton(dl, "##swatch", swatch, smn, smx, alpha);
        ImGui::PopID();
    }
    return pressed;
}

void SwatchRow(const char* label, ImVec4* swatch)
{
    const Layout& L = theme.lay;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    const float size = P(L.check_size);
    const float x1 = RightX();
    const float alpha = FilterAlpha(label);
    Label(dl, label, y, theme.pal.text_dim, alpha);
    ImGui::PushID(label);
    SwatchButton(dl, "##swatch", swatch, ImVec2(x1 - size, y), ImVec2(x1, y + size), alpha);
    ImGui::PopID();
}

bool SliderRow(const char* label, float* v, float v_min, float v_max, const char* fmt, ImFont* value_font)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    const float alpha = FilterAlpha(label);
    Label(dl, label, y, C.text_dim, alpha);

    const float x1 = g_panel.x1 - P(L.track_inset);
    const float x0 = x1 - P(L.track_w);
    const float ty = y + P(L.track_dy), th = P(L.track_h);

    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiID id = w->GetID(label);
    const ImRect bb(ImVec2(x0 - P(L.hit_slop), y - P(L.frame_dy)), ImVec2(x1 + P(L.hit_slop), y + P(L.frame_h) - P(L.frame_dy)));
    ImGui::ItemSize(ImVec2(0.f, 0.f));
    bool hovered = false, held = false, changed = false;
    if (ImGui::ItemAdd(bb, id))
        ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (held && ImGui::IsMousePosValid())
    {
        const float t = ImSaturate((ImGui::GetIO().MousePos.x - x0) / (x1 - x0));
        const float nv = v_min + (v_max - v_min) * t;
        changed = nv != *v;
        *v = nv;
    }
    const float t = ImSaturate((*v - v_min) / (v_max - v_min));
    const float hv = Ramp(id, 1, (hovered || held) ? 1.f : 0.f, theme.motion.speed);

    FillAA(dl, ImVec2(x0, ty), ImVec2(x1, ty + th), U32(Mix(C.track, C.field_border, hv * 0.6f), alpha));
    FillAA(dl, ImVec2(x0, ty), ImVec2(x0 + (x1 - x0) * t, ty + th), U32(theme.accent, alpha));

    char buf[32];
    ImFormatString(buf, IM_ARRAYSIZE(buf), fmt, *v);
    ImFont* vf = value_font ? value_font : fonts::m13;
    const ImVec2 ts = TextSize(vf, buf);
    TextAt(dl, vf, ImVec2(TextX(x0 - PF(L.value_gap) - ts.x), y + PF(L.row_text_dy)), U32(C.value, alpha), buf);
    return changed;
}

bool ComboRow(const char* label, int* cur, const char* const items[], int count)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    const float alpha = FilterAlpha(label);
    Label(dl, label, y, C.text_dim, alpha);

    const float x1 = RightX(), x0 = x1 - P(L.combo_w);
    const float fy0 = y - P(L.frame_dy), fy1 = fy0 + P(L.frame_h);
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    const ImGuiID id = w->GetID(label);
    bool hovered = false;
    if (Hotspot(label, ImVec2(x0, fy0), ImVec2(x1, fy1), &hovered))
        ImGui::OpenPopup(id);
    const bool open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    const float hv = Ramp(id, 1, (hovered || open) ? 1.f : 0.f, theme.motion.speed);
    const float r = PF(L.frame_rounding);
    dl->AddRectFilled(ImVec2(x0, fy0), ImVec2(x1, fy1), U32(Mix(C.field, C.field_hover, hv), alpha), r);
    dl->AddRect(ImVec2(x0, fy0), ImVec2(x1, fy1), U32(C.field_border, alpha), r, 0, 1.f);
    TextAt(dl, fonts::f12, ImVec2(x0 + P(L.combo_text_x), y + P(L.combo_text_dy)), U32(C.value, alpha), items[*cur]);

    TextAt(dl, fonts::i09, ImVec2(x0 + P(L.chevron_x), y + P(L.chevron_dy)), U32(C.text_dim, alpha), icon::caret_down);

    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(x0, fy1 + P(L.popup_gap)));
    ImGui::SetNextWindowSize(ImVec2(x1 - x0, P(L.popup_item_h) * count + P(L.popup_pad) * 2.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, r);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, C.field);
    ImGui::PushStyleColor(ImGuiCol_Border, C.field_border);
    if (ImGui::BeginPopupEx(id, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        ImDrawList* pd = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        for (int i = 0; i < count; ++i)
        {
            ImGui::PushID(i);
            const float iy = wp.y + P(L.popup_pad) + P(L.popup_item_h) * i;
            bool item_hov = false;
            const ImVec2 imn(wp.x + P(L.popup_pad), iy), imx(wp.x + (x1 - x0) - P(L.popup_pad), iy + P(L.popup_item_h));
            if (Hotspot("##item", imn, imx, &item_hov))
            {
                *cur = i; changed = true;
                ImGui::CloseCurrentPopup();
            }
            if (item_hov) pd->AddRectFilled(imn, imx, U32(C.field_hover), r * 0.7f);
            const ImU32 tc = (i == *cur) ? U32(theme.accent) : U32(item_hov ? C.text : C.value);
            TextAt(pd, fonts::f12, ImVec2(wp.x + P(L.combo_text_x), iy + (P(L.popup_item_h) - fonts::f12->FontSize) * 0.5f), tc, items[i]);
            ImGui::PopID();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    return changed;
}

bool InputRow(const char* label, char* buf, int buf_size)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    const float alpha = FilterAlpha(label);
    Label(dl, label, y, C.text_dim, alpha);

    const float x1 = RightX(), x0 = x1 - P(L.input_w);
    const float fy0 = y - P(L.frame_dy), fy1 = fy0 + P(L.frame_h);
    const float r = PF(L.frame_rounding);
    dl->AddRectFilled(ImVec2(x0, fy0), ImVec2(x1, fy1), U32(C.field, alpha), r);
    dl->AddRect(ImVec2(x0, fy0), ImVec2(x1, fy1), U32(C.field_border, alpha), r, 0, 1.f);

    ImGui::PushFont(fonts::f13);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme.clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme.clear);
    ImGui::PushStyleColor(ImGuiCol_Text, C.text);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, Mix(C.field, theme.accent, 0.45f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(P(L.combo_text_x), (fy1 - fy0 - fonts::f13->FontSize) * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
    ImGui::SetCursorScreenPos(ImVec2(x0, fy0));
    ImGui::SetNextItemWidth(x1 - x0);
    ImGui::PushID(label);
    const bool changed = ImGui::InputText("##in", buf, static_cast<size_t>(buf_size));
    ImGui::PopID();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    ImGui::PopFont();
    return changed;
}

bool Button(const char* id, const char* text, float x, float y, float w, float h)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID gid = win->GetID(id);
    const ImRect bb(ImVec2(x, y), ImVec2(x + w, y + h));
    ImGui::ItemSize(ImVec2(0.f, 0.f));
    bool hovered = false, held = false, pressed = false;
    if (ImGui::ItemAdd(bb, gid))
        pressed = ImGui::ButtonBehavior(bb, gid, &hovered, &held);
    const float hv = Ramp(gid, 1, hovered ? 1.f : 0.f, theme.motion.speed);
    const float dn = Ramp(gid, 2, held ? 1.f : 0.f, theme.motion.speed * 2.f);
    const float r = PF(L.frame_rounding);
    const ImVec4 fill = Mix(Mix(C.field, C.field_hover, hv), C.field, dn);
    dl->AddRectFilled(bb.Min, bb.Max, U32(fill), r);
    dl->AddRect(bb.Min, bb.Max, U32(Mix(C.field_border, theme.accent, dn * 0.6f)), r, 0, 1.f);
    const ImVec2 ts = TextSize(fonts::f13, text, ImGui::FindRenderedTextEnd(text));
    TextAt(dl, fonts::f13, ImVec2(TextX(x + (w - ts.x) * 0.5f), y + P(L.frame_dy)), U32(Mix(C.value, C.text, hv)), text, ImGui::FindRenderedTextEnd(text));
    return pressed;
}

bool ButtonRow(const char* label, const char* button)
{
    const Layout& L = theme.lay;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    Label(dl, label, y, theme.pal.text_dim, FilterAlpha(label));
    const float x1 = RightX(), w = P(L.button_w);
    ImGui::PushID(label);
    const bool pressed = Button("##btn", button, x1 - w, y - P(L.frame_dy), w, P(L.frame_h));
    ImGui::PopID();
    return pressed;
}

const char* KeyName(int key)
{
    if (key == ImGuiKey_None) return "None";
    if (key == ImGuiKey_MouseLeft) return "LMB";
    if (key == ImGuiKey_MouseRight) return "RMB";
    if (key == ImGuiKey_MouseMiddle) return "MMB";
    if (key == ImGuiKey_MouseX1) return "M4";
    if (key == ImGuiKey_MouseX2) return "M5";
    return ImGui::GetKeyName(static_cast<ImGuiKey>(key));
}

bool KeyRow(const char* label, int* key)
{
    const Layout& L = theme.lay;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float y = NextRow();
    Label(dl, label, y, theme.pal.text_dim, FilterAlpha(label));
    const float x1 = RightX(), w = P(L.config_button_w);

    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiID id = win->GetID(label);
    static ImGuiID s_listening = 0;
    bool changed = false;
    const char* shown = (s_listening == id) ? "..." : KeyName(*key);
    ImGui::PushID(label);
    if (Button("##key", shown, x1 - w, y - P(L.frame_dy), w, P(L.frame_h)) && s_listening == 0)
        s_listening = id;
    else if (s_listening == id)
    {

        for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k)
        {
            if (k >= ImGuiKey_ReservedForModCtrl) break;
            if (!ImGui::IsKeyPressed(static_cast<ImGuiKey>(k), false)) continue;
            if (k == ImGuiKey_Escape) { s_listening = 0; break; }
            if (k == ImGuiKey_MouseLeft) continue;
            *key = k; changed = true; s_listening = 0;
            break;
        }
    }
    ImGui::PopID();
    return changed;
}

bool SearchInput(char* buf, int buf_size)
{
    const Layout& L = theme.lay;
    const Palette& C = theme.pal;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 o = P(L.search_text);
    ImFont* f = fonts::f14;
    if (!buf[0])
        TextAt(dl, f, o, U32(C.placeholder), "Search...");
    ImGui::PushFont(f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, theme.clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, theme.clear);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, theme.clear);
    ImGui::PushStyleColor(ImGuiCol_Text, C.text);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, Mix(C.field, theme.accent, 0.45f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::SetCursorScreenPos(o);
    ImGui::SetNextItemWidth(P(L.search_x1) - o.x);
    const bool changed = ImGui::InputText("##search", buf, static_cast<size_t>(buf_size));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(5);
    ImGui::PopFont();
    return changed;
}

}
