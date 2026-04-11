#pragma once

#include "dsp/InstrumentMode.hpp"
#include "dsp/DrumMode.hpp"
#include "dsp/VocalMode.hpp"
#include "dsp/MeteringData.hpp"
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <memory>
#include <cmath>

// one-pole parameter smoother (~5ms)
struct ParamSmoother {
    float current = 0.0f;
    float sampleCoeff = 0.999f; // per-sample коэффициент

    void prepare(double sampleRate, float timeMs = 5.0f) {
        sampleCoeff = std::exp(-1.0f / (static_cast<float>(sampleRate) * timeMs * 0.001f));
    }
    // вызывается раз в блок — корректно скейлится по размеру блока
    float smooth(float target, int blockSize) {
        float blockCoeff = std::pow(sampleCoeff, static_cast<float>(blockSize));
        current = current * blockCoeff + target * (1.0f - blockCoeff);
        return current;
    }
    void reset(float v) { current = v; }
};

class BaseProcessor : public Steinberg::Vst::AudioEffect {
public:
    BaseProcessor();
    ~BaseProcessor() override;

    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new BaseProcessor);
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) override;
    Steinberg::tresult setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                          Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
    Steinberg::uint32 getLatencySamples() override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

private:
    void readParameterChanges(Steinberg::Vst::ProcessData& data);
    void updateModeParams(int blockSize);
    void sendMeteringPointer();

    std::unique_ptr<dsp::InstrumentMode> instrumentMode_;
    std::unique_ptr<dsp::DrumMode> drumMode_;
    std::unique_ptr<dsp::VocalMode> vocalMode_;

    dsp::MeteringData metering_;

    // глобальные параметры (нормализованные 0..1)
    float saturation_ = 0.0f;
    float inputGainNorm_ = 0.5f;
    float outputGainNorm_ = 0.5f;
    float dryWet_ = 1.0f;
    int oversamplingMode_ = 0;
    int algorithmMode_ = 0;
    bool bypass_ = false;

    // инструмент
    float instLowSat_ = 0.5f;
    float instMidSat_ = 0.5f;
    float instHighSat_ = 0.5f;
    float instLowMidFreq_ = 0.3f;
    float instMidHighFreq_ = 0.3f;
    float instCharacter_ = 0.5f;

    // ударные
    float drumTransientSens_ = 0.5f;
    float drumAttackMs_ = 0.25f;
    float drumSustainSat_ = 0.5f;
    float drumPunch_ = 0.5f;

    // вокал/лента
    float tapeBias_ = 0.5f;
    float tapeWow_ = 0.3f;
    float tapeFlutter_ = 0.2f;
    float tapeHissLevel_ = 0.0f;
    float tapeHeadCutoff_ = 0.5f;
    float tapeSpeed_ = 0.5f;

    // клиппер
    bool clipEnabled_ = false;
    float clipAmount_ = 0.0f;

    // сглаживание параметров (anti-zipper)
    ParamSmoother smoothSaturation_;
    ParamSmoother smoothDryWet_;
    ParamSmoother smoothClipAmount_;

    // bypass fade (anti-click)
    float bypassGain_ = 0.0f;      // 0=bypassed, 1=active
    float bypassGainStep_ = 0.0f;  // шаг fade per sample

    // mode/OS switch crossfade (anti-click)
    int prevMode_ = 0;             // предыдущий режим
    int prevOsMode_ = 0;           // предыдущий oversampling
    int modeFadeSamples_ = 0;      // оставшиеся семплы fade
    int modeFadeTotal_ = 0;        // всего семплов fade (~5ms)
    std::vector<float> modeFadeBuf_[2]; // буфер старого выхода (L/R)

    static constexpr int INSTRUMENT_MODE = 0;
    static constexpr int DRUM_MODE = 1;
    static constexpr int VOCAL_MODE = 2;

    static float normToFreq(float norm, float minHz, float maxHz) {
        return minHz * std::pow(maxHz / minHz, norm);
    }
};
