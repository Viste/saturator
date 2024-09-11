#include "BaseController.hpp"
#include "MessagesConsts.hpp"
#include "PluginIds.hpp"
#include "subcontrollers/MainKnobController.hpp"
#include "views/CustomVST3Editor.hpp"
#include <base/source/fstreamer.h>

BaseController::BaseController() {}

tresult PLUGIN_API BaseController::initialize(FUnknown *context) {
    tresult result = EditController::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    EditController::setKnobMode(Vst::kLinearMode);

    parameters.addParameter(STR16("Bypass"), nullptr, 1, 0,
                            ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, Params::kBypass);

    parameters.addParameter(STR16("Saturation"), nullptr, 0, 0.0f,
                        ParameterInfo::kCanAutomate, Params::kSaturation);

    parameters.addParameter(STR16("Switch"), nullptr, 0, 0.0f,
                            ParameterInfo::kNoFlags, Params::kSwitch);

    parameters.addParameter(STR16("Gain"), nullptr, 0, 0.5f,
                        ParameterInfo::kCanAutomate, Params::kGain);

    parameters.addParameter(STR16("death"), nullptr, 1, 0,
                            ParameterInfo::kCanAutomate, Params::kDeath);

    return result;
}

tresult PLUGIN_API BaseController::terminate() {
    return EditController::terminate();
}

tresult PLUGIN_API BaseController::setComponentState(IBStream *state) {
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    bool bypass = false;
    if (!streamer.readBool(bypass))
        return kResultFalse;
    setParamNormalized(Params::kBypass, bypass ? 1.0f : 0.0f);

    float saturation = 0.0f;
    if (!streamer.readFloat(saturation))
        return kResultFalse;
    setParamNormalized(Params::kSaturation, saturation);
    
    float gain = 0.5f;
    if (!streamer.readFloat(gain))
        return kResultFalse;
    setParamNormalized(Params::kGain, gain);

    int algorithm = 0;
    if (!streamer.readInt32(algorithm))
        return kResultFalse;
    setParamNormalized(Params::kSwitch, static_cast<float>(algorithm) / 1.0f);

    bool death = false;
    if (!streamer.readBool(death))
        return kResultFalse;
    setParamNormalized(Params::kDeath, death ? 1.0f : 0.0f);

    return kResultOk;
}

IPlugView *PLUGIN_API BaseController::createView(FIDString name) {
    if (FIDStringsEqual(name, Vst::ViewType::kEditor)) {
#ifdef WIN32
        return new VST3Editor(this, "view", "layout.uidesc");
#elif defined(__APPLE__)
        return new CustomVST3Editor(this, "view", "layout.uidesc");
#endif

    }
    return nullptr;
}


tresult PLUGIN_API BaseController::setParamNormalized(Vst::ParamID tag,
                                                      Vst::ParamValue value) {
    tresult result = EditController::setParamNormalized(tag, value);
    return result;
}


tresult PLUGIN_API BaseController::getParamStringByValue(
        Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) {
    return EditController::getParamStringByValue(tag, valueNormalized, string);
}


tresult PLUGIN_API BaseController::getParamValueByString(
        Vst::ParamID tag, Vst::TChar *string, Vst::ParamValue &valueNormalized) {
    return EditController::getParamValueByString(tag, string, valueNormalized);
}

IController *BaseController::createSubController(
        const char *name,
        const IUIDescription *description,
        VST3Editor *editor
) {
    if (strcmp(name, "KnobController") == 0) {
        return new MainKnobController(editor);
    }
    if (strcmp(name, "MainController") == 0) {
        return new ModeSwitchController(editor);
    }
    if (strcmp(name, "OscKnobController") == 0) {
        return new MainKnobController(editor);
    }
    return nullptr;
}

tresult PLUGIN_API BaseController::notify(IMessage *message) 
{
    return ComponentBase::notify(message);
}
