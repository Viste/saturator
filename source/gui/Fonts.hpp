#pragma once

struct ImFont;

namespace gui {

struct Fonts {
    ImFont* head = nullptr;
    ImFont* label = nullptr;
    ImFont* light = nullptr;
};

Fonts fonts();

void loadFonts();

} // namespace gui
