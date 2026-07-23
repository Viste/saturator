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

    dcBlockL_.emplace(cycfi::q::frequency(10.0), static_cast<float>(sampleRate));
    dcBlockR_.emplace(cycfi::q::frequency(10.0), static_cast<float>(sampleRate));
}

void InstrumentMode::process(float** in, float** out, int channels, int numSamples) {
    int chCount = std::min(channels, 2);
    if (numSamples <= 0) return;

    float mix = params_.dryWet;
    float targetDrive[3] = {
        params_.lowSat * params_.saturation * mix,
        params_.midSat * params_.saturation * mix,
        params_.highSat * params_.saturation * mix
    };
    float targetChar = params_.character;
    if (!rampInit_) {
        for (int ch = 0; ch < 2; ++ch) {
            for (int b = 0; b < 3; ++b) {
                heldDrive_[ch][b] = targetDrive[b];
                heldChar_[ch][b] = targetChar;
            }
        }
        for (int b = 0; b < 3; ++b) rampDrive_[b] = targetDrive[b];
        rampChar_ = targetChar;
        rampInit_ = true;
    }

    evenLpCoeff_ = std::min(1.0f, 2.0f * 3.14159265f * 25.0f /
        (static_cast<float>(sampleRate_) * static_cast<float>(static_cast<int>(params_.osFactor))));

    for (int ch = 0; ch < chCount; ++ch) {
        auto& splitter = splitters_[ch];
        splitter.reconfigure(params_.lowMidFreq, params_.midHighFreq);

        for (int i = 0; i < numSamples; ++i) {
            float dry = in[ch][i] * params_.inputGain;

            auto bands = splitter.split(dry);
            bandBuf_[0][i] = bands.low;
            bandBuf_[1][i] = bands.mid;
            bandBuf_[2][i] = bands.high;
        }

        for (int b = 0; b < 3; ++b) {
            auto& os = oversamplers_[ch][b];
            os.setFactor(params_.osFactor);
            float osN = static_cast<float>(numSamples * static_cast<int>(params_.osFactor));
            float d = rampDrive_[b];
            float dStep = (targetDrive[b] - d) / osN;
            float c = rampChar_;
            float cStep = (targetChar - c) / osN;

            os.process(bandBuf_[b].data(), bandBuf_[b].data(), numSamples,
                [this, d, dStep, c, cStep, &lp = evenLp_[ch][b],
                 &held = heldDrive_[ch][b], &heldC = heldChar_[ch][b],
                 &prev = prevSample_[ch][b]](float s) mutable {
                    d += dStep;
                    c += cStep;
                    // драйв защёлкивается раз в период, иначе DC (бас-толчок)
                    if (prev < 0.0f && s >= 0.0f) {
                        held = d;
                        heldC = c;
                    }
                    prev = s;
                    return processBand(s, held, heldC, lp);
                });
        }

        for (int i = 0; i < numSamples; ++i) {
            float wet = bandBuf_[0][i] + bandBuf_[1][i] + bandBuf_[2][i];

            if (ch == 0 && dcBlockL_) {
                wet = (*dcBlockL_)(wet);
            } else if (ch == 1 && dcBlockR_) {
                wet = (*dcBlockR_)(wet);
            }

            out[ch][i] = std::clamp(wet * params_.outputGain, -1.0f, 1.0f);
        }
    }

    for (int b = 0; b < 3; ++b) rampDrive_[b] = targetDrive[b];
    rampChar_ = targetChar;
}

// character: 0=tanh, 1=+T2; T2 через highpass — его DC иначе едет за драйвом
float InstrumentMode::processBand(float sample, float drive, float character, float& evenLp) {
    if (drive < 0.001f) return sample;

    float odd = softSaturate(sample, drive);
    if (character < 0.001f) return odd;

    float xc = std::clamp(odd, -1.0f, 1.0f);
    float t2 = 2.0f * xc * xc - 1.0f;
    evenLp += evenLpCoeff_ * (t2 - evenLp);
    return odd + (t2 - evenLp) * (drive * 0.15f) * character;
}

void InstrumentMode::reset() {
    for (auto& s : splitters_) s.reset();
    for (auto& chOs : oversamplers_) {
        for (auto& os : chOs) os.reset();
    }
    if (dcBlockL_) dcBlockL_.reset();
    if (dcBlockR_) dcBlockR_.reset();
    rampInit_ = false;
    for (int ch = 0; ch < 2; ++ch) {
        for (int b = 0; b < 3; ++b) {
            evenLp_[ch][b] = 0.0f;
            prevSample_[ch][b] = 0.0f;
        }
    }
}

} // namespace dsp
