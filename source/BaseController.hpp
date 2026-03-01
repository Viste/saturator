#pragma once

#include "dsp/MeteringData.hpp"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace gui { class ImGuiPlugView; }

class BaseController : public Steinberg::Vst::EditController {
public:
    BaseController();
    ~BaseController() override = default;

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IEditController*>(new BaseController);
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;

    Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
    Steinberg::tresult PLUGIN_API getParamStringByValue(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) override;
    Steinberg::tresult PLUGIN_API getParamValueByString(Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized) override;

    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

    void viewRemoved(gui::ImGuiPlugView* view);

    OBJ_METHODS(BaseController, EditController)
    REFCOUNT_METHODS(EditController)

private:
    dsp::MeteringData* meteringData_ = nullptr;
    gui::ImGuiPlugView* activeView_ = nullptr;
};
