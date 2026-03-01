#include "DrumMode.hpp"
#include <algorithm>
#include <cmath>

namespace dsp {

void DrumMode::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;
    float sps = static_cast<float>(sampleRate);

    // 2мс lookahead: детекция транзиента до его прихода
    lookaheadSamples_ = static_cast<int>(sampleRate * 0.002);
    if (lookaheadSamples_ < 1) lookaheadSamples_ = 1;

    size_t sz = static_cast<size_t>(maxBlockSize);
    delayedBuf_.resize(sz);
    gateBuf_.resize(sz);

    for (auto& ch : channels_) {
        // 3мс пиковый детектор для транзиентов
        ch.fastEnv.emplace(cycfi::q::duration(0.003), sps);

        // 15мс медленный фоловер — отстаёт от транзиентов
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
    float sensitivity = 1.0f + params_.transientSensitivity * 4.0f;
    float satDrive = params_.saturation * params_.sustainSat;

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

        state.oversampler.process(delayedBuf_.data(), out[ch], numSamples,
            [satDrive](float s) {
                return softSaturate(s, satDrive);
            });

        for (int i = 0; i < numSamples; ++i) {
            float delayed = delayedBuf_[i];
            float saturated = out[ch][i];
            float gate = gateBuf_[i];

            float punchAmount = params_.punch * gate;
            float punchBoost = 1.0f + params_.punch * 0.3f * gate;
            float transientClean = delayed * punchBoost;

            // gate=0 (сустейн) → сатурация, gate=1 (транзиент) → чистый+удар
            float wet = std::lerp(saturated, transientClean, punchAmount);

            if (state.dcBlock) {
                wet = (*state.dcBlock)(wet);
            }

            float mixed = std::lerp(delayed, wet, params_.dryWet);
            out[ch][i] = std::clamp(mixed * params_.outputGain, -1.0f, 1.0f);
        }
    }
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
}

} // namespace dsp
