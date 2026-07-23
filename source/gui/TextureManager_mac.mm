#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include "TextureManager.hpp"

namespace gui {

static id<MTLDevice> sMetalDevice = nil;

void TextureManager::setPlatformContext(void* ctx) {
    sMetalDevice = (__bridge id<MTLDevice>)ctx;
}

ImTextureID TextureManager::createTexture(const unsigned char* rgba, int width, int height) {
    if (!sMetalDevice || !rgba || width <= 0 || height <= 0) return 0;

    MTLTextureDescriptor* desc = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                     width:width
                                    height:height
                                 mipmapped:NO];
    desc.usage = MTLTextureUsageShaderRead;
    desc.storageMode = MTLStorageModeShared;

    id<MTLTexture> tex = [sMetalDevice newTextureWithDescriptor:desc];
    if (!tex) return 0;

    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [tex replaceRegion:region
           mipmapLevel:0
             withBytes:rgba
           bytesPerRow:width * 4];

    void* retained = (void*)CFBridgingRetain(tex);
    return reinterpret_cast<ImTextureID>(retained);
}

void TextureManager::destroyTexture(ImTextureID texId) {
    if (!texId) return;
    void* ptr = reinterpret_cast<void*>(texId);
    CFBridgingRelease(ptr);
}

} // namespace gui
