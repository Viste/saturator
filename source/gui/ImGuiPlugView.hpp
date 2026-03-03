#pragma once

#include "UIState.hpp"
#include <public.sdk/source/common/pluginview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace gui {

// vst3 IPlugView на dear imgui + opengl
// платформенный рендеринг в ImGuiPlugView_mac.mm / ImGuiPlugView_win.cpp
class ImGuiPlugView : public Steinberg::CPluginView {
public:
    explicit ImGuiPlugView(Steinberg::Vst::EditController* controller);
    ~ImGuiPlugView() override;

    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override;
    Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
    Steinberg::tresult PLUGIN_API removed() override;
    Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* size) override;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) override;
    Steinberg::tresult PLUGIN_API canResize() override;

    void renderFrame();

    UIState& getUIState() { return uiState_; }

private:
    bool platformInit(void* parentWindow);
    void platformShutdown();
    void platformBeginFrame();
    void platformEndFrame();
    void platformResize(int width, int height);

    Steinberg::Vst::EditController* controller_ = nullptr;
    UIState uiState_;

    static constexpr int kDefaultWidth = 900;
    static constexpr int kDefaultHeight = 500;
    static constexpr int kExpandedHeight = 700;

    void* platformData_ = nullptr;
    bool initialized_ = false;
};

} // namespace gui
