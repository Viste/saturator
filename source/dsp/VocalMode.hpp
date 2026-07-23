#pragma once

#include "SaturationMode.hpp"
#include "Oversampler.hpp"
#include "Waveshapers.hpp"
#include <q/fx/biquad.hpp>
#include <q/fx/dc_block.hpp>
#include <q/support/frequency.hpp>
#include <optional>
#include <array>
#include <vector>

namespace dsp {

class VocalMode : public SaturationMode {
public:
    struct Params {
        float saturation = 0.0f;
        float inputGain = 1.0f;
        float outputGain = 1.0f;
        float dryWet = 1.0f;
        float tapeBias = 0.5f;
        float wowAmount = 0.3f;
        float flutterAmount = 0.2f;
        float hissLevel = 0.0f;
        float headCutoff = 12000.0f;
        float tapeSpeed = 0.5f;
        Oversampler::Factor osFactor = Oversampler::kNone;
    };

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** in, float** out, int channels, int numSamples) override;
    void reset() override;

    Params& params() { return params_; }

private:
    Params params_;
    double sampleRate_ = 44100.0;

    float rampDrive_ = 0.0f;
    float rampFb_ = 0.0f;
    bool rampInit_ = false;

    struct ChannelState {
        TapeHysteresis hysteresis;
        std::optional<cycfi::q::lowpass> headRolloff;
        std::optional<cycfi::q::dc_block> dcBlock;
        Oversampler oversampler;

        float wowPhase = 0.0f;
        float flutterPhase = 0.0f;

        static constexpr int kMaxDelaySamples = 512;
        std::array<float, kMaxDelaySamples> delayBuffer{};
        int delayWritePos = 0;
    };

    std::array<ChannelState, 2> channels_;
    std::vector<float> dryBuf_;

    uint32_t noiseState_ = 0x67452301;
    float generateNoise();
};

} // namespace dsp
