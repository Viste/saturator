#include "VocalMode.hpp"
#define _USE_MATH_DEFINES
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
    if (numSamples <= 0) return;

    float mix = params_.dryWet;

    float speedFactor = 0.5f + params_.tapeSpeed * 1.0f;

    float wowFreq = 1.5f * speedFactor;
    float flutterFreq = 7.0f * speedFactor;
    float wowDepth = params_.wowAmount * 0.015f * mix / speedFactor;
    float flutterDepth = params_.flutterAmount * 0.004f * mix / speedFactor;
    float baseDelay = 8.0f;

    float satDrive = params_.saturation * mix;
    float feedback = (0.1f + params_.tapeBias * 0.4f) * mix;
    if (!rampInit_) {
        rampDrive_ = satDrive;
        rampFb_ = feedback;
        rampInit_ = true;
    }

    for (int ch = 0; ch < chCount; ++ch) {
        auto& state = channels_[ch];

        float effectiveSr = static_cast<float>(sampleRate_) * static_cast<float>(params_.osFactor);
        state.hysteresis.setParams(satDrive, feedback, effectiveSr);
        state.oversampler.setFactor(params_.osFactor);

        if (state.headRolloff) {
            float effectiveCutoff = params_.headCutoff * speedFactor;
            state.headRolloff->config(cycfi::q::frequency(effectiveCutoff),
                                       static_cast<float>(sampleRate_));
        }

        for (int i = 0; i < numSamples; ++i) {
            dryBuf_[i] = in[ch][i] * params_.inputGain;
        }

        {
            float osN = static_cast<float>(numSamples * static_cast<int>(params_.osFactor));
            float d = rampDrive_;
            float dStep = (satDrive - d) / osN;
            float fb = rampFb_;
            float fbStep = (feedback - fb) / osN;
            state.oversampler.process(dryBuf_.data(), out[ch], numSamples,
                [&state, d, dStep, fb, fbStep](float s) mutable {
                    d += dStep;
                    fb += fbStep;
                    state.hysteresis.setDriveFeedback(d, fb);
                    return state.hysteresis.process(s);
                });
        }

        const float sr = static_cast<float>(sampleRate_);
        const float twoPi = 2.0f * static_cast<float>(M_PI);
        const float wowInc = twoPi * wowFreq / sr;
        const float flInc = twoPi * flutterFreq / sr;
        float wowSin = std::sin(state.wowPhase), wowCos = std::cos(state.wowPhase);
        float flSin = std::sin(state.flutterPhase), flCos = std::cos(state.flutterPhase);
        const float wowSinInc = std::sin(wowInc), wowCosInc = std::cos(wowInc);
        const float flSinInc = std::sin(flInc), flCosInc = std::cos(flInc);

        for (int i = 0; i < numSamples; ++i) {
            float saturated = out[ch][i];

            float totalMod = (wowSin * wowDepth + flSin * flutterDepth) * sr;

            float wt = wowSin * wowCosInc + wowCos * wowSinInc;
            wowCos = wowCos * wowCosInc - wowSin * wowSinInc;
            wowSin = wt;
            float ft = flSin * flCosInc + flCos * flSinInc;
            flCos = flCos * flCosInc - flSin * flSinInc;
            flSin = ft;

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
                filtered += generateNoise() * params_.hissLevel * 0.05f * mix;
            }

            float wet = filtered;
            if (state.dcBlock) {
                wet = (*state.dcBlock)(filtered);
            }

            out[ch][i] = std::clamp(wet * params_.outputGain, -1.0f, 1.0f);
        }

        state.wowPhase = std::fmod(state.wowPhase + wowInc * numSamples, twoPi);
        state.flutterPhase = std::fmod(state.flutterPhase + flInc * numSamples, twoPi);
    }

    rampDrive_ = satDrive;
    rampFb_ = feedback;
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
    rampInit_ = false;
}

} // namespace dsp
