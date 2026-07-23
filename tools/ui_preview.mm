// Offscreen-рендер SaturatorUI в PNG (dev-инструмент, без DAW).
// Использование: ui_preview out.png [adv] [clip] [en] [silent] [dual]
//   adv    — открытая advanced-панель (окно 900x700)
//   clip   — включённый клипер (третий кноб)
//   en     — английская локаль
//   silent — без синтезированного сигнала в метерах
//   dual   — регресс мультиинстансов: два контекста, первый закрывается,
//            сохраняется рендер второго (текстуры/шрифты должны выжить)
#import <Metal/Metal.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#import <Foundation/Foundation.h>

#include <imgui.h>
#include <imgui_impl_metal.h>
#include <cmath>
#include <cstring>
#include <vector>

#include "gui/SaturatorUI.hpp"
#include "gui/UIState.hpp"
#include "gui/Fonts.hpp"
#include "gui/TextureManager.hpp"
#include "BaseController.hpp"
#include "PluginIds.hpp"
#include "dsp/MeteringData.hpp"

static int gW = 900, gH = 500;
static const int kFB = 2;

static ImGuiContext* makeInstance(id<MTLDevice> device) {
    ImGuiContext* ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2((float)gW, (float)gH);
    io.DisplayFramebufferScale = ImVec2((float)kFB, (float)kFB);
    gui::loadFonts();
    ImGui_ImplMetal_Init(device);
    gui::TextureManager::setPlatformContext((__bridge void*)device);
    gui::TextureManager::get().loadAll();
    return ctx;
}

static void destroyInstance(ImGuiContext* ctx) {
    ImGui::SetCurrentContext(ctx);
    gui::TextureManager::get().unloadAll();
    ImGui_ImplMetal_Shutdown();
    ImGui::DestroyContext(ctx);
}

static void renderFrames(ImGuiContext* ctx, gui::UIState& state,
                         id<MTLCommandQueue> queue, id<MTLTexture> target, int frames) {
    ImGui::SetCurrentContext(ctx);
    for (int frame = 0; frame < frames; ++frame) {
        MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
        rpd.colorAttachments[0].texture = target;
        rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
        rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
        rpd.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);

        ImGui_ImplMetal_NewFrame(rpd);
        ImGui::NewFrame();
        gui::SaturatorUI::render(state);
        ImGui::Render();

        id<MTLCommandBuffer> cb = [queue commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rpd];
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cb, enc);
        [enc endEncoding];
        [cb commit];
        [cb waitUntilCompleted];
        state.requestedHeight = 0;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s out.png [adv] [clip] [en] [silent] [dual]\n", argv[0]);
        return 1;
    }
    const char* outPath = argv[1];
    bool adv = false, clip = false, en = false, silent = false, dual = false;
    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "adv")) adv = true;
        else if (!strcmp(argv[i], "clip")) clip = true;
        else if (!strcmp(argv[i], "en")) en = true;
        else if (!strcmp(argv[i], "silent")) silent = true;
        else if (!strcmp(argv[i], "dual")) dual = true;
    }
    gH = adv ? 700 : 500;

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) { fprintf(stderr, "no metal device\n"); return 1; }
    id<MTLCommandQueue> queue = [device newCommandQueue];

    auto* controller = new BaseController();
    controller->initialize(nullptr);
    if (clip) controller->setParamNormalized(kClipEnabled, 1.0);
    controller->setParamNormalized(kSaturation, 0.35);

    dsp::MeteringData metering;
    if (!silent) {
        for (int i = 0; i < dsp::MeteringData::kWaveformSize; ++i) {
            float t = (float)i / 44100.0f;
            float in = 0.55f * std::sin(2.0f * (float)M_PI * 220.0f * t)
                     + 0.12f * std::sin(2.0f * (float)M_PI * 2900.0f * t);
            float out = std::tanh(2.2f * in);
            metering.pushInputSample(in, in);
            metering.pushOutputSample(out, out);
        }
    }

    gui::UIState state;
    state.controller = controller;
    state.metering = silent ? nullptr : &metering;
    state.advancedOpen = adv;
    state.lang = en ? gui::Lang::EN : gui::Lang::RU;

    const int texW = gW * kFB, texH = gH * kFB;
    MTLTextureDescriptor* td = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:texW height:texH mipmapped:NO];
    td.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    td.storageMode = MTLStorageModeShared;
    id<MTLTexture> target = [device newTextureWithDescriptor:td];

    ImGuiContext* ctxA = makeInstance(device);

    if (dual) {
        ImGuiContext* ctxB = makeInstance(device);
        gui::UIState stateB = state;
        renderFrames(ctxA, state, queue, target, 3);
        renderFrames(ctxB, stateB, queue, target, 3);
        destroyInstance(ctxA);                       // «закрыли» первый инстанс
        renderFrames(ctxB, stateB, queue, target, 10);
    } else {
        renderFrames(ctxA, state, queue, target, 10);
    }

    std::vector<uint8_t> px((size_t)texW * texH * 4);
    [target getBytes:px.data()
         bytesPerRow:(NSUInteger)texW * 4
          fromRegion:MTLRegionMake2D(0, 0, texW, texH)
         mipmapLevel:0];

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(px.data(), texW, texH, 8, (size_t)texW * 4, cs,
                                             kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
    CGImageRef img = CGBitmapContextCreateImage(ctx);
    NSURL* url = [NSURL fileURLWithPath:@(outPath)];
    CGImageDestinationRef dst =
        CGImageDestinationCreateWithURL((__bridge CFURLRef)url, CFSTR("public.png"), 1, NULL);
    if (!dst) { fprintf(stderr, "cannot create %s\n", outPath); return 1; }
    CGImageDestinationAddImage(dst, img, NULL);
    bool ok = CGImageDestinationFinalize(dst);
    CFRelease(dst);
    CGImageRelease(img);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);

    fprintf(stderr, "%s: %s (%dx%d @%dx)%s\n", ok ? "written" : "FAILED", outPath,
            gW, gH, kFB, dual ? " [dual]" : "");
    return ok ? 0 : 1;
}
