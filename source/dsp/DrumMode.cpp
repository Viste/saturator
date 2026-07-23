#include "DrumMode.hpp"
#include <algorithm>
#include <cmath>

namespace dsp {

void DrumMode::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;
    float sps = static_cast<float>(sampleRate);

    lookaheadSamples_ = static_cast<int>(sampleRate * 0.002);
    if (lookaheadSamples_ < 1) lookaheadSamples_ = 1;

    size_t sz = static_cast<size_t>(maxBlockSize);
    delayedBuf_.resize(sz);
    gateBuf_.resize(sz);

    for (auto& ch : channels_) {
        ch.fastEnv.emplace(cycfi::q::duration(0.003), sps);

        ch.slowEnv.emplace(
            cycfi::q::duration(0.015),
            cycfi::q::duration(0.080),
            sps
        );
        ch.dcBlock.emplace(cycfi::q::frequency(10.0), sps);
        ch.oversampler.prepare(maxBlockSize);

        ch.lookaheadBuf.assign(static_cast<size_t>(lookaheadSamples_), 0.0f);
        ch.lookaheadWritePos = 0;
    }
}

void DrumMode::process(float** in, float** out, int channels, int numSamples) {
    int chCount = std::min(channels, 2);
    if (numSamples <= 0) return;
    float sensitivity = 1.0f + params_.transientSensitivity * 4.0f;
    float mix = params_.dryWet;
    float targetSat = params_.saturation * params_.sustainSat * mix;
    float targetPunch = params_.punch * mix;
    if (!rampInit_) {
        rampSat_ = targetSat;
        rampPunch_ = targetPunch;
        rampInit_ = true;
    }

    float attackSec = params_.attackMs * 0.001f;
    if (std::abs(attackSec - lastAttackSec_) > 0.0001f) {
        lastAttackSec_ = attackSec;
        float sps = static_cast<float>(sampleRate_);
        for (auto& ch : channels_) {
            ch.fastEnv.emplace(cycfi::q::duration(attackSec), sps);
        }
    }

    for (int ch = 0; ch < chCount; ++ch) {
        auto& state = channels_[ch];
        int laSize = static_cast<int>(state.lookaheadBuf.size());
        if (laSize < 1) continue;

        state.oversampler.setFactor(params_.osFactor);

        for (int i = 0; i < numSamples; ++i) {
            float current = in[ch][i] * params_.inputGain;
            float absCurrent = std::abs(current);
            float fastLevel = (*state.fastEnv)(absCurrent);
            float slowLevel = (*state.slowEnv)(absCurrent);
            float transientAmount = std::max(0.0f, fastLevel - slowLevel);
            float gate = std::clamp(transientAmount * sensitivity, 0.0f, 1.0f);
            gateBuf_[i] = gate * gate;

            int readPos = state.lookaheadWritePos;
            delayedBuf_[i] = state.lookaheadBuf[readPos];

            state.lookaheadBuf[state.lookaheadWritePos] = current;
            state.lookaheadWritePos = (state.lookaheadWritePos + 1) % laSize;
        }

        {
            float osN = static_cast<float>(numSamples * static_cast<int>(params_.osFactor));
            float d = rampSat_;
            float dStep = (targetSat - d) / osN;
            state.oversampler.process(delayedBuf_.data(), out[ch], numSamples,
                [d, dStep](float s) mutable {
                    d += dStep;
                    return softSaturate(s, d);
                });
        }

        float punchStep = (targetPunch - rampPunch_) / static_cast<float>(numSamples);
        float punchMix = rampPunch_;
        for (int i = 0; i < numSamples; ++i) {
            punchMix += punchStep;
            float delayed = delayedBuf_[i];
            float saturated = out[ch][i];
            float gate = gateBuf_[i];

            float punchAmount = punchMix * gate;
            float punchBoost = 1.0f + 0.3f * punchMix * gate;
            float transientClean = delayed * punchBoost;

            float wet = std::lerp(saturated, transientClean, punchAmount);

            if (state.dcBlock) {
                wet = (*state.dcBlock)(wet);
            }

            out[ch][i] = std::clamp(wet * params_.outputGain, -1.0f, 1.0f);
        }
    }

    rampSat_ = targetSat;
    rampPunch_ = targetPunch;
}

void DrumMode::reset() {
    for (auto& ch : channels_) {
        ch.fastEnv.reset();
        ch.slowEnv.reset();
        ch.dcBlock.reset();
        ch.oversampler.reset();
        std::fill(ch.lookaheadBuf.begin(), ch.lookaheadBuf.end(), 0.0f);
        ch.lookaheadWritePos = 0;
    }
    rampInit_ = false;
}

} // namespace dsp
