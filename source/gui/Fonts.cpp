#include "Fonts.hpp"
#include "embedded_assets.h"
#include <imgui.h>

namespace gui {

static ImFont* addFont(const unsigned char* data, unsigned long size, float basePx) {
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    ImFont* f = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(data), static_cast<int>(size), basePx, &cfg);
    return f ? f : ImGui::GetIO().Fonts->AddFontDefault();
}

void loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // порядок фиксирован: fonts() резолвит слоты по индексу атласа
    using namespace embedded;
    addFont(ZenKurenaido_Regular_data,    ZenKurenaido_Regular_size,    16.0f);
    addFont(ZenKakuGothicNew_Medium_data, ZenKakuGothicNew_Medium_size, 13.0f);
    addFont(ZenKakuGothicNew_Light_data,  ZenKakuGothicNew_Light_size,  13.0f);

    io.FontDefault = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : nullptr;
}

// из атласа ТЕКУЩЕГО контекста — глобальный кеш ImFont* ломает мультиинстансы
Fonts fonts() {
    ImFontAtlas* atlas = ImGui::GetIO().Fonts;
    int n = atlas->Fonts.Size;
    Fonts f;
    f.head  = n > 0 ? atlas->Fonts[0] : nullptr;
    f.label = n > 1 ? atlas->Fonts[1] : f.head;
    f.light = n > 2 ? atlas->Fonts[2] : f.label;
    return f;
}

} // namespace gui
