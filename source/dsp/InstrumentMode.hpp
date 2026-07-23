#pragma once

#include "SaturationMode.hpp"
#include "MultibandSplitter.hpp"
#include "Oversampler.hpp"
#include "Waveshapers.hpp"
#include <q/fx/dc_block.hpp>
#include <q/support/frequency.hpp>
#include <optional>
#include <array>
#include <vector>

namespace dsp {

class InstrumentMode : public SaturationMode {
public:
    struct Params {
        float saturation = 0.0f;
        float inputGain = 1.0f;
        float outputGain = 1.0f;
        float dryWet = 1.0f;
        float lowSat = 1.0f;
        float midSat = 1.0f;
        float highSat = 1.0f;
        float lowMidFreq = 200.0f;
        float midHighFreq = 2500.0f;
        float character = 0.5f;
        Oversampler::Factor osFactor = Oversampler::kNone;
    };

    void prepare(double sampleRate, int maxBlockSize) override;
    void process(float** in, float** out, int channels, int numSamples) override;
    void reset() override;

    Params& params() { return params_; }

private:
    float processBand(float sample, float drive, float character, float& evenLp);

    Params params_;
    double sampleRate_ = 44100.0;

    float rampDrive_[3] = {};
    float rampChar_ = 0.5f;
    bool rampInit_ = false;
    float heldDrive_[2][3] = {};
    float heldChar_[2][3] = {};
    float prevSample_[2][3] = {};

    // highpass ~25Гц на чётной гармонике T2: у неё сигнало-зависимый DC
    float evenLp_[2][3] = {};
    float evenLpCoeff_ = 0.003f;

    std::array<MultibandSplitter, 2> splitters_;

    std::array<std::array<Oversampler, 3>, 2> oversamplers_;

    std::vector<float> bandBuf_[3];

    std::optional<cycfi::q::dc_block> dcBlockL_;
    std::optional<cycfi::q::dc_block> dcBlockR_;
};

} // namespace dsp
