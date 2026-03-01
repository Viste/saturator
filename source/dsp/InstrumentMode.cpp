#include "InstrumentMode.hpp"
#include <algorithm>
#include <cmath>

namespace dsp {

void InstrumentMode::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;

    for (auto& splitter : splitters_) {
        splitter.prepare(static_cast<float>(sampleRate));
    }
    for (auto& chOs : oversamplers_) {
        for (auto& os : chOs) {
            os.prepare(maxBlockSize);
        }
    }

    size_t sz = static_cast<size_t>(maxBlockSize);
    for (auto& buf : bandBuf_) buf.resize(sz);
    dryBuf_.resize(sz);

    dcBlockL_.emplace(cycfi::q::frequency(10.0), static_cast<float>(sampleRate));
    dcBlockR_.emplace(cycfi::q::frequency(10.0), static_cast<float>(sampleRate));
}

void InstrumentMode::process(float** in, float** out, int channels, int numSamples) {
    int chCount = std::min(channels, 2);

    for (int ch = 0; ch < chCount; ++ch) {
        auto& splitter = splitters_[ch];
        splitter.reconfigure(params_.lowMidFreq, params_.midHighFreq);

        float lowDrive = params_.lowSat * params_.saturation;
        float midDrive = params_.midSat * params_.saturation;
        float highDrive = params_.highSat * params_.saturation;

        for (int i = 0; i < numSamples; ++i) {
            float dry = in[ch][i] * params_.inputGain;
            dryBuf_[i] = dry;

            auto bands = splitter.split(dry);
            bandBuf_[0][i] = bands.low;
            bandBuf_[1][i] = bands.mid;
            bandBuf_[2][i] = bands.high;
        }

        // сатурация каждой полосы с оверсемплингом
        float drives[3] = { lowDrive, midDrive, highDrive };
        for (int b = 0; b < 3; ++b) {
            auto& os = oversamplers_[ch][b];
            os.setFactor(params_.osFactor);
            float d = drives[b];

            os.process(bandBuf_[b].data(), bandBuf_[b].data(), numSamples,
                [this, d](float s) { return processBand(s, d); });
        }

        for (int i = 0; i < numSamples; ++i) {
            float wet = bandBuf_[0][i] + bandBuf_[1][i] + bandBuf_[2][i];

            if (ch == 0 && dcBlockL_) {
                wet = (*dcBlockL_)(wet);
            } else if (ch == 1 && dcBlockR_) {
                wet = (*dcBlockR_)(wet);
            }

            float mixed = std::lerp(dryBuf_[i], wet, params_.dryWet);
            out[ch][i] = std::clamp(mixed * params_.outputGain, -1.0f, 1.0f);
        }
    }
}

float InstrumentMode::processBand(float sample, float drive) {
    if (drive < 0.001f) return sample;

    // character=0: нечётные гармоники (tanh), character=1: + чётные (tube T2)
    float warm = softSaturate(sample, drive);
    float tubed = tubeSaturate(sample, drive);
    return std::lerp(warm, tubed, params_.character);
}

void InstrumentMode::reset() {
    for (auto& s : splitters_) s.reset();
    for (auto& chOs : oversamplers_) {
        for (auto& os : chOs) os.reset();
    }
    if (dcBlockL_) dcBlockL_.reset();
    if (dcBlockR_) dcBlockR_.reset();
}

} // namespace dsp
