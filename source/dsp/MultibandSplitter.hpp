#pragma once

#include <q/fx/biquad.hpp>
#include <q/support/frequency.hpp>
#include <optional>
#include <cmath>

namespace dsp {

// 3-полосный кроссовер linkwitz-riley 4-го порядка
class MultibandSplitter {
public:
    struct Bands {
        float low = 0.0f;
        float mid = 0.0f;
        float high = 0.0f;
    };

    void prepare(float sampleRate) {
        sampleRate_ = sampleRate;
        setCrossoverFrequencies(200.0f, 2500.0f);
    }

    void setCrossoverFrequencies(float lowMidHz, float midHighHz) {
        using namespace cycfi::q;

        lp1a_.emplace(frequency(lowMidHz), sampleRate_, 0.707);
        lp1b_.emplace(frequency(lowMidHz), sampleRate_, 0.707);
        hp1a_.emplace(frequency(lowMidHz), sampleRate_, 0.707);
        hp1b_.emplace(frequency(lowMidHz), sampleRate_, 0.707);

        lp2a_.emplace(frequency(midHighHz), sampleRate_, 0.707);
        lp2b_.emplace(frequency(midHighHz), sampleRate_, 0.707);
        hp2a_.emplace(frequency(midHighHz), sampleRate_, 0.707);
        hp2b_.emplace(frequency(midHighHz), sampleRate_, 0.707);

        lastLowMid_ = lowMidHz;
        lastMidHigh_ = midHighHz;
    }

    void reconfigure(float lowMidHz, float midHighHz) {
        using namespace cycfi::q;

        if (!lp1a_.has_value()) {
            setCrossoverFrequencies(lowMidHz, midHighHz);
            return;
        }

        if (std::abs(lowMidHz - lastLowMid_) < 0.1f &&
            std::abs(midHighHz - lastMidHigh_) < 0.1f)
            return;

        lp1a_->config(frequency(lowMidHz), sampleRate_);
        lp1b_->config(frequency(lowMidHz), sampleRate_);
        hp1a_->config(frequency(lowMidHz), sampleRate_);
        hp1b_->config(frequency(lowMidHz), sampleRate_);

        lp2a_->config(frequency(midHighHz), sampleRate_);
        lp2b_->config(frequency(midHighHz), sampleRate_);
        hp2a_->config(frequency(midHighHz), sampleRate_);
        hp2b_->config(frequency(midHighHz), sampleRate_);

        lastLowMid_ = lowMidHz;
        lastMidHigh_ = midHighHz;
    }

    Bands split(float input) {
        Bands bands;

        float lp1 = (*lp1a_)(input);
        bands.low = (*lp1b_)(lp1);

        float hp1 = (*hp1a_)(input);
        float highFromFirst = (*hp1b_)(hp1);

        float lp2 = (*lp2a_)(highFromFirst);
        bands.mid = (*lp2b_)(lp2);

        float hp2 = (*hp2a_)(highFromFirst);
        bands.high = (*hp2b_)(hp2);

        return bands;
    }

    static float recombine(const Bands& bands) {
        return bands.low + bands.mid + bands.high;
    }

    void reset() {
        lp1a_.reset(); lp1b_.reset();
        hp1a_.reset(); hp1b_.reset();
        lp2a_.reset(); lp2b_.reset();
        hp2a_.reset(); hp2b_.reset();
    }

private:
    float sampleRate_ = 44100.0f;
    float lastLowMid_ = 0.0f;
    float lastMidHigh_ = 0.0f;

    // lr4 нч-сч кроссовер
    std::optional<cycfi::q::lowpass> lp1a_, lp1b_;
    std::optional<cycfi::q::highpass> hp1a_, hp1b_;

    // lr4 сч-вч кроссовер
    std::optional<cycfi::q::lowpass> lp2a_, lp2b_;
    std::optional<cycfi::q::highpass> hp2a_, hp2b_;
};

} // namespace dsp
