#pragma once

#include <array>
#include <vector>
#include <cstring>

namespace dsp {

struct HalfBandKernel {
    static constexpr int kTaps = 33;
    static constexpr int kCenter = 16;

    static constexpr std::array<float, kTaps> coeffs = {
         0.0f, -0.00007f,  0.0f,  0.00085f,  0.0f, -0.00323f,  0.0f,  0.00879f,
         0.0f, -0.02017f,  0.0f,  0.04247f,  0.0f, -0.09189f,  0.0f,  0.31331f,
         0.50000f,
         0.31331f,  0.0f, -0.09189f,  0.0f,  0.04247f,  0.0f, -0.02017f,
         0.0f,  0.00879f,  0.0f, -0.00323f,  0.0f,  0.00085f,  0.0f, -0.00007f,
         0.0f
    };
};

class Oversampler {
public:
    enum Factor { kNone = 1, k2x = 2, k4x = 4 };

    void prepare(int maxBlockSize) {
        buf2x_.resize(static_cast<size_t>(maxBlockSize) * 2, 0.0f);
        buf4x_.resize(static_cast<size_t>(maxBlockSize) * 4, 0.0f);
        reset();
    }

    void setFactor(Factor f) {
        if (f != factor_) {
            factor_ = f;
            reset(); // очистка FIR delay lines при смене фактора
        }
    }
    Factor getFactor() const { return factor_; }

    template<typename Fn>
    void process(const float* input, float* output, int numSamples, Fn&& fn) {
        if (factor_ == kNone) {
            for (int i = 0; i < numSamples; ++i)
                output[i] = fn(input[i]);
            return;
        }

        if (factor_ == k2x) {
            process2x(input, output, numSamples, std::forward<Fn>(fn));
            return;
        }

        process4x(input, output, numSamples, std::forward<Fn>(fn));
    }

    void reset() {
        for (auto& dl : upDelay_) dl.fill(0.0f);
        for (auto& dl : downDelay_) dl.fill(0.0f);
    }

private:
    using DelayLine = std::array<float, HalfBandKernel::kTaps>;

    static void applyFIR(float* data, int length, DelayLine& dl) {
        constexpr auto& k = HalfBandKernel::coeffs;
        constexpr int C = HalfBandKernel::kCenter;

        for (int i = 0; i < length; ++i) {
            for (int j = HalfBandKernel::kTaps - 1; j > 0; --j)
                dl[j] = dl[j - 1];
            dl[0] = data[i];

            float sum = dl[C] * k[C];
            for (int j = 1; j <= C; j += 2) {
                sum += (dl[C - j] + dl[C + j]) * k[C - j];
            }
            data[i] = sum;
        }
    }

    template<typename Fn>
    void process2x(const float* input, float* output, int numSamples, Fn&& fn) {
        int n2 = numSamples * 2;

        for (int i = 0; i < numSamples; ++i) {
            buf2x_[static_cast<size_t>(i) * 2]     = input[i] * 2.0f;
            buf2x_[static_cast<size_t>(i) * 2 + 1] = 0.0f;
        }

        applyFIR(buf2x_.data(), n2, upDelay_[0]);

        for (int i = 0; i < n2; ++i)
            buf2x_[i] = fn(buf2x_[i]);

        applyFIR(buf2x_.data(), n2, downDelay_[0]);

        for (int i = 0; i < numSamples; ++i)
            output[i] = buf2x_[static_cast<size_t>(i) * 2];
    }

    template<typename Fn>
    void process4x(const float* input, float* output, int numSamples, Fn&& fn) {
        int n2 = numSamples * 2;
        int n4 = numSamples * 4;

        for (int i = 0; i < numSamples; ++i) {
            buf2x_[static_cast<size_t>(i) * 2]     = input[i] * 2.0f;
            buf2x_[static_cast<size_t>(i) * 2 + 1] = 0.0f;
        }
        applyFIR(buf2x_.data(), n2, upDelay_[0]);

        for (int i = 0; i < n2; ++i) {
            buf4x_[static_cast<size_t>(i) * 2]     = buf2x_[i] * 2.0f;
            buf4x_[static_cast<size_t>(i) * 2 + 1] = 0.0f;
        }
        applyFIR(buf4x_.data(), n4, upDelay_[1]);

        for (int i = 0; i < n4; ++i)
            buf4x_[i] = fn(buf4x_[i]);

        applyFIR(buf4x_.data(), n4, downDelay_[1]);
        for (int i = 0; i < n2; ++i)
            buf2x_[i] = buf4x_[static_cast<size_t>(i) * 2];

        applyFIR(buf2x_.data(), n2, downDelay_[0]);
        for (int i = 0; i < numSamples; ++i)
            output[i] = buf2x_[static_cast<size_t>(i) * 2];
    }

    Factor factor_ = kNone;
    std::vector<float> buf2x_;
    std::vector<float> buf4x_;
    std::array<DelayLine, 2> upDelay_{};
    std::array<DelayLine, 2> downDelay_{};
};

} // namespace dsp
