#pragma once

#include "SaturationMode.hpp"
#include "Oversampler.hpp"
#include "Waveshapers.hpp"
#include <q/fx/envelope.hpp>
#include <q/fx/dc_block.hpp>
#include <q/support/frequency.hpp>
#include <q/support/duration.hpp>
#include <optional>
#include <array>
#include <vector>

namespace dsp {

class DrumMode : public SaturationMode {
public:
    struct Params {
        float saturation = 0.0f;
        float inputGain = 1.0f;
        float outputGain = 1.0f;
        float dryWet = 1.0f;
        float transientSensitivity = 0.5f;
        float attackMs = 3.0f;
        float sustainSat = 1.0f;
        float punch = 0.5f;
        Oversampler::Factor osFactor = Oversampler::kNone;
    };

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** in, float** out, int channels, int numSamples) override;
    void reset() override;
    int getLatencySamples() const override { return lookaheadSamples_; }

    Params& params() { return params_; }

private:
    Params params_;
    double sampleRate_ = 44100.0;
    int lookaheadSamples_ = 0;
    float lastAttackSec_ = 0.003f;

    float rampSat_ = 0.0f;
    float rampPunch_ = 0.0f;
    bool rampInit_ = false;

    struct ChannelState {
        std::optional<cycfi::q::fast_envelope_follower> fastEnv;
        std::optional<cycfi::q::ar_envelope_follower> slowEnv;
        std::optional<cycfi::q::dc_block> dcBlock;
        Oversampler oversampler;

        std::vector<float> lookaheadBuf;
        int lookaheadWritePos = 0;
        float lastAbs = 0.0f;
    };

    std::array<ChannelState, 2> channels_;

    std::vector<float> delayedBuf_;
    std::vector<float> gateBuf_;
};

} // namespace dsp
