#include "AudioBuffers.hpp"

AudioBuffers::AudioBuffers() {}

AudioBuffers::AudioBuffers(const AudioBuffers &other) noexcept
{
    copy(other);
}

AudioBuffers &AudioBuffers::operator=(const AudioBuffers &other)
{
    if (this != &other) {
        release();
        copy(other);
    }
    return *this;
}

void AudioBuffers::setBuffersSize(int size)
{
    release();
    for (int i = 0; i < MAX_CHANNEL; ++i) {
        inputBuffer[i] = std::make_unique<BufferArray[]>(size);
        deltaBuffer[i] = std::make_unique<BufferArray[]>(size);
        sidechainBuffer[i] = std::make_unique<BufferArray[]>(size);
    }
}

BufferArray *AudioBuffers::getInput(int channel) const
{
    return inputBuffer[channel].get();
}

BufferArray *AudioBuffers::getDelta(int channel) const
{
    return deltaBuffer[channel].get();
}

BufferArray *AudioBuffers::getSidechain(int channel) const
{
    return sidechainBuffer[channel].get();
}

void AudioBuffers::release() {
    for (int i = 0; i < MAX_CHANNEL; ++i) {
        inputBuffer[i].reset();
        deltaBuffer[i].reset();
        sidechainBuffer[i].reset();
    }
}

void AudioBuffers::copy(const AudioBuffers &other)
{
    int size = other.inputBuffer[0][0].getSize();
    setBuffersSize(size);
    for (int i = 0; i < MAX_CHANNEL; ++i) {
        std::copy(other.inputBuffer[i].get(), other.inputBuffer[i].get() + size, inputBuffer[i].get());
        std::copy(other.deltaBuffer[i].get(), other.deltaBuffer[i].get() + size, deltaBuffer[i].get());
        std::copy(other.sidechainBuffer[i].get(), other.sidechainBuffer[i].get() + size, sidechainBuffer[i].get());
    }
}