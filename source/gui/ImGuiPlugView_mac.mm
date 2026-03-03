#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "ImGuiPlugView.hpp"
#include <imgui.h>
#include <imgui_impl_metal.h>
#include <imgui_impl_osx.h>

@interface SaturatorMetalView : MTKView <MTKViewDelegate> {
    gui::ImGuiPlugView* _plugView;
    ImGuiContext* _imguiContext;
    id<MTLCommandQueue> _commandQueue;
    BOOL _imguiInitialized;
}
- (instancetype)initWithFrame:(NSRect)frame plugView:(gui::ImGuiPlugView*)plugView;
- (void)shutdownImGui;
@end

@implementation SaturatorMetalView

- (instancetype)initWithFrame:(NSRect)frame plugView:(gui::ImGuiPlugView*)plugView {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    self = [super initWithFrame:frame device:device];
    if (self) {
        _plugView = plugView;
        _imguiInitialized = NO;
        _commandQueue = [device newCommandQueue];

        self.delegate = self;
        self.clearColor = MTLClearColorMake(0.12, 0.12, 0.14, 1.0);
        self.preferredFramesPerSecond = 60;

        IMGUI_CHECKVERSION();
        _imguiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(_imguiContext);

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.55f, 0.85f, 1.0f);

        // шрифт с кириллицей
        ImFontConfig fontCfg;
        fontCfg.FontNo = 0;
        fontCfg.OversampleH = 2;
        fontCfg.OversampleV = 1;
        const ImWchar* cyrillicRanges = io.Fonts->GetGlyphRangesCyrillic();
        if (!io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Helvetica.ttc",
                                           14.0f, &fontCfg, cyrillicRanges)) {
            io.Fonts->AddFontDefault();
        }

        ImGui_ImplMetal_Init(device);
        ImGui_ImplOSX_Init(self);
        _imguiInitialized = YES;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    // MTKView обрабатывает ресайз автоматически
}

- (void)drawInMTKView:(MTKView*)view {
    if (!_imguiInitialized || !_plugView)
        return;

    ImGui::SetCurrentContext(_imguiContext);

    MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
    if (rpd == nil)
        return;

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    // begin ImGui frame
    ImGui_ImplMetal_NewFrame(rpd);
    ImGui_ImplOSX_NewFrame(view);
    ImGui::NewFrame();

    // отрисовка UI плагина
    _plugView->renderFrame();

    // end ImGui frame
    ImGui::Render();
    id<MTLRenderCommandEncoder> encoder =
        [commandBuffer renderCommandEncoderWithDescriptor:rpd];
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
    [encoder endEncoding];

    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];
}

#pragma mark - Shutdown

- (void)shutdownImGui {
    if (!_imguiInitialized) return;
    _imguiInitialized = NO;

    ImGui::SetCurrentContext(_imguiContext);
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext(_imguiContext);
    _imguiContext = nullptr;
}

- (void)dealloc {
    [self shutdownImGui];
    [super dealloc];
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { return YES; }

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    for (NSTrackingArea* area in [self trackingAreas]) {
        [self removeTrackingArea:area];
    }
    NSTrackingArea* ta = [[NSTrackingArea alloc]
        initWithRect:[self bounds]
             options:(NSTrackingMouseMoved | NSTrackingActiveAlways |
                      NSTrackingInVisibleRect | NSTrackingMouseEnteredAndExited)
               owner:self
            userInfo:nil];
    [self addTrackingArea:ta];
}

// события мыши/клавиатуры → перерисовка
- (void)mouseDown:(NSEvent*)event       { [self setNeedsDisplay:YES]; }
- (void)mouseUp:(NSEvent*)event         { [self setNeedsDisplay:YES]; }
- (void)mouseMoved:(NSEvent*)event      { [self setNeedsDisplay:YES]; }
- (void)mouseDragged:(NSEvent*)event    { [self setNeedsDisplay:YES]; }
- (void)rightMouseDown:(NSEvent*)event  { [self setNeedsDisplay:YES]; }
- (void)rightMouseUp:(NSEvent*)event    { [self setNeedsDisplay:YES]; }
- (void)scrollWheel:(NSEvent*)event     { [self setNeedsDisplay:YES]; }
- (void)keyDown:(NSEvent*)event         { [self setNeedsDisplay:YES]; }
- (void)keyUp:(NSEvent*)event           { [self setNeedsDisplay:YES]; }
- (void)flagsChanged:(NSEvent*)event    { [self setNeedsDisplay:YES]; }

@end

namespace gui {

bool ImGuiPlugView::platformInit(void* parentWindow) {
    NSView* parentView = (NSView*)parentWindow;

    NSRect frame = [parentView bounds];
    if (frame.size.width < 1 || frame.size.height < 1) {
        frame = NSMakeRect(0, 0, kDefaultWidth, kDefaultHeight);
    }

    SaturatorMetalView* metalView = [[SaturatorMetalView alloc] initWithFrame:frame plugView:this];
    [metalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [parentView addSubview:metalView];

    [metalView retain];
    platformData_ = (void*)metalView;
    return true;
}

void ImGuiPlugView::platformShutdown() {
    if (platformData_) {
        SaturatorMetalView* metalView = (SaturatorMetalView*)platformData_;
        metalView.paused = YES;
        [metalView shutdownImGui];
        [metalView removeFromSuperview];
        [metalView release];
        platformData_ = nullptr;
    }
}

void ImGuiPlugView::platformBeginFrame() {
    // Metal: begin/end frame обрабатывается в drawInMTKView
}

void ImGuiPlugView::platformEndFrame() {
    // Metal: begin/end frame обрабатывается в drawInMTKView
}

void ImGuiPlugView::platformResize(int /*width*/, int /*height*/) {
    // MTKView авто-ресайзится через NSViewWidthSizable | NSViewHeightSizable
}

} // namespace gui
