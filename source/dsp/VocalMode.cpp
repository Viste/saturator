#include "VocalMode.hpp"
#include <algorithm>
#include <cmath>

using namespace cycfi::q;

namespace dsp {

void VocalMode::prepare(double sampleRate, int maxBlockSize) {
    sampleRate_ = sampleRate;
    float sps = static_cast<float>(sampleRate);

    dryBuf_.resize(static_cast<size_t>(maxBlockSize));

    for (auto& ch : channels_) {
        ch.headRolloff.emplace(cycfi::q::frequency(params_.headCutoff), sps);
        ch.dcBlock.emplace(cycfi::q::frequency(10.0), sps);
        ch.hysteresis.reset();
        ch.oversampler.prepare(maxBlockSize);
        ch.wowPhase = 0.0f;
        ch.flutterPhase = 0.0f;
        ch.delayBuffer.fill(0.0f);
        ch.delayWritePos = 0;
    }
}

void VocalMode::process(float** in, float** out, int channels, int numSamples) {
    int chCount = std::min(channels, 2);

    float wowFreq = 1.5f;
    float flutterFreq = 7.0f;
    float wowDepth = params_.wowAmount * 0.003f;
    float flutterDepth = params_.flutterAmount * 0.0005f;
    float baseDelay = 8.0f;

    float satDrive = params_.saturation;
    float feedback = 0.1f + params_.tapeBias * 0.2f;

    for (int ch = 0; ch < chCount; ++ch) {
        auto& state = channels_[ch];

        // учёт оверсемплинга в частоте дискретизации гистерезиса
        float effectiveSr = static_cast<float>(sampleRate_) * static_cast<float>(params_.osFactor);
        state.hysteresis.setParams(satDrive, feedback, effectiveSr);
        state.oversampler.setFactor(params_.osFactor);

        if (state.headRolloff) {
            state.headRolloff->config(cycfi::q::frequency(params_.headCutoff),
                                       static_cast<float>(sampleRate_));
        }

        for (int i = 0; i < numSamples; ++i) {
            dryBuf_[i] = in[ch][i] * params_.inputGain;
        }

        state.oversampler.process(dryBuf_.data(), out[ch], numSamples,
            [&state](float s) { return state.hysteresis.process(s); });

        // wow/flutter + фильтр головки + шум + dc block
        for (int i = 0; i < numSamples; ++i) {
            float saturated = out[ch][i];

            // синусоидальная модуляция задержки (wow & flutter)
            float wowMod = std::sin(state.wowPhase) * wowDepth;
            float flutterMod = std::sin(state.flutterPhase) * flutterDepth;
            float totalMod = (wowMod + flutterMod) * static_cast<float>(sampleRate_);

            float twoPi = 2.0f * static_cast<float>(M_PI);
            state.wowPhase += twoPi * wowFreq / static_cast<float>(sampleRate_);
            state.flutterPhase += twoPi * flutterFreq / static_cast<float>(sampleRate_);
            if (state.wowPhase > twoPi) state.wowPhase -= twoPi;
            if (state.flutterPhase > twoPi) state.flutterPhase -= twoPi;

            state.delayBuffer[state.delayWritePos] = saturated;

            float delaySamples = baseDelay + totalMod;
            delaySamples = std::clamp(delaySamples, 1.0f,
                static_cast<float>(ChannelState::kMaxDelaySamples - 2));

            int idx0 = static_cast<int>(delaySamples);
            float frac = delaySamples - static_cast<float>(idx0);

            int readPos0 = (state.delayWritePos - idx0 + ChannelState::kMaxDelaySamples)
                         % ChannelState::kMaxDelaySamples;
            int readPos1 = (readPos0 - 1 + ChannelState::kMaxDelaySamples)
                         % ChannelState::kMaxDelaySamples;

            float modulated = state.delayBuffer[readPos0] * (1.0f - frac)
                            + state.delayBuffer[readPos1] * frac;

            state.delayWritePos = (state.delayWritePos + 1) % ChannelState::kMaxDelaySamples;

            float filtered = modulated;
            if (state.headRolloff) {
                filtered = (*state.headRolloff)(modulated);
            }

            if (params_.hissLevel > 0.001f) {
                filtered += generateNoise() * params_.hissLevel * 0.003f;
            }

            float wet = filtered;
            if (state.dcBlock) {
                wet = (*state.dcBlock)(filtered);
            }

            float mixed = std::lerp(dryBuf_[i], wet, params_.dryWet);
            out[ch][i] = std::clamp(mixed * params_.outputGain, -1.0f, 1.0f);
        }
    }
}

float VocalMode::generateNoise() {
    noiseState_ ^= noiseState_ << 13;
    noiseState_ ^= noiseState_ >> 17;
    noiseState_ ^= noiseState_ << 5;
    return static_cast<float>(noiseState_) / static_cast<float>(0xFFFFFFFF) * 2.0f - 1.0f;
}

void VocalMode::reset() {
    for (auto& ch : channels_) {
        ch.hysteresis.reset();
        ch.headRolloff.reset();
        ch.dcBlock.reset();
        ch.oversampler.reset();
        ch.wowPhase = 0.0f;
        ch.flutterPhase = 0.0f;
        ch.delayBuffer.fill(0.0f);
        ch.delayWritePos = 0;
    }
    noiseState_ = 0x67452301;
}

} // namespace dsp
