#pragma once

#include <atomic>
#include <array>
#include <algorithm>
#include <cmath>

namespace dsp {

// lock-free метеринг; samples[] не atomic — допустимый data race
struct MeteringData {
    static constexpr int kWaveformSize = 2048;

    std::atomic<float> inputPeak{0.0f};
    std::atomic<float> outputPeak{0.0f};
    std::atomic<float> sampleRate{44100.0f};

    struct WaveformBuffer {
        std::array<float, kWaveformSize> samples{};
        std::atomic<int> writePos{0};
    };

    WaveformBuffer inputWaveform;
    WaveformBuffer outputWaveform;

    void pushInputSample(float l, float r) {
        float peak = std::max(std::abs(l), std::abs(r));
        float current = inputPeak.load(std::memory_order_relaxed);
        if (peak > current) {
            inputPeak.store(peak, std::memory_order_relaxed);
        }

        int pos = inputWaveform.writePos.load(std::memory_order_relaxed);
        inputWaveform.samples[pos] = (l + r) * 0.5f;
        inputWaveform.writePos.store((pos + 1) % kWaveformSize, std::memory_order_relaxed);
    }

    void pushOutputSample(float l, float r) {
        float peak = std::max(std::abs(l), std::abs(r));
        float current = outputPeak.load(std::memory_order_relaxed);
        if (peak > current) {
            outputPeak.store(peak, std::memory_order_relaxed);
        }

        int pos = outputWaveform.writePos.load(std::memory_order_relaxed);
        outputWaveform.samples[pos] = (l + r) * 0.5f;
        outputWaveform.writePos.store((pos + 1) % kWaveformSize, std::memory_order_relaxed);
    }

    void decayPeaks(float decayFactor = 0.95f) {
        auto decay = [&](std::atomic<float>& peak) {
            float val = peak.load(std::memory_order_relaxed);
            peak.store(val * decayFactor, std::memory_order_relaxed);
        };
        decay(inputPeak);
        decay(outputPeak);
    }
};

} // namespace dsp
