#pragma once

#include <cmath>
#include <algorithm>

namespace dsp {

// мягкая сатурация tanh, нормализация по пику
inline float softSaturate(float x, float drive) {
    if (drive < 0.001f) return x;
    float d = 1.0f + drive * 4.0f;
    float tanhD = std::tanh(d);
    return std::tanh(d * x) / tanhD;
}

// ламповая сатурация: tanh + чётная гармоника через T2
inline float tubeSaturate(float x, float drive) {
    if (drive < 0.001f) return x;
    float d = 1.0f + drive * 4.0f;
    float tanhD = std::tanh(d);

    float odd = std::tanh(d * x) / tanhD;

    float xc = std::clamp(odd, -1.0f, 1.0f);
    float t2 = 2.0f * xc * xc - 1.0f;

    float evenAmount = drive * 0.15f;
    return odd + t2 * evenAmount;
}

// лента: atan, мягкое колено
inline float tapeSaturate(float x, float drive) {
    if (drive < 0.001f) return x;
    float d = 1.0f + drive * 5.0f;
    float atanD = std::atan(d);
    return std::atan(d * x) / atanD;
}

// мягкий клиппер: tanh-колено после порога
inline float softClip(float x, float amount) {
    if (amount < 0.001f) return x;
    // пологая кривая: порог опускается медленно, колено мягкое
    float threshold = 1.0f - amount * 0.35f;   // 1.0 → 0.65
    float absX = std::abs(x);
    if (absX <= threshold) return x;
    float knee = 0.5f + amount * 1.5f;         // 0.5 → 2.0
    float excess = absX - threshold;
    float headroom = 1.0f - threshold;
    float compressed = threshold + headroom * std::tanh(knee * excess / headroom);
    return (x >= 0.0f) ? compressed : -compressed;
}

// обогащение гармониками через полиномы чебышёва
inline float harmonicEnrich(float x, float amount, int maxOrder = 5) {
    if (amount < 0.001f || maxOrder < 2) return x;

    float xc = std::clamp(x, -1.0f, 1.0f);
    float x2 = xc * xc;

    float t2 = 2.0f * x2 - 1.0f;
    float t3 = xc * (4.0f * x2 - 3.0f);
    float harmonics = t2 * 0.20f + t3 * 0.15f;

    if (maxOrder >= 4) {
        float t4 = 8.0f * x2 * x2 - 8.0f * x2 + 1.0f;
        harmonics += t4 * 0.08f;
    }
    if (maxOrder >= 5) {
        float t5 = xc * (16.0f * x2 * x2 - 20.0f * x2 + 5.0f);
        harmonics += t5 * 0.04f;
    }

    return x + harmonics * amount;
}

// эмуляция магнитного гистерезиса ленты
class TapeHysteresis {
public:
    void setParams(float drive, float feedback, float sampleRate = 44100.0f) {
        drive_ = drive;
        feedback_ = std::clamp(feedback, 0.0f, 0.35f);
        float fc = 3000.0f;
        float w = 2.0f * 3.14159265f * fc / sampleRate;
        smoothCoeff_ = w / (1.0f + w);
    }

    float process(float x) {
        if (drive_ < 0.001f && feedback_ < 0.001f) return x;

        float input = x + feedback_ * state_;
        input = std::clamp(input, -2.0f, 2.0f);

        float d = 1.0f + drive_ * 3.0f;
        float tanhD = std::tanh(d);
        float shaped = std::tanh(d * input) / tanhD;

        state_ += smoothCoeff_ * (shaped - state_);
        return shaped;
    }

    void reset() { state_ = 0.0f; }

private:
    float state_ = 0.0f;
    float drive_ = 0.5f;
    float feedback_ = 0.2f;
    float smoothCoeff_ = 0.3f;
};

} // namespace dsp
