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

// мультиполосная сатурация: lr4 кроссовер → побандовый tube/tanh → сборка
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
    float processBand(float sample, float drive);

    Params params_;
    double sampleRate_ = 44100.0;

    std::array<MultibandSplitter, 2> splitters_;

    // 3 оверсемплера на канал (нч/сч/вч)
    std::array<std::array<Oversampler, 3>, 2> oversamplers_;

    std::vector<float> bandBuf_[3];

    std::optional<cycfi::q::dc_block> dcBlockL_;
    std::optional<cycfi::q::dc_block> dcBlockR_;
};

} // namespace dsp
