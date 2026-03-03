#include "BaseProcessor.hpp"
#include "PluginIds.hpp"
#include "dsp/Waveshapers.hpp"
#include "MessagesConsts.hpp"
#include <base/source/fstreamer.h>
#include <public.sdk/source/vst/vstaudioprocessoralgo.h>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

BaseProcessor::BaseProcessor() {
    setControllerClass(FUID::fromTUID(kSaturatorControllerUID));
}

BaseProcessor::~BaseProcessor() = default;

tresult PLUGIN_API BaseProcessor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
        return result;

    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo, BusTypes::kMain);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo, BusTypes::kMain);

    instrumentMode_ = std::make_unique<dsp::InstrumentMode>();
    drumMode_ = std::make_unique<dsp::DrumMode>();
    vocalMode_ = std::make_unique<dsp::VocalMode>();

    return kResultOk;
}

tresult PLUGIN_API BaseProcessor::terminate() {
    instrumentMode_.reset();
    drumMode_.reset();
    vocalMode_.reset();
    return AudioEffect::terminate();
}

tresult PLUGIN_API BaseProcessor::setActive(TBool state) {
    if (state) {
        int maxBlock = processSetup.maxSamplesPerBlock;
        double sr = processSetup.sampleRate;

        instrumentMode_->prepare(sr, maxBlock);
        drumMode_->prepare(sr, maxBlock);
        vocalMode_->prepare(sr, maxBlock);

        metering_.sampleRate.store(static_cast<float>(sr), std::memory_order_relaxed);
        sendMeteringPointer();
    } else {
        instrumentMode_->reset();
        drumMode_->reset();
        vocalMode_->reset();
    }
    return AudioEffect::setActive(state);
}

void BaseProcessor::sendMeteringPointer() {
    if (auto* msg = allocateMessage()) {
        msg->setMessageID(METERING_PTR_MESSAGE);
        msg->getAttributes()->setInt("ptr",
            static_cast<int64>(reinterpret_cast<intptr_t>(&metering_)));
        sendMessage(msg);
        msg->release();
    }
}

tresult PLUGIN_API BaseProcessor::setupProcessing(ProcessSetup& newSetup) {
    newSetup.processMode = ProcessModes::kRealtime;
    processSetup = newSetup;
    return AudioEffect::setupProcessing(newSetup);
}

tresult BaseProcessor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                           SpeakerArrangement* outputs, int32 numOuts) {
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == SpeakerArr::kStereo &&
        outputs[0] == SpeakerArr::kStereo) {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }
    return kResultFalse;
}

tresult PLUGIN_API BaseProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    if (symbolicSampleSize == kSample32)
        return kResultTrue;
    return kResultFalse;
}

void BaseProcessor::readParameterChanges(ProcessData& data) {
    if (!data.inputParameterChanges)
        return;

    int32 numChanges = data.inputParameterChanges->getParameterCount();
    for (int32 index = 0; index < numChanges; ++index) {
        auto* queue = data.inputParameterChanges->getParameterData(index);
        if (!queue) continue;

        ParamValue value;
        int32 sampleOffset;
        int32 lastIdx = queue->getPointCount() - 1;
        if (lastIdx < 0) continue;
        if (queue->getPoint(lastIdx, sampleOffset, value) != kResultTrue)
            continue;

        float v = static_cast<float>(value);

        switch (queue->getParameterId()) {
            case kBypass:        bypass_ = (v > 0.5f); break;
            case kMode:          algorithmMode_ = std::clamp(static_cast<int>(v * 2.0f + 0.5f), 0, 2); break;
            case kSaturation:    saturation_ = v; break;
            case kInputGain:     inputGainNorm_ = v; break;
            case kOutputGain:    outputGainNorm_ = v; break;
            case kDryWet:        dryWet_ = v; break;
            case kOversampling:  oversamplingMode_ = static_cast<int>(v * 2.0f + 0.5f); break;

            case kInstLowSat:      instLowSat_ = v; break;
            case kInstMidSat:      instMidSat_ = v; break;
            case kInstHighSat:     instHighSat_ = v; break;
            case kInstLowMidFreq:  instLowMidFreq_ = v; break;
            case kInstMidHighFreq: instMidHighFreq_ = v; break;
            case kInstCharacter:   instCharacter_ = v; break;

            case kDrumTransientSens: drumTransientSens_ = v; break;
            case kDrumAttackMs:      drumAttackMs_ = v; break;
            case kDrumSustainSat:    drumSustainSat_ = v; break;
            case kDrumPunch:         drumPunch_ = v; break;

            case kTapeBias:       tapeBias_ = v; break;
            case kTapeWow:        tapeWow_ = v; break;
            case kTapeFlutter:    tapeFlutter_ = v; break;
            case kTapeHissLevel:  tapeHissLevel_ = v; break;
            case kTapeHeadCutoff: tapeHeadCutoff_ = v; break;
            case kTapeSpeed:      tapeSpeed_ = v; break;

            case kClipEnabled:    clipEnabled_ = (v > 0.5f); break;
            case kClipAmount:     clipAmount_ = v; break;
        }
    }
}

