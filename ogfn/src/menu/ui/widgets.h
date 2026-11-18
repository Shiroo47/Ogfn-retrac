
#pragma once
#include "imgui.h"
#include "theme.h"
#include "anim.h"

namespace ui {

void NewFrame();

void  SetScale(float percent, bool snap);
float ScalePercent();
bool  Snapped();
float P(float v);
float PF(float v);
float TextX(float x);
float RowY(float y0, int i);
inline ImVec2 P(const ImVec2& v) { return ImVec2(P(v.x), P(v.y)); }

ImU32 U32(const ImVec4& c, float alpha = 1.f);
ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t);

void FillAA(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col);

void   TextAt(ImDrawList* dl, ImFont* f, const ImVec2& pos, ImU32 col, const char* text, const char* end = nullptr);
ImVec2 TextSize(ImFont* f, const char* text, const char* end = nullptr);

void SetFilter(const char* query);

struct PanelDesc { const char* title; const char* icon; bool info; bool plain; float title_dx = 0.f; };
void BeginPanel(const PanelDesc& d, float x0, float x1, float screen_y0, int rows);
float EndPanel();
float PanelHeight(bool plain, int rows);

bool CheckRow(const char* label, bool* v, ImVec4* swatch = nullptr);
void SwatchRow(const char* label, ImVec4* swatch);
bool SliderRow(const char* label, float* v, float v_min, float v_max, const char* fmt, ImFont* value_font = nullptr);
bool ComboRow(const char* label, int* cur, const char* const items[], int count);
bool InputRow(const char* label, char* buf, int buf_size);
bool ButtonRow(const char* label, const char* button);
bool KeyRow(const char* label, int* key);

bool Button(const char* id, const char* text, float x, float y, float w, float h);
bool SearchInput(char* buf, int buf_size);
bool Hotspot(const char* id, const ImVec2& mn, const ImVec2& mx, bool* hovered = nullptr);

const char* KeyName(int key);

}
