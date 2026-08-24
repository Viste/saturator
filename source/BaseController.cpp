#include "BaseController.hpp"
#include "PluginIds.hpp"
#include "MessagesConsts.hpp"
#include "gui/ImGuiPlugView.hpp"
#include <base/source/fstreamer.h>
#include <pluginterfaces/base/ustring.h>
#include <cmath>
#include <algorithm>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

BaseController::BaseController() = default;

tresult PLUGIN_API BaseController::initialize(FUnknown* context) {
    tresult result = EditController::initialize(context);
    if (result != kResultOk)
        return result;

    EditController::setKnobMode(kLinearMode);

    parameters.addParameter(STR16("Bypass"), nullptr, 1, 0,
                            ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypass);

    auto* modeParam = new StringListParameter(STR16("Mode"), kMode);
    modeParam->appendString(STR16("Instrument"));
    modeParam->appendString(STR16("Drums"));
    modeParam->appendString(STR16("Vocal"));
    parameters.addParameter(modeParam);

    parameters.addParameter(STR16("Saturation"), STR16("%"), 0, 0.0,
                            ParameterInfo::kCanAutomate, kSaturation);

    parameters.addParameter(STR16("Input Gain"), STR16("dB"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kInputGain);

    parameters.addParameter(STR16("Output Gain"), STR16("dB"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kOutputGain);

    parameters.addParameter(STR16("Dry/Wet"), STR16("%"), 0, 1.0,
                            ParameterInfo::kCanAutomate, kDryWet);

    auto* osParam = new StringListParameter(STR16("Oversampling"), kOversampling);
    osParam->appendString(STR16("Off"));
    osParam->appendString(STR16("2x"));
    osParam->appendString(STR16("4x"));
    parameters.addParameter(osParam);

    parameters.addParameter(STR16("Low Band Sat"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kInstLowSat);
    parameters.addParameter(STR16("Mid Band Sat"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kInstMidSat);
    parameters.addParameter(STR16("High Band Sat"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kInstHighSat);
    parameters.addParameter(STR16("Low-Mid Freq"), STR16("Hz"), 0, 0.3,
                            ParameterInfo::kCanAutomate, kInstLowMidFreq);
    parameters.addParameter(STR16("Mid-High Freq"), STR16("Hz"), 0, 0.3,
                            ParameterInfo::kCanAutomate, kInstMidHighFreq);
    parameters.addParameter(STR16("Character"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kInstCharacter);

    parameters.addParameter(STR16("Transient Sens"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kDrumTransientSens);
    parameters.addParameter(STR16("Attack"), STR16("ms"), 0, 0.25,
                            ParameterInfo::kCanAutomate, kDrumAttackMs);
    parameters.addParameter(STR16("Sustain Sat"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kDrumSustainSat);
    parameters.addParameter(STR16("Punch"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kDrumPunch);

    parameters.addParameter(STR16("Tape Bias"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kTapeBias);
    parameters.addParameter(STR16("Wow"), STR16("%"), 0, 0.3,
                            ParameterInfo::kCanAutomate, kTapeWow);
    parameters.addParameter(STR16("Flutter"), STR16("%"), 0, 0.2,
                            ParameterInfo::kCanAutomate, kTapeFlutter);
    parameters.addParameter(STR16("Hiss Level"), STR16("%"), 0, 0.0,
                            ParameterInfo::kCanAutomate, kTapeHissLevel);
    parameters.addParameter(STR16("Head Cutoff"), STR16("Hz"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kTapeHeadCutoff);
    parameters.addParameter(STR16("Tape Speed"), STR16("%"), 0, 0.5,
                            ParameterInfo::kCanAutomate, kTapeSpeed);

    auto* clipEnParam = new StringListParameter(STR16("Clip Enabled"), kClipEnabled);
    clipEnParam->appendString(STR16("Off"));
    clipEnParam->appendString(STR16("On"));
    parameters.addParameter(clipEnParam);

    parameters.addParameter(STR16("Clip Amount"), STR16("%"), 0, 0.0,
                            ParameterInfo::kCanAutomate, kClipAmount);

    return kResultOk;
}

tresult PLUGIN_API BaseController::terminate() {
    return EditController::terminate();
}

tresult PLUGIN_API BaseController::setComponentState(IBStream* state) {
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    int32 version = 0;
    if (!streamer.readInt32(version))
        return kResultFalse;

    if (version == 0) {
        // совместимость с v1
        setParamNormalized(kBypass, 0.0);

        float sat = 0.0f;
        if (streamer.readFloat(sat)) setParamNormalized(kSaturation, sat);

        int32 mode = 0;
        if (streamer.readInt32(mode)) setParamNormalized(kMode, static_cast<double>(mode) / 2.0);

        float gain = 0.5f;
        if (streamer.readFloat(gain)) {
            setParamNormalized(kInputGain, gain);
            setParamNormalized(kOutputGain, 0.5);
        }
        return kResultOk;
    }

    if (version >= 2) {
        bool bp = false;
        if (streamer.readBool(bp)) setParamNormalized(kBypass, bp ? 1.0 : 0.0);

        int32 mode = 0;
        if (streamer.readInt32(mode)) setParamNormalized(kMode, static_cast<double>(mode) / 2.0);

        float val;
        if (streamer.readFloat(val)) setParamNormalized(kSaturation, val);
        if (streamer.readFloat(val)) setParamNormalized(kInputGain, val);
        if (streamer.readFloat(val)) setParamNormalized(kOutputGain, val);
        if (streamer.readFloat(val)) setParamNormalized(kDryWet, val);

        int32 os = 0;
        if (streamer.readInt32(os)) setParamNormalized(kOversampling, static_cast<double>(os) / 2.0);

        if (streamer.readFloat(val)) setParamNormalized(kInstLowSat, val);
        if (streamer.readFloat(val)) setParamNormalized(kInstMidSat, val);
        if (streamer.readFloat(val)) setParamNormalized(kInstHighSat, val);
        if (streamer.readFloat(val)) setParamNormalized(kInstLowMidFreq, val);
        if (streamer.readFloat(val)) setParamNormalized(kInstMidHighFreq, val);
        if (streamer.readFloat(val)) setParamNormalized(kInstCharacter, val);

        if (streamer.readFloat(val)) setParamNormalized(kDrumTransientSens, val);
        if (streamer.readFloat(val)) setParamNormalized(kDrumAttackMs, val);
        if (streamer.readFloat(val)) setParamNormalized(kDrumSustainSat, val);
        if (streamer.readFloat(val)) setParamNormalized(kDrumPunch, val);

        if (streamer.readFloat(val)) setParamNormalized(kTapeBias, val);
        if (streamer.readFloat(val)) setParamNormalized(kTapeWow, val);
        if (streamer.readFloat(val)) setParamNormalized(kTapeFlutter, val);
        if (streamer.readFloat(val)) setParamNormalized(kTapeHissLevel, val);
        if (streamer.readFloat(val)) setParamNormalized(kTapeHeadCutoff, val);
        if (streamer.readFloat(val)) setParamNormalized(kTapeSpeed, val);
    }

    if (version >= 3) {
        bool ce = false;
        if (streamer.readBool(ce)) setParamNormalized(kClipEnabled, ce ? 1.0 : 0.0);
        float val;
        if (streamer.readFloat(val)) setParamNormalized(kClipAmount, val);
    }

    return kResultOk;
}

IPlugView* PLUGIN_API BaseController::createView(FIDString name) {
    if (FIDStringsEqual(name, ViewType::kEditor)) {
        auto* view = new gui::ImGuiPlugView(this);
        activeView_ = view;

        if (meteringData_) {
            view->getUIState().metering = meteringData_;
        }

        return view;
    }
    return nullptr;
}

void BaseController::viewRemoved(gui::ImGuiPlugView* view) {
    if (activeView_ == view) {
        activeView_ = nullptr;
    }
}

tresult PLUGIN_API BaseController::setParamNormalized(ParamID tag, ParamValue value) {
    if ((tag == kMode || tag == kOversampling) && componentHandler) {
        ParamValue prev = getParamNormalized(tag);
        tresult res = EditController::setParamNormalized(tag, value);
        if (res == kResultOk && prev != value) {
            componentHandler->restartComponent(kLatencyChanged);
        }
        return res;
    }
    return EditController::setParamNormalized(tag, value);
}

tresult PLUGIN_API BaseController::getParamStringByValue(
    ParamID tag, ParamValue valueNormalized, String128 string) {
    return EditController::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API BaseController::getParamValueByString(
    ParamID tag, TChar* string, ParamValue& valueNormalized) {
    return EditController::getParamValueByString(tag, string, valueNormalized);
}

tresult PLUGIN_API BaseController::notify(IMessage* message) {
    if (!message)
        return kInvalidArgument;

    if (strcmp(message->getMessageID(), METERING_PTR_MESSAGE) == 0) {
        int64 ptrVal = 0;
        if (message->getAttributes()->getInt("ptr", ptrVal) == kResultTrue) {
            meteringData_ = reinterpret_cast<dsp::MeteringData*>(static_cast<intptr_t>(ptrVal));

            if (activeView_) {
                activeView_->getUIState().metering = meteringData_;
            }
        }
        return kResultOk;
    }

    return ComponentBase::notify(message);
}