void BaseProcessor::updateModeParams() {
    float inGainLin = 1.0f;
    float outGainLin = 1.0f;

    // маппинг оверсемплинга: 0=выкл, 1=2x, 2=4x
    dsp::Oversampler::Factor osFactor;
    if (oversamplingMode_ >= 2)
        osFactor = dsp::Oversampler::k4x;
    else if (oversamplingMode_ >= 1)
        osFactor = dsp::Oversampler::k2x;
    else
        osFactor = dsp::Oversampler::kNone;

    // инструмент
    auto& ip = instrumentMode_->params();
    ip.saturation = saturation_;
    ip.inputGain = inGainLin;
    ip.outputGain = outGainLin;
    ip.dryWet = dryWet_;
    ip.lowSat = instLowSat_ * 2.0f;
    ip.midSat = instMidSat_ * 2.0f;
    ip.highSat = instHighSat_ * 2.0f;
    ip.lowMidFreq = normToFreq(instLowMidFreq_, 50.0f, 1000.0f);
    ip.midHighFreq = normToFreq(instMidHighFreq_, 1000.0f, 8000.0f);
    ip.character = instCharacter_;
    ip.osFactor = osFactor;

    // ударные
    auto& dp = drumMode_->params();
    dp.saturation = saturation_;
    dp.inputGain = inGainLin;
    dp.outputGain = outGainLin;
    dp.dryWet = dryWet_;
    dp.transientSensitivity = drumTransientSens_;
    dp.sustainSat = drumSustainSat_ * 2.0f;
    dp.punch = drumPunch_;
    dp.osFactor = osFactor;

    // вокал/лента
    auto& vp = vocalMode_->params();
    vp.saturation = saturation_;
    vp.inputGain = inGainLin;
    vp.outputGain = outGainLin;
    vp.dryWet = dryWet_;
    vp.tapeBias = tapeBias_;
    vp.wowAmount = tapeWow_ * 0.3f;
    vp.flutterAmount = tapeFlutter_ * 0.2f;
    vp.hissLevel = tapeHissLevel_;
    vp.headCutoff = normToFreq(tapeHeadCutoff_, 4000.0f, 20000.0f);
    vp.tapeSpeed = tapeSpeed_;
    vp.osFactor = osFactor;
}

tresult PLUGIN_API BaseProcessor::process(ProcessData& data) {
    readParameterChanges(data);

    if (data.numInputs != 1 || data.numOutputs != 1)
        return kResultOk;

    int32 numChannels = data.inputs[0].numChannels;
    uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);

    float** in = data.inputs[0].channelBuffers32;
    float** out = data.outputs[0].channelBuffers32;

    if (bypass_) {
        for (int32 i = 0; i < numChannels; ++i) {
            if (in[i] != out[i])
                std::memcpy(out[i], in[i], sampleFramesSize);
        }
        return kResultOk;
    }

    if (data.inputs[0].silenceFlags != 0) {
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
        for (int32 i = 0; i < numChannels; ++i) {
            if (in[i] != out[i])
                std::memset(out[i], 0, sampleFramesSize);
        }
        return kResultOk;
    }

    data.outputs[0].silenceFlags = 0;
    updateModeParams();

    switch (algorithmMode_) {
        case INSTRUMENT_MODE:
            instrumentMode_->process(in, out, numChannels, data.numSamples);
            break;
        case DRUM_MODE:
            drumMode_->process(in, out, numChannels, data.numSamples);
            break;
        case VOCAL_MODE:
            vocalMode_->process(in, out, numChannels, data.numSamples);
            break;
    }

    if (clipEnabled_ && clipAmount_ > 0.001f) {
        for (int32 ch = 0; ch < numChannels; ++ch) {
            for (int32 i = 0; i < data.numSamples; ++i) {
                out[ch][i] = dsp::softClip(out[ch][i], clipAmount_);
            }
        }
    }

    for (int i = 0; i < data.numSamples; ++i) {
        float inL = in[0][i];
        float inR = (numChannels > 1) ? in[1][i] : inL;
        float outL = out[0][i];
        float outR = (numChannels > 1) ? out[1][i] : outL;

        metering_.pushInputSample(inL, inR);
        metering_.pushOutputSample(outL, outR);
    }

    return kResultOk;
}

