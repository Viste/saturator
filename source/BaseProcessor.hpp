#ifndef SATURATOR_DEMO_BASEPROCESSOR_H
#define SATURATOR_DEMO_BASEPROCESSOR_H

#include "utils/AudioBuffers.hpp"
#include <public.sdk/source/vst/vstaudioeffect.h>
#include <q/fx/biquad.hpp>
#include <q/fx/edge.hpp>
#include <q/detail/fast_math.hpp>
#include <q/support/literals.hpp>
#include <q/utility/ring_buffer.hpp>
#include <memory>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

template<typename SampleType>
class BiquadFilter {
public:
    enum FilterType {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Peak,
        LowShelf,
        HighShelf
    };

    BiquadFilter() : a0(1), a1(0), a2(0), b0(1), b1(0), b2(0), z1(0), z2(0) {}

    void setFilterParams(FilterType type, double frequency, double sampleRate, double Q = 0.707, double gainDB = 0.0) {
        double omega = 2.0 * M_PI * frequency / sampleRate;
        double alpha = fastersin(omega) / (2.0 * Q);
        double gain = fasterpow(10, gainDB / 40);

        switch (type) {
            case HighPass:
                b0 =  (1 + fastercos(omega)) / 2;
                b1 = -(1 + fastercos(omega));
                b2 =  (1 + fastercos(omega)) / 2;
                a0 =   1 + alpha;
                a1 =  -2 * fastercos(omega);
                a2 =   1 - alpha;
                break;
            // Другие типы фильтров можно добавить по аналогии
            default:
                break;
        }

        // Нормализация коэффициентов
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    SampleType process(SampleType input) {
        double output = b0 * input + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
        z2 = z1;
        z1 = output;
        return static_cast<SampleType>(output);
    }

private:
    double a0, a1, a2, b0, b1, b2;
    double z1, z2;
};

class BaseProcessor : public AudioEffect
{
public:
    BaseProcessor();

    ~BaseProcessor() SMTG_OVERRIDE;

    static FUnknown *createInstance(void * /*context*/) { return (IAudioProcessor *) new BaseProcessor; }

    tresult PLUGIN_API initialize(FUnknown *context) SMTG_OVERRIDE;

    tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    tresult PLUGIN_API setActive(TBool state) SMTG_OVERRIDE;

    tresult PLUGIN_API setupProcessing(ProcessSetup &newSetup) SMTG_OVERRIDE;

    tresult setBusArrangements(SpeakerArrangement *inputs, int32 numIns, SpeakerArrangement *outputs, int32 numOuts) override;

    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize)
    SMTG_OVERRIDE;

    tresult PLUGIN_API process(ProcessData &data) SMTG_OVERRIDE;

    tresult PLUGIN_API setState(IBStream *state) SMTG_OVERRIDE;

    tresult PLUGIN_API getState(IBStream *state) SMTG_OVERRIDE;

    uint32 getLatencySamples() override;

    template<typename SampleType>
    SampleType chebyshevHarmonics(SampleType inputSample, int order);

    tresult PLUGIN_API notify(IMessage *message) SMTG_OVERRIDE;

private:
    static constexpr int maxSampleRate = 44100;  // maximum sample rate supported
    static constexpr double latencySeconds = 0.004;  // Example: maximum expected latency in seconds
    int maxBufferSize = static_cast<int>(maxSampleRate * latencySeconds);
    cycfi::q::ring_buffer<double> ringBuffer;  // Use float or double as needed
    BiquadFilter<float> highPassFilter;

    template<typename SampleType>
    void fastProcessAudio(SampleType **in,SampleType **out, int32 numChannels,int32 sampleFrames);

    template<typename SampleType>
    void processAudioWithSaturation(SampleType **in, SampleType **out, int32 numChannels, int32 sampleFrames);

    template<typename SampleType>
    void processVocalWithSaturation(SampleType **in, SampleType **out, int32 numChannels, int32 sampleFrames);

    template<typename T, typename U>
    T lerp(T v0, T v1, U t);

    template<typename T>
    T clamp(T val, T minVal, T maxVal);

    template<typename T, typename U>
    T softTanh(T x, U softness);

    tresult updateOscData(const void *data, size_t size);

    bool restartMessage();
    static float randFloat();

    double calculatedRatio = 0.0;
    void recalculateBlendRatio(double ratio);
    template<typename SampleType>
    SampleType blend(SampleType signal1, SampleType signal2);

    float saturation;
    float gain;
    bool bypass;
    int algorithmMode;
    int algorithmBeforeState;
    bool phaseInverse;
    int latency = 0;

    
    AudioBuffers *buffers = nullptr;
    template<typename SampleType>
    SampleType cosCos(SampleType val, double coef);

    static int const BASS_MODE = 0;
    static int const BEAT_MODE = 1;
    static int const VOCAL_MODE = 2;

};
#endif // SATURATOR_DEMO_BASEPROCESSOR_H