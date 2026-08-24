#include "BaseProcessor.hpp"
#include "PluginIds.hpp"
#include "dsp/Waveshapers.hpp"
#include "MessagesConsts.hpp"
#include <base/source/fstreamer.h>
#include <public.sdk/source/vst/vstaudioprocessoralgo.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <immintrin.h>
#define SAT_HAS_SSE 1
#endif

namespace {
struct ScopedNoDenormals {
#if defined(SAT_HAS_SSE)
    unsigned int saved_;
    ScopedNoDenormals() : saved_(_mm_getcsr()) { _mm_setcsr(saved_ | 0x8040); }
    ~ScopedNoDenormals() { _mm_setcsr(saved_); }
#elif defined(__aarch64__)
    uint64_t saved_;
    ScopedNoDenormals() {
        __asm__ __volatile__("mrs %0, fpcr" : "=r"(saved_));
        __asm__ __volatile__("msr fpcr, %0" :: "r"(saved_ | (1ull << 24)));
    }
    ~ScopedNoDenormals() {
        __asm__ __volatile__("msr fpcr, %0" :: "r"(saved_));
    }
#endif
};
}

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

        for (auto& b : dryBuf_) b.assign(static_cast<size_t>(maxBlock), 0.0f);

        ParamSmoother* smoothers[] = {
            &smoothSaturation_, &smoothDryWet_, &smoothClipAmount_,
            &smoothInstLow_, &smoothInstMid_, &smoothInstHigh_, &smoothInstChar_,
            &smoothDrumSustain_, &smoothDrumPunch_, &smoothTapeBias_
        };
        for (auto* sm : smoothers) sm->prepare(sr, 20.0f);
        smoothSaturation_.reset(saturation_);
        smoothDryWet_.reset(dryWet_);
        smoothClipAmount_.reset(clipEnabled_ ? clipAmount_ : 0.0f);
        smoothInstLow_.reset(instLowSat_);
        smoothInstMid_.reset(instMidSat_);
        smoothInstHigh_.reset(instHighSat_);
        smoothInstChar_.reset(instCharacter_);
        smoothDrumSustain_.reset(drumSustainSat_);
        smoothDrumPunch_.reset(drumPunch_);
        smoothTapeBias_.reset(tapeBias_);

        bypassGainStep_ = 1.0f / (static_cast<float>(sr) * 0.002f);
        bypassGain_ = bypass_ ? 0.0f : 1.0f;

        switchGainStep_ = 1.0f / (static_cast<float>(sr) * 0.004f);
        switchGain_ = 1.0f;
        activeMode_ = algorithmMode_;
        activeOsMode_ = oversamplingMode_;

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
        if (!std::isfinite(v)) continue;
        v = std::clamp(v, 0.0f, 1.0f);

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

void BaseProcessor::updateModeParams(int blockSize) {
    auto normToGain = [](float norm) -> float {
        if (norm < 0.001f) return 0.0f;
        float db = (norm - 0.5f) * 24.0f; // 0.5 → 0dB, 1.0 → +12dB, 0.0 → -12dB
        return std::pow(10.0f, db / 20.0f);
    };
    float inGainLin = normToGain(inputGainNorm_);
    float outGainLin = normToGain(outputGainNorm_);

    float smoothedSat = smoothSaturation_.smooth(saturation_, blockSize);
    float smoothedMix = smoothDryWet_.smooth(dryWet_, blockSize);

    dsp::Oversampler::Factor osFactor;
    if (activeOsMode_ >= 2)
        osFactor = dsp::Oversampler::k4x;
    else if (activeOsMode_ >= 1)
        osFactor = dsp::Oversampler::k2x;
    else
        osFactor = dsp::Oversampler::kNone;

    auto& ip = instrumentMode_->params();
    ip.saturation = smoothedSat;
    ip.inputGain = inGainLin;
    ip.outputGain = outGainLin;
    ip.dryWet = smoothedMix;
    ip.lowSat = smoothInstLow_.smooth(instLowSat_, blockSize) * 2.0f;
    ip.midSat = smoothInstMid_.smooth(instMidSat_, blockSize) * 2.0f;
    ip.highSat = smoothInstHigh_.smooth(instHighSat_, blockSize) * 2.0f;
    ip.lowMidFreq = normToFreq(instLowMidFreq_, 50.0f, 1000.0f);
    ip.midHighFreq = normToFreq(instMidHighFreq_, 1000.0f, 8000.0f);
    ip.character = smoothInstChar_.smooth(instCharacter_, blockSize);
    ip.osFactor = osFactor;

    auto& dp = drumMode_->params();
    dp.saturation = smoothedSat;
    dp.inputGain = inGainLin;
    dp.outputGain = outGainLin;
    dp.dryWet = smoothedMix;
    dp.transientSensitivity = drumTransientSens_;
    dp.attackMs = 1.0f + drumAttackMs_ * 9.0f; // norm 0..1 → 1..10 ms
    dp.sustainSat = smoothDrumSustain_.smooth(drumSustainSat_, blockSize) * 2.0f;
    dp.punch = smoothDrumPunch_.smooth(drumPunch_, blockSize);
    dp.osFactor = osFactor;

    auto& vp = vocalMode_->params();
    vp.saturation = smoothedSat;
    vp.inputGain = inGainLin;
    vp.outputGain = outGainLin;
    vp.dryWet = smoothedMix;
    vp.tapeBias = smoothTapeBias_.smooth(tapeBias_, blockSize);
    vp.wowAmount = tapeWow_ * 0.3f;
    vp.flutterAmount = tapeFlutter_ * 0.2f;
    vp.hissLevel = tapeHissLevel_;
    vp.headCutoff = normToFreq(tapeHeadCutoff_, 4000.0f, 20000.0f);
    vp.tapeSpeed = tapeSpeed_;
    vp.osFactor = osFactor;
}

tresult PLUGIN_API BaseProcessor::process(ProcessData& data) {
    ScopedNoDenormals noDenormals;

    readParameterChanges(data);

    if (data.numInputs != 1 || data.numOutputs != 1)
        return kResultOk;

    int32 numChannels = data.inputs[0].numChannels;
    uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);

    float** in = data.inputs[0].channelBuffers32;
    float** out = data.outputs[0].channelBuffers32;

    float bypassTarget = bypass_ ? 0.0f : 1.0f;

    uint64 silentMask = (numChannels >= 64) ? ~0ull : ((1ull << numChannels) - 1ull);
    if ((data.inputs[0].silenceFlags & silentMask) == silentMask &&
        bypassTarget == 0.0f && bypassGain_ < 0.0001f) {
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
        for (int32 i = 0; i < numChannels; ++i) {
            if (in[i] != out[i])
                std::memset(out[i], 0, sampleFramesSize);
        }
        return kResultOk;
    }

    data.outputs[0].silenceFlags = 0;

    const int32 numSamples = std::min(
        data.numSamples, static_cast<int32>(dryBuf_[0].size()));
    if (numSamples <= 0) return kResultOk;
    for (int32 ch = 0; ch < numChannels; ++ch) {
        if (in[ch] != out[ch] && data.numSamples > numSamples) {
            std::memcpy(out[ch] + numSamples, in[ch] + numSamples,
                        static_cast<size_t>(data.numSamples - numSamples) * sizeof(float));
        }
    }

    int32 dryChannels = std::min(numChannels, 2);
    float* dry[2] = { dryBuf_[0].data(), dryBuf_[1].data() };
    for (int32 ch = 0; ch < dryChannels; ++ch) {
        const float* src = in[ch];
        float* dst = dry[ch];
        for (int32 i = 0; i < numSamples; ++i) {
            float v = src[i];
            dst[i] = std::isfinite(v) ? v : 0.0f;
        }
    }
    if (dryChannels < 2) dry[1] = dry[0];

    // дак: гейн в ноль на старом DSP, смена режима/ОС на границе блока в тишине
    if (switchGain_ <= 0.0f) {
        activeMode_ = algorithmMode_;
        activeOsMode_ = oversamplingMode_;
    }
    bool switching = (algorithmMode_ != activeMode_) || (oversamplingMode_ != activeOsMode_);

    updateModeParams(numSamples);

    switch (activeMode_) {
        case DRUM_MODE:
            drumMode_->process(dry, out, dryChannels, numSamples);
            break;
        case VOCAL_MODE:
            vocalMode_->process(dry, out, dryChannels, numSamples);
            break;
        default:
            instrumentMode_->process(dry, out, dryChannels, numSamples);
            break;
    }

    for (int32 ch = 2; ch < numChannels; ++ch) {
        if (in[ch] != out[ch])
            std::memcpy(out[ch], in[ch], static_cast<size_t>(numSamples) * sizeof(float));
    }

    int32 fadeChannels = dryChannels;

    float switchTarget = switching ? 0.0f : 1.0f;
    if (switchGain_ != switchTarget) {
        for (int32 i = 0; i < numSamples; ++i) {
            if (switchGain_ < switchTarget)
                switchGain_ = std::min(switchGain_ + switchGainStep_, 1.0f);
            else if (switchGain_ > switchTarget)
                switchGain_ = std::max(switchGain_ - switchGainStep_, 0.0f);
            for (int32 ch = 0; ch < fadeChannels; ++ch) {
                out[ch][i] *= switchGain_;
            }
        }
    }

    // клипер: amount рампится по сэмплам; цель 0 при выключении — деклик toggle
    {
        float clipStart = smoothClipAmount_.current;
        float clipEnd = smoothClipAmount_.smooth(
            clipEnabled_ ? clipAmount_ : 0.0f, numSamples);
        if (clipStart > 0.001f || clipEnd > 0.001f) {
            float clipStep = (clipEnd - clipStart) / static_cast<float>(numSamples);
            for (int32 ch = 0; ch < fadeChannels; ++ch) {
                float amount = clipStart;
                for (int32 i = 0; i < numSamples; ++i) {
                    amount += clipStep;
                    out[ch][i] = dsp::softClip(out[ch][i], amount);
                }
            }
        }
    }

    for (int32 i = 0; i < numSamples; ++i) {
        if (bypassGain_ < bypassTarget)
            bypassGain_ = std::min(bypassGain_ + bypassGainStep_, 1.0f);
        else if (bypassGain_ > bypassTarget)
            bypassGain_ = std::max(bypassGain_ - bypassGainStep_, 0.0f);

        if (bypassGain_ < 1.0f) {
            float dryMix = 1.0f - bypassGain_;
            for (int32 ch = 0; ch < fadeChannels; ++ch) {
                out[ch][i] = out[ch][i] * bypassGain_ + dry[ch][i] * dryMix;
            }
        }
    }

    for (int32 i = 0; i < numSamples; ++i) {
        float outL = out[0][i];
        float outR = (numChannels > 1) ? out[1][i] : outL;

        metering_.pushInputSample(dry[0][i], dry[1][i]);
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
        if (streamer.readInt32(savedMode)) algorithmMode_ = std::clamp(savedMode, 0, 2);

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
        oversamplingMode_ = std::clamp(os, 0, 2);

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

    streamer.writeBool(clipEnabled_);
    streamer.writeFloat(clipAmount_);

    return kResultOk;
}

uint32 BaseProcessor::getLatencySamples() {
    static constexpr uint32 kOsLatency[3] = { 0, 16, 24 };
    uint32 lat = kOsLatency[std::clamp(oversamplingMode_, 0, 2)];
    if (algorithmMode_ == DRUM_MODE && drumMode_) {
        lat += static_cast<uint32>(drumMode_->getLatencySamples());
    }
    return lat;
}

tresult PLUGIN_API BaseProcessor::notify(IMessage* message) {
    return ComponentBase::notify(message);
}
