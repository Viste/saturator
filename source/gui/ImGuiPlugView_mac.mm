#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <OpenGL/OpenGL.h>

#include "ImGuiPlugView.hpp"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_osx.h>

@interface SaturatorOpenGLView : NSOpenGLView {
    gui::ImGuiPlugView* _plugView;
    NSTimer* _renderTimer;
    ImGuiContext* _imguiContext;
    BOOL _imguiInitialized;
}
- (instancetype)initWithFrame:(NSRect)frame plugView:(gui::ImGuiPlugView*)plugView;
- (void)startRendering;
- (void)stopRendering;
- (void)shutdownImGui;
@end

@implementation SaturatorOpenGLView

- (instancetype)initWithFrame:(NSRect)frame plugView:(gui::ImGuiPlugView*)plugView {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        0
    };

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    self = [super initWithFrame:frame pixelFormat:pixelFormat];
    if (self) {
        _plugView = plugView;
        _renderTimer = nil;
        _imguiInitialized = NO;

        [[self openGLContext] makeCurrentContext];

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

        ImGui_ImplOSX_Init(self);
        ImGui_ImplOpenGL3_Init("#version 150");
        _imguiInitialized = YES;
    }
    return self;
}

// top-left origin для imgui
- (BOOL)isFlipped { return YES; }

- (void)startRendering {
    if (!_renderTimer) {
        _renderTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                       target:self
                                                     selector:@selector(renderTick:)
                                                     userInfo:nil
                                                      repeats:YES];
        [[NSRunLoop currentRunLoop] addTimer:_renderTimer forMode:NSRunLoopCommonModes];
    }
}

- (void)stopRendering {
    [_renderTimer invalidate];
    _renderTimer = nil;
}

- (void)renderTick:(NSTimer*)timer {
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [[self openGLContext] makeCurrentContext];
    ImGui::SetCurrentContext(_imguiContext);

    _plugView->renderFrame();
}

// синхронный shutdown imgui до удаления view
// убирает nsevent monitor до деинициализации
- (void)shutdownImGui {
    if (!_imguiInitialized) return;
    _imguiInitialized = NO;

    [[self openGLContext] makeCurrentContext];
    ImGui::SetCurrentContext(_imguiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext(_imguiContext);
    _imguiContext = nullptr;
}

- (void)dealloc {
    [self stopRendering];
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

// события мыши/клавиатуры → перерисовка (imgui ловит через nsevent monitor)
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

    // используем bounds родителя — daw мог закешировать другой размер
    NSRect frame = [parentView bounds];
    if (frame.size.width < 1 || frame.size.height < 1) {
        frame = NSMakeRect(0, 0, kDefaultWidth, kDefaultHeight);
    }

    SaturatorOpenGLView* glView = [[SaturatorOpenGLView alloc] initWithFrame:frame plugView:this];
    [glView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    [parentView addSubview:glView];
    [glView startRendering];

    [glView retain];
    platformData_ = (void*)glView;
    return true;
}

void ImGuiPlugView::platformShutdown() {
    if (platformData_) {
        SaturatorOpenGLView* glView = (SaturatorOpenGLView*)platformData_;
        [glView stopRendering];
        // синхронный shutdown imgui до release
        [glView shutdownImGui];
        [glView removeFromSuperview];
        [glView release];
        platformData_ = nullptr;
    }
}

void ImGuiPlugView::platformBeginFrame() {
    SaturatorOpenGLView* glView = (SaturatorOpenGLView*)platformData_;
    NSRect bounds = [glView bounds];
    NSRect backing = [glView convertRectToBacking:bounds];

    glViewport(0, 0, (int)backing.size.width, (int)backing.size.height);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplOSX_NewFrame(glView);
    ImGui::NewFrame();
}

void ImGuiPlugView::platformEndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SaturatorOpenGLView* glView = (SaturatorOpenGLView*)platformData_;
    [[glView openGLContext] flushBuffer];
}

} // namespace gui