tresult PLUGIN_API BaseProcessor::setState(IBStream* state) {
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    int32 version = 0;
    if (!streamer.readInt32(version))
        return kResultFalse;

    if (version == 0) {
        // совместимость с v1: поле version было bypass (0=выкл)
        bypass_ = false;

        float savedSat = 0.0f;
        if (streamer.readFloat(savedSat)) saturation_ = savedSat;

        int32 savedMode = 0;
        if (streamer.readInt32(savedMode)) algorithmMode_ = savedMode;

        float savedGain = 0.5f;
        if (streamer.readFloat(savedGain)) {
            inputGainNorm_ = savedGain;
            outputGainNorm_ = 0.5f;
        }
        return kResultOk;
    }

    if (version >= 2) {
        bool bp = false;
        if (!streamer.readBool(bp)) return kResultFalse;
        bypass_ = bp;

        int32 mode = 0;
        streamer.readInt32(mode);
        algorithmMode_ = std::clamp(mode, 0, 2);

        streamer.readFloat(saturation_);
        streamer.readFloat(inputGainNorm_);
        streamer.readFloat(outputGainNorm_);
        streamer.readFloat(dryWet_);

        int32 os = 0;
        streamer.readInt32(os);
        oversamplingMode_ = os;

        streamer.readFloat(instLowSat_);
        streamer.readFloat(instMidSat_);
        streamer.readFloat(instHighSat_);
        streamer.readFloat(instLowMidFreq_);
        streamer.readFloat(instMidHighFreq_);
        streamer.readFloat(instCharacter_);

        streamer.readFloat(drumTransientSens_);
        streamer.readFloat(drumAttackMs_);
        streamer.readFloat(drumSustainSat_);
        streamer.readFloat(drumPunch_);

        streamer.readFloat(tapeBias_);
        streamer.readFloat(tapeWow_);
        streamer.readFloat(tapeFlutter_);
        streamer.readFloat(tapeHissLevel_);
        streamer.readFloat(tapeHeadCutoff_);
        streamer.readFloat(tapeSpeed_);
    }

    if (version >= 3) {
        bool ce = false;
        if (streamer.readBool(ce)) clipEnabled_ = ce;
        streamer.readFloat(clipAmount_);
    }

    return kResultOk;
}

tresult PLUGIN_API BaseProcessor::getState(IBStream* state) {
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);

    streamer.writeInt32(3);
    streamer.writeBool(bypass_);
    streamer.writeInt32(algorithmMode_);
    streamer.writeFloat(saturation_);
    streamer.writeFloat(inputGainNorm_);
    streamer.writeFloat(outputGainNorm_);
    streamer.writeFloat(dryWet_);
    streamer.writeInt32(oversamplingMode_);

    streamer.writeFloat(instLowSat_);
    streamer.writeFloat(instMidSat_);
    streamer.writeFloat(instHighSat_);
    streamer.writeFloat(instLowMidFreq_);
    streamer.writeFloat(instMidHighFreq_);
    streamer.writeFloat(instCharacter_);

    streamer.writeFloat(drumTransientSens_);
    streamer.writeFloat(drumAttackMs_);
    streamer.writeFloat(drumSustainSat_);
    streamer.writeFloat(drumPunch_);

    streamer.writeFloat(tapeBias_);
    streamer.writeFloat(tapeWow_);
    streamer.writeFloat(tapeFlutter_);
    streamer.writeFloat(tapeHissLevel_);
    streamer.writeFloat(tapeHeadCutoff_);
    streamer.writeFloat(tapeSpeed_);

    // v3: клиппер
    streamer.writeBool(clipEnabled_);
    streamer.writeFloat(clipAmount_);

    return kResultOk;
}

uint32 BaseProcessor::getLatencySamples() {
    if (algorithmMode_ == DRUM_MODE && drumMode_) {
        return static_cast<uint32>(drumMode_->getLatencySamples());
    }
    return 0;
}

tresult PLUGIN_API BaseProcessor::notify(IMessage* message) {
    return ComponentBase::notify(message);
}
