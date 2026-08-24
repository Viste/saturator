#include "ImGuiPlugView.hpp"
#include "SaturatorUI.hpp"
#include "../BaseController.hpp"
#include <imgui.h>

namespace gui {

ImGuiPlugView::ImGuiPlugView(Steinberg::Vst::EditController* controller)
    : CPluginView(nullptr)
    , controller_(controller) {
    rect = Steinberg::ViewRect(0, 0, kDefaultWidth, kDefaultHeight);
    uiState_.controller = controller;
}

ImGuiPlugView::~ImGuiPlugView() {
    if (auto* ctrl = dynamic_cast<BaseController*>(controller_)) {
        ctrl->viewRemoved(this);
    }
    if (initialized_) {
        initialized_ = false;
        platformShutdown();
    }
}

Steinberg::tresult PLUGIN_API ImGuiPlugView::isPlatformTypeSupported(Steinberg::FIDString type) {
#ifdef __APPLE__
    if (strcmp(type, Steinberg::kPlatformTypeNSView) == 0)
        return Steinberg::kResultTrue;
#elif defined(_WIN32)
    if (strcmp(type, Steinberg::kPlatformTypeHWND) == 0)
        return Steinberg::kResultTrue;
#endif
    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API ImGuiPlugView::attached(void* parent, Steinberg::FIDString type) {
    if (!parent)
        return Steinberg::kResultFalse;

    if (!platformInit(parent))
        return Steinberg::kResultFalse;

    initialized_ = true;
    return CPluginView::attached(parent, type);
}

Steinberg::tresult PLUGIN_API ImGuiPlugView::removed() {
    if (initialized_) {
        initialized_ = false;
        platformShutdown();
    }
    return CPluginView::removed();
}

Steinberg::tresult PLUGIN_API ImGuiPlugView::getSize(Steinberg::ViewRect* size) {
    if (!size)
        return Steinberg::kResultFalse;
    *size = rect;
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API ImGuiPlugView::onSize(Steinberg::ViewRect* newSize) {
    if (!newSize)
        return Steinberg::kResultFalse;
    rect = *newSize;
    if (initialized_) {
        platformResize(newSize->right - newSize->left, newSize->bottom - newSize->top);
    }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API ImGuiPlugView::canResize() {
    return Steinberg::kResultTrue;
}

void ImGuiPlugView::renderFrame() {
    if (!initialized_)
        return;

    platformBeginFrame();
    SaturatorUI::render(uiState_);
    platformEndFrame();

    int reqH = uiState_.requestedHeight;
    if (reqH > 0 && reqH != (rect.bottom - rect.top)) {
        pendingResizeHeight_ = reqH;
    }
    uiState_.requestedHeight = 0;
}

void ImGuiPlugView::flushPendingResize() {
    int reqH = pendingResizeHeight_;
    pendingResizeHeight_ = 0;
    if (!initialized_ || reqH <= 0 || reqH == (rect.bottom - rect.top))
        return;
    Steinberg::ViewRect newRect(0, 0, rect.right - rect.left, reqH);
    rect = newRect;
    if (plugFrame)
        plugFrame->resizeView(this, &newRect);
}

} // namespace gui
