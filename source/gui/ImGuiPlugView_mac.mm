#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "ImGuiPlugView.hpp"
#include "TextureManager.hpp"
#include "Fonts.hpp"
#include <imgui.h>
#include <imgui_impl_metal.h>
#include <algorithm>

@interface SaturatorMetalView : MTKView <MTKViewDelegate> {
    gui::ImGuiPlugView* _plugView;
    ImGuiContext* _imguiContext;
    id<MTLCommandQueue> _commandQueue;
    BOOL _imguiInitialized;
    double _lastFrameTime;
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
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.GrabRounding = 3.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.22f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.45f, 0.55f, 0.85f, 1.0f);

        gui::loadFonts();
        ImGui_ImplMetal_Init(device);
        gui::TextureManager::setPlatformContext((__bridge void*)device);
        gui::TextureManager::get().loadAll();

        _lastFrameTime = 0.0;
        _imguiInitialized = YES;
    }
    return self;
}

- (BOOL)isFlipped { return YES; }

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
}

- (void)drawInMTKView:(MTKView*)view {
    if (!_imguiInitialized || !_plugView)
        return;

    ImGui::SetCurrentContext(_imguiContext);

    MTLRenderPassDescriptor* rpd = view.currentRenderPassDescriptor;
    if (rpd == nil)
        return;

    ImGuiIO& io = ImGui::GetIO();
    NSSize sz = self.bounds.size;
    io.DisplaySize = ImVec2((float)sz.width, (float)sz.height);
    CGFloat fbScale = self.window ? self.window.backingScaleFactor : 1.0;
    io.DisplayFramebufferScale = ImVec2((float)fbScale, (float)fbScale);
    double now = CACurrentMediaTime();
    io.DeltaTime = (_lastFrameTime > 0.0)
        ? (float)std::max(1e-4, now - _lastFrameTime) : (1.0f / 60.0f);
    _lastFrameTime = now;

    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    ImGui_ImplMetal_NewFrame(rpd);
    ImGui::NewFrame();

    _plugView->renderFrame();

    ImGui::Render();
    id<MTLRenderCommandEncoder> encoder =
        [commandBuffer renderCommandEncoderWithDescriptor:rpd];
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
    [encoder endEncoding];

    [commandBuffer presentDrawable:view.currentDrawable];
    [commandBuffer commit];

    if (_plugView && _plugView->hasPendingResize()) {
        [self performSelectorOnMainThread:@selector(flushResize)
                               withObject:nil
                            waitUntilDone:NO];
    }
}

- (void)flushResize {
    if (_imguiInitialized && _plugView)
        _plugView->flushPendingResize();
}

#pragma mark - Shutdown

- (void)shutdownImGui {
    if (!_imguiInitialized) return;
    _imguiInitialized = NO;
    _plugView = nullptr;
    self.delegate = nil;

    ImGui::SetCurrentContext(_imguiContext);
    gui::TextureManager::get().unloadAll();
    ImGui_ImplMetal_Shutdown();
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

- (void)feedMousePos:(NSEvent*)event {
    if (!_imguiInitialized) return;
    ImGui::SetCurrentContext(_imguiContext);
    NSPoint p = [self convertPoint:event.locationInWindow fromView:nil];
    ImGui::GetIO().AddMousePosEvent((float)p.x, (float)p.y);
    [self setNeedsDisplay:YES];
}

- (void)feedMouseButton:(int)btn down:(BOOL)down event:(NSEvent*)event {
    if (!_imguiInitialized) return;
    [self feedMousePos:event];
    ImGui::GetIO().AddMouseButtonEvent(btn, down);
}

- (void)mouseDown:(NSEvent*)event       { [self feedMouseButton:0 down:YES event:event]; }
- (void)mouseUp:(NSEvent*)event         { [self feedMouseButton:0 down:NO  event:event]; }
- (void)rightMouseDown:(NSEvent*)event  { [self feedMouseButton:1 down:YES event:event]; }
- (void)rightMouseUp:(NSEvent*)event    { [self feedMouseButton:1 down:NO  event:event]; }
- (void)otherMouseDown:(NSEvent*)event  { [self feedMouseButton:2 down:YES event:event]; }
- (void)otherMouseUp:(NSEvent*)event    { [self feedMouseButton:2 down:NO  event:event]; }
- (void)mouseMoved:(NSEvent*)event      { [self feedMousePos:event]; }
- (void)mouseDragged:(NSEvent*)event    { [self feedMousePos:event]; }
- (void)rightMouseDragged:(NSEvent*)event { [self feedMousePos:event]; }
- (void)otherMouseDragged:(NSEvent*)event { [self feedMousePos:event]; }

- (void)mouseExited:(NSEvent*)event {
    if (!_imguiInitialized) return;
    ImGui::SetCurrentContext(_imguiContext);
    ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    [self setNeedsDisplay:YES];
}

- (void)scrollWheel:(NSEvent*)event {
    if (!_imguiInitialized) return;
    [self feedMousePos:event];
    double dx = event.scrollingDeltaX;
    double dy = event.scrollingDeltaY;
    if (event.hasPreciseScrollingDeltas) {
        dx *= 0.1;
        dy *= 0.1;
    }
    ImGui::GetIO().AddMouseWheelEvent((float)dx, (float)dy);
}

- (void)flagsChanged:(NSEvent*)event {
    if (!_imguiInitialized) return;
    ImGui::SetCurrentContext(_imguiContext);
    NSEventModifierFlags f = event.modifierFlags;
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Shift, (f & NSEventModifierFlagShift) != 0);
    io.AddKeyEvent(ImGuiMod_Ctrl,  (f & NSEventModifierFlagControl) != 0);
    io.AddKeyEvent(ImGuiMod_Alt,   (f & NSEventModifierFlagOption) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (f & NSEventModifierFlagCommand) != 0);
    [self setNeedsDisplay:YES];
}

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

    // владение +1 от alloc/init держит platformData_; лишний retain тёк по MTKView на каждое закрытие окна
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
}

} // namespace gui
