#pragma once

#include <imgui.h>
#include <string>
#include <unordered_map>

namespace gui {

// GPU-текстуры из embedded_assets; таблицы per-ImGuiContext (мультиинстансы:
// чужие текстурные ID невалидны, unloadAll не должен трогать чужие)
class TextureManager {
public:
    struct Texture {
        ImTextureID id = 0;
        int width = 0;
        int height = 0;
    };

    static TextureManager& get();

    void loadAll();

    void unloadAll();

    Texture find(const char* name) const;

    static void setPlatformContext(void* ctx);

private:
    TextureManager() = default;
    ~TextureManager() = default;

    static ImTextureID createTexture(const unsigned char* rgba, int width, int height);
    static void destroyTexture(ImTextureID id);

    std::unordered_map<ImGuiContext*, std::unordered_map<std::string, Texture>> tables_;
};

} // namespace gui
