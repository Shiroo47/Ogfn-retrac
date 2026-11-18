
#pragma once
#include "imgui.h"

namespace ui {

struct AnimState
{
    float  f[6] = {};
    ImVec4 c[4] = {};
    bool   initialized = false;
    int    last_frame  = -1;
};

AnimState& GetAnim(ImGuiID id);

float  Approach(float& v, float target, float speed);
ImVec4 Approach(ImVec4& v, const ImVec4& target, float speed);

void AnimNewFrame();

}
