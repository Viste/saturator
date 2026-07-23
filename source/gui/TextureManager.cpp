#include "TextureManager.hpp"
#include "embedded_assets.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_TGA
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_JPEG
#include "stb_image.h"

namespace gui {

TextureManager& TextureManager::get() {
    static TextureManager instance;
    return instance;
}

namespace {
struct Asset {
    const char* logical;
    const unsigned char* data;
    unsigned long size;
};
}

void TextureManager::loadAll() {
    auto& table = tables_[ImGui::GetCurrentContext()];
    if (!table.empty()) return;

    using namespace embedded;
    const Asset assets[] = {
        { "bg",            bg_900x700_data,                               bg_900x700_size },
        { "logo",          logo_193x169_data,                             logo_193x169_size },
        { "knob_big",      big_knob_complete_90x90_spritesheet_data,      big_knob_complete_90x90_spritesheet_size },
        { "knob_small",    small_knob_48x48_spritesheet_data,             small_knob_48x48_spritesheet_size },
        { "mode_button",   mode_button_136x24_off_on_data,                mode_button_136x24_off_on_size },
        { "os_1x",         OS_multiplier_button_1x_34x18_off_on_data,     OS_multiplier_button_1x_34x18_off_on_size },
        { "os_2x",         OS_multiplier_button_2x_34x18_off_on_data,     OS_multiplier_button_2x_34x18_off_on_size },
        { "os_4x",         OS_multiplier_button_4x_34x18_off_on_data,     OS_multiplier_button_4x_34x18_off_on_size },
        { "lang_switch",   switch_ru_en_36x19_data,                       switch_ru_en_36x19_size },
        { "update_button", update_popup_button_17x17_data,                update_popup_button_17x17_size },
    };

    for (const auto& a : assets) {
        int w = 0, h = 0, channels = 0;
        unsigned char* pixels = stbi_load_from_memory(a.data, static_cast<int>(a.size),
                                                       &w, &h, &channels, 4);
        if (!pixels) continue;

        ImTextureID texId = createTexture(pixels, w, h);
        stbi_image_free(pixels);

        if (texId) {
            table[a.logical] = Texture{ texId, w, h };
        }
    }
}

void TextureManager::unloadAll() {
    auto it = tables_.find(ImGui::GetCurrentContext());
    if (it == tables_.end()) return;
    for (auto& [name, tex] : it->second) {
        if (tex.id) destroyTexture(tex.id);
    }
    tables_.erase(it);
}

TextureManager::Texture TextureManager::find(const char* name) const {
    auto it = tables_.find(ImGui::GetCurrentContext());
    if (it == tables_.end()) return {};
    auto it2 = it->second.find(name);
    if (it2 == it->second.end()) return {};
    return it2->second;
}

} // namespace gui
