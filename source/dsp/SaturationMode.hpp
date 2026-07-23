#pragma once

#include <cstdint>

namespace dsp {

class SaturationMode {
public:
    virtual ~SaturationMode() = default;

    virtual void prepare(double sampleRate, int maxBlockSize) = 0;
    virtual void process(float** in, float** out, int channels, int numSamples) = 0;
    virtual void reset() = 0;
    virtual int getLatencySamples() const { return 0; }
};

} // namespace dsp
