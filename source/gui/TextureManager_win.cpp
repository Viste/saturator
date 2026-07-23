#ifdef _WIN32

#include "TextureManager.hpp"
#include <windows.h>
#include <gl/GL.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace gui {

void TextureManager::setPlatformContext(void* /*ctx*/) {
}

ImTextureID TextureManager::createTexture(const unsigned char* rgba, int width, int height) {
    if (!rgba || width <= 0 || height <= 0) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (!tex) return 0;

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    return static_cast<ImTextureID>(tex);
}

void TextureManager::destroyTexture(ImTextureID id) {
    if (!id) return;
    GLuint tex = static_cast<GLuint>(id);
    glDeleteTextures(1, &tex);
}

} // namespace gui

#endif // _WIN32
