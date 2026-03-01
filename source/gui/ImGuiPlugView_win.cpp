#ifdef _WIN32

#include "ImGuiPlugView.hpp"
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include <windows.h>
#include <GL/gl.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace gui {

struct Win32PlatformData {
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC hglrc = nullptr;
    UINT_PTR timerId = 0;
    ImGuiContext* imguiContext = nullptr;
    ImGuiPlugView* plugView = nullptr;
};

static const wchar_t* kWindowClassName = L"SaturatorImGuiView";
static bool sClassRegistered = false;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* data = reinterpret_cast<Win32PlatformData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (data && data->imguiContext) {
        ImGui::SetCurrentContext(data->imguiContext);
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return 1;
    }

    switch (msg) {
        case WM_TIMER:
            if (data && data->plugView) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_PAINT: {
            if (data && data->plugView) {
                wglMakeCurrent(data->hdc, data->hglrc);
                ImGui::SetCurrentContext(data->imguiContext);
                data->plugView->renderFrame();
                ValidateRect(hwnd, nullptr);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        default:
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool ImGuiPlugView::platformInit(void* parentWindow) {
    HWND parentHwnd = static_cast<HWND>(parentWindow);
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    if (!sClassRegistered) {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = kWindowClassName;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassEx(&wc);
        sClassRegistered = true;
    }

    auto* data = new Win32PlatformData();
    data->plugView = this;

    data->hwnd = CreateWindowEx(
        0, kWindowClassName, L"Saturator",
        WS_CHILD | WS_VISIBLE,
        0, 0, kDefaultWidth, kDefaultHeight,
        parentHwnd, nullptr, hInstance, nullptr
    );

    if (!data->hwnd) {
        delete data;
        return false;
    }

    SetWindowLongPtr(data->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));

    data->hdc = GetDC(data->hwnd);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(data->hdc, &pfd);
    SetPixelFormat(data->hdc, pixelFormat, &pfd);

    data->hglrc = wglCreateContext(data->hdc);
    wglMakeCurrent(data->hdc, data->hglrc);

    IMGUI_CHECKVERSION();
    data->imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(data->imguiContext);

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

    {
        ImFontConfig fontCfg;
        fontCfg.OversampleH = 2;
        fontCfg.OversampleV = 1;
        const ImWchar* cyrillicRanges = io.Fonts->GetGlyphRangesCyrillic();
        if (!io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf",
                                           15.0f, &fontCfg, cyrillicRanges)) {
            io.Fonts->AddFontDefault();
        }
    }

    ImGui_ImplWin32_Init(data->hwnd);
    ImGui_ImplOpenGL3_Init("#version 130");

    // таймер рендеринга 60fps
    data->timerId = SetTimer(data->hwnd, 1, 16, nullptr);

    platformData_ = data;
    return true;
}

void ImGuiPlugView::platformShutdown() {
    auto* data = static_cast<Win32PlatformData*>(platformData_);
    if (!data) return;

    if (data->timerId) {
        KillTimer(data->hwnd, data->timerId);
    }

    wglMakeCurrent(data->hdc, data->hglrc);
    ImGui::SetCurrentContext(data->imguiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext(data->imguiContext);

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(data->hglrc);
    ReleaseDC(data->hwnd, data->hdc);
    DestroyWindow(data->hwnd);

    delete data;
    platformData_ = nullptr;
}

void ImGuiPlugView::platformBeginFrame() {
    auto* data = static_cast<Win32PlatformData*>(platformData_);
    if (!data) return;

    wglMakeCurrent(data->hdc, data->hglrc);
    ImGui::SetCurrentContext(data->imguiContext);

    RECT rc;
    GetClientRect(data->hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    glViewport(0, 0, w, h);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiPlugView::platformEndFrame() {
    auto* data = static_cast<Win32PlatformData*>(platformData_);
    if (!data) return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SwapBuffers(data->hdc);
}

} // namespace gui

#endif // _WIN32
