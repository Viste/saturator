#ifndef VSRATURATOR_DEMO_AUDIOBUFFERS_H
#define VSRATURATOR_DEMO_AUDIOBUFFERS_H

#include "BufferArray.hpp"
#include <memory>
#include <algorithm>

class AudioBuffers
{
public:
    AudioBuffers();
    AudioBuffers(const AudioBuffers &other) noexcept;
    AudioBuffers &operator=(const AudioBuffers &other);
    ~AudioBuffers() = default;

    BufferArray *getInput(int channel) const;
    BufferArray *getDelta(int channel) const;
    BufferArray *getSidechain(int channel) const;

    void setBuffersSize(int size);

    static constexpr int MAX_CHANNEL = 2;

private:
    void release();
    void copy(const AudioBuffers &other);

    std::unique_ptr<BufferArray[]> inputBuffer[MAX_CHANNEL];
    std::unique_ptr<BufferArray[]> deltaBuffer[MAX_CHANNEL];
    std::unique_ptr<BufferArray[]> sidechainBuffer[MAX_CHANNEL];
};

#endif // VSRATURATOR_DEMO_AUDIOBUFFERS_H