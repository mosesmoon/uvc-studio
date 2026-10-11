#pragma once
#include "imgui.h"

namespace icons {
ImTextureID get(const char* name, int px);
// Draws icon `name` (size in UI units) centred at `center`, tinted `col`.
void draw(ImDrawList* dl, const char* name, ImVec2 center, float size, ImU32 col, float fbScale);
void clear();
}  // namespace icons
