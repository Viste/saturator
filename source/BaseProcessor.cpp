#include "BaseProcessor.hpp"
#include "MessagesConsts.hpp"
#include "PluginIds.hpp"
#include "ctime"
#include <base/source/fstreamer.h>
#include <public.sdk/source/vst/vstaudioprocessoralgo.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>


BaseProcessor::BaseProcessor() : ringBuffer(maxBufferSize), saturation(0.0f), gain(1.0f), bypass(false), algorithmMode(false), phaseInverse(false)
{
    setControllerClass(FUID::fromTUID(kSaturatorControllerUID));
    processContextRequirements.needTempo();
    processContextRequirements.needTransportState();
    processContextRequirements.needSystemTime();
}

BaseProcessor::~BaseProcessor() {}

tresult PLUGIN_API BaseProcessor::initialize(FUnknown *context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo, BusTypes::kMain);
    addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo, BusTypes::kMain);

    buffers = new AudioBuffers();
    return kResultOk;
}


tresult PLUGIN_API BaseProcessor::terminate()
{
    if (buffers) delete buffers;
    return AudioEffect::terminate();
}


tresult PLUGIN_API BaseProcessor::setActive(TBool state)
{

    SpeakerArrangement arr;
    if (getBusArrangement(kOutput, 0, arr) != kResultTrue)
    {
        return kResultFalse;
    }
    int32 numChannels = SpeakerArr::getChannelCount(arr);
    if (numChannels < 1 || numChannels > 2)
    {
        return kResultFalse;
    }
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API BaseProcessor::process(Vst::ProcessData &data)
{
    if (data.inputParameterChanges) {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 index = 0; index < numParamsChanged; index++)
        {
            if (auto *paramQueue = data.inputParameterChanges->getParameterData(index))
            {
                Vst::ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();
                switch (paramQueue->getParameterId())
                {
                    case kSaturation:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                        {
                            saturation = static_cast<float>(value);
                        }
                        break;
                    case kBypass:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                        {
                            bypass = (value > 0.5f);
                        }
                        break;
                    case kDeath:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                        {
                            saturation *= (value > 0.5f) ? 10.0f : 1.0f;
                        }
                    case kSwitch:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                        {
                            algorithmMode = static_cast<int>(value * 2.0);
                            if (algorithmMode < 0) algorithmMode = 0;
                            if (algorithmMode > 2) algorithmMode = 2;
                        }
                        break;
                    case kGain:
                        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
                        {
                            gain = static_cast<float>(value);
                        }
                        break;
                }
            }
        }
    }

    if (data.numInputs != 1 || data.numOutputs != 1)
    {
        return kResultOk;
    }

    if (algorithmBeforeState != algorithmMode)
    {
        algorithmBeforeState = algorithmMode;
        return restartMessage();
    }

    int32 numChannels = data.inputs[0].numChannels;
    uint32 sampleFramesSize = getSampleFramesSizeInBytes(processSetup, data.numSamples);

    void **in = getChannelBuffersPointer(processSetup, data.inputs[0]);
    void **out = getChannelBuffersPointer(processSetup, *data.outputs);

    if (bypass)
    {
        for (int32 i = 0; i < numChannels; i++)
        {
            if (in[i] != out[i]) {
                memcpy(out[i], in[i], sampleFramesSize);
            }
        }
        return kResultOk;
    }

    if (data.inputs[0].silenceFlags != 0)
    {
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
        for (int32 i = 0; i < numChannels; i++)
        {
            if (in[i] != out[i]) {
                memset(out[i], 0, sampleFramesSize);
            }
        }
        return kResultOk;
    }

    data.outputs[0].silenceFlags = 0;

    if (data.processContext) {
        if (data.processContext->state & ProcessContext::kTempoValid) {
            int normalizedPow = static_cast<int>(floor(saturation * 5.f));
            normalizedPow = (normalizedPow > 4) ? 4 : normalizedPow;
            int oscBars = pow(2, normalizedPow);
        }
    }

    recalculateBlendRatio(saturation);
    switch (algorithmMode) {
        case BASS_MODE:
            if (data.symbolicSampleSize == kSample32)
            {
                processAudioWithSaturation<Sample32>(data.inputs[0].channelBuffers32, data.outputs[0].channelBuffers32, data.inputs[0].numChannels, data.numSamples);
            }
            else
            {
                processAudioWithSaturation<Sample64>(reinterpret_cast<Sample64 **>(data.inputs[0].channelBuffers32), reinterpret_cast<Sample64 **>(data.outputs[0].channelBuffers32), data.inputs[0].numChannels, data.numSamples);
            }
            break;
        case BEAT_MODE:
        {
            if (data.symbolicSampleSize == kSample32)
            {
                fastProcessAudio<Sample32>((Sample32 **) in, (Sample32 **) out, numChannels, data.numSamples);
            }
            else
            {
                fastProcessAudio<Sample64>((Sample64 **) in, (Sample64 **) out, numChannels, data.numSamples);
            }
            break;
        }
        case VOCAL_MODE:
            if (data.symbolicSampleSize == kSample32)
            {
                processVocalWithSaturation<Sample32>(data.inputs[0].channelBuffers32, data.outputs[0].channelBuffers32, data.inputs[0].numChannels, data.numSamples);
            }
            else
            {
                processVocalWithSaturation<Sample64>(reinterpret_cast<Sample64 **>(data.inputs[0].channelBuffers32), reinterpret_cast<Sample64 **>(data.outputs[0].channelBuffers32), data.inputs[0].numChannels, data.numSamples);
            }
            break;
    }

    return kResultOk;
}

template<typename T>
T BaseProcessor::clamp(T val, T minVal, T maxVal) {
    return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
}

template<typename T, typename U>
T BaseProcessor::lerp(T v0, T v1, U t)
{
    return (1 - t) * v0 + t * v1;
}

template<typename T, typename U>
T BaseProcessor::softTanh(T x, U softness)
{
    return std::tanh(x * softness);
}

float softClip(float inputSample, float drive, float mix, float threshold) {
    // Применяем усиление
    inputSample *= drive;

    // Применяем софт-клиппинг
    float clippedSample;
    if (inputSample < -threshold) {
        clippedSample = -2.0f / 3.0f;
    } else if (inputSample > threshold) {
        clippedSample = 2.0f / 3.0f;
    } else {
        // Используем арктангенс для более плавного искажения
        clippedSample = (2.0f / M_PI) * atan(inputSample);
        
    }

    // Смешиваем обработанный и исходный сигналы
    float outputSample = mix * clippedSample + (1.0f - mix) * inputSample;

    return outputSample;
}

template<typename SampleType>
SampleType BaseProcessor::chebyshevHarmonics(SampleType inputSample, int order)
{
    SampleType outputSample = 0;
    SampleType Tn_1 = inputSample; // T(n-1) для n=1
    SampleType Tn = inputSample; // T(n) для n=1
    SampleType Tn_2 = 0; // T(n-2) для n=2

    // Генерация полиномов Чебышева и суммирование их вклада
    for (int n = 1; n <= order; ++n) {
        if (n > 1) {
            Tn = 2 * inputSample * Tn_1 - Tn_2; // Рекурсивное вычисление T(n)
        }
        outputSample += Tn; // Добавляем вклад от текущего полинома

        // Подготовка к следующей итерации
        Tn_2 = Tn_1;
        Tn_1 = Tn;
    }

    // Нормализация
    outputSample = outputSample / order;

    return outputSample;
}

template<typename SampleType>
void BaseProcessor::processVocalWithSaturation(SampleType **in, SampleType **out, int32 numChannels, int32 sampleFrames)
{
    auto sysTime = std::time(nullptr);
    float gainAmplification = fasterpow(10.0f, (gain * 10.0f) / 20.0f);

    for (int32 channel = 0; channel < numChannels; channel++)
    {
        for (int frame = 0; frame < sampleFrames; frame++)
        {
            SampleType inputSample = in[channel][frame] * gainAmplification;
            SampleType outputSample;

            // Ограничиваем входной сигнал диапазоном [-1, 1]
            inputSample = std::fmax(-1.0f, std::fmin(1.0f, inputSample));

            // Применяем модифицированную tanh сатурацию
            SampleType tanhSample = softTanh(inputSample * (saturation * 0.3f), static_cast<SampleType>(0.3)); // Мягкость tanh

            // Применяем софтклипперную сатурацию
            SampleType softClipSample = (inputSample < -1.0f) ? -2.0f / 3.0f : ((inputSample > 1.0f) ? 2.0f / 3.0f : inputSample - (inputSample * inputSample * inputSample / 3.0f));

            // Применяем сатурацию с использованием полиномов Чебышева
            SampleType chebyshevSample = chebyshevHarmonics(inputSample, 8);

            // Комбинируем все вместе
            outputSample = lerp(inputSample, saturation * 2.0f * (tanhSample * 5.0f + chebyshevSample + softClipSample), static_cast<SampleType>(saturation));

            // Нормализация результата
            outputSample = std::fmax(-1.0f, std::fmin(1.0f, outputSample));

            out[channel][frame] = outputSample;
        }
    }
}


template<typename SampleType>
void BaseProcessor::processAudioWithSaturation(SampleType **in, SampleType **out, int32 numChannels, int32 sampleFrames)
{
    auto sysTime = std::time(nullptr);
    float gainAmplification = fasterpow(10.0f, (gain * 10.0f) / 20.0f);
    float drive = 1.0f;       // Уровень насыщенности
    float mix = 0.8f;         // Процент обработанного сигнала
    float threshold = 1.0f;   // Порог клиппинг

    for (int32 channel = 0; channel < numChannels; channel++)
    {
        for (int frame = 0; frame < sampleFrames; frame++)
        {
            SampleType inputSample = in[channel][frame] * gainAmplification;
            SampleType outputSample;

            // Ограничиваем входной сигнал диапазоном [-1, 1]
            inputSample = std::fmax(-1.0f, std::fmin(1.0f, inputSample));

            // Применяем tanh сатурацию
            SampleType tanhSample = softTanh(inputSample * saturation, static_cast<SampleType>(0.5)); // Мягкость tanh

            // Применяем софтклипперную сатурацию
            SampleType softClipSample = softClip(inputSample, drive, mix, threshold);

            // Применяем сатурацию с использованием полиномов Чебышева
            SampleType chebyshevSample = chebyshevHarmonics(inputSample, 12);

            // Комбинируем все вместе
            SampleType saturatedSample = lerp(inputSample, saturation * 4.0f * (tanhSample * 15.0f + chebyshevSample + softClipSample), static_cast<SampleType>(saturation));

            // Нормализация
            outputSample = std::fmax(-1.0f, std::fmin(1.0f, saturatedSample));

            out[channel][frame] = outputSample;
        }
    }
}


template<typename SampleType>
void BaseProcessor::fastProcessAudio(SampleType **in, SampleType **out, int32 numChannels, int32 sampleFrames)
{
    SampleType inputSample, tanhSample, softClipSample, saturatedSample, delayedSample, outputSample;
    cycfi::q::rising_edge edge_detector;

    float gainAmplification = fasterpow(10.0f, (gain * 10.0f) / 20.0f);
    int currentLatencySamples = static_cast<int>(processSetup.sampleRate * latencySeconds);
    auto sysTime = std::time(nullptr);
    float drive = 1.0f;       // Уровень насыщенности
    float mix = 0.8f;         // Процент обработанного сигнала
    float threshold = 1.0f;   // Порог клиппинга

    for (int32 channel = 0; channel < numChannels; channel++)
    {
        ringBuffer.clear();
        SampleType last_sample = 0;
        for (int frame = 0; frame < sampleFrames; frame++)
        {
            inputSample = in[channel][frame] * gainAmplification;

            ringBuffer.push(inputSample);
            inputSample = clamp<float>(inputSample, -1.0f, 1.0f);

            bool is_transient = edge_detector(inputSample > last_sample);

            SampleType chebyshevSample = chebyshevHarmonics(inputSample, 12);

            tanhSample = softTanh(inputSample * saturation, static_cast<SampleType>(0.5));

            softClipSample = softClip(inputSample, drive, mix, threshold);
            
            saturatedSample = lerp(inputSample, saturation * 2.0f * (tanhSample * 13.0f + chebyshevSample + softClipSample), static_cast<SampleType>(saturation));

            if (ringBuffer.size() > currentLatencySamples)
                delayedSample = ringBuffer.back();

            SampleType filtredSample = highPassFilter.process(delayedSample);

            outputSample = saturatedSample + (is_transient ? (delayedSample - saturatedSample) * (saturation / 2) : 0);

            outputSample = clamp<float>(outputSample, -1.0f, 1.0f);

            out[channel][frame] = outputSample;
            last_sample = delayedSample;
        }
    }
}

/*
template<typename SampleType>
void BaseProcessor::fastProcessAudio(SampleType **in, SampleType **out, int32 numChannels, int32 sampleFrames, bool noise)
{
    SampleType transientEffect;
    SampleType inputSample;
    SampleType tanhSample;
    SampleType chebyshevSample;
    SampleType softClipSample;
    SampleType saturatedSample;
    SampleType delayedSample;
    SampleType outputSample;
    SampleType filtredSample;
    cycfi::q::rising_edge edge_detector;

    float gainAmplification = fasterpow(10.0f, (gain * 10.0f) / 20.0f);
    int currentLatencySamples = static_cast<int>(processSetup.sampleRate * latencySeconds);  // Calculate current latency in samples

    for (int32 channel = 0; channel < numChannels; channel++)
    {
        ringBuffer.clear();
        SampleType last_sample = 0;
        for (int frame = 0; frame < sampleFrames; frame++)
        {
            inputSample = in[channel][frame] * gainAmplification;

            // Копируем входной сигнал в ринг-буфер
            ringBuffer.push(inputSample);

            // Ограничиваем входной сигнал диапазоном [-1, 1]
            inputSample = std::fmax(-1.0f, std::fmin(1.0f, inputSample));

            // Детекция транзиентов
            bool is_transient = edge_detector(inputSample > last_sample);

            SampleType chebyshevSample = chebyshevHarmonics(inputSample, 12);

            // Применяем модифицированную tanh сатурацию
            tanhSample = softTanh(chebyshevSample * saturation, static_cast<SampleType>(0.5)); // Мягкость tanh

            // Применяем софтклипперную сатурацию
            softClipSample = (tanhSample < -1.0f) ? -2.0f / 3.0f : ((tanhSample > 1.0f) ? 2.0f / 3.0f : tanhSample - (tanhSample * tanhSample * tanhSample / 3.0f));

            // Комбинируем все вместе
            //saturatedSample = lerp(inputSample, saturation * 2.0f * (tanhSample * 13.0f + softClipSample), static_cast<SampleType>(saturation));

            // Нормализация результата
            saturatedSample = std::fmax(-1.0f, std::fmin(1.0f, saturatedSample));

            if (ringBuffer.size() > currentLatencySamples)
                //delayedSample = ringBuffer[ringBuffer.size() - 1];
                delayedSample = ringBuffer.back();

            // Получаем задержанный сигнал из ринг-буфера с линейной интерполяцией
            //delayedSample = ringBuffer.back();

            //delayedSample = std::fmax(-1.0f, std::fmin(1.0f, delayedSample));

            filtredSample = highPassFilter.process(delayedSample);

            outputSample = saturatedSample + (is_transient ? (filtredSample - saturatedSample) * saturation/2 : 0);

            outputSample = std::fmax(-1.0f, std::fmin(1.0f, outputSample));

            // Запись результата
            out[channel][frame] = outputSample;

            last_sample = delayedSample;

            if (channel == 0 && oscProcessor)
            {
                licenseCounterTime++;
                oscProcessor->process(abs(inputSample), outputSample, filtredSample, saturatedSample, currentTime++);
            }
        }
    }
}
*/

tresult PLUGIN_API BaseProcessor::setupProcessing(ProcessSetup &newSetup) {

    latency = static_cast<int>(newSetup.sampleRate * 0.002); // 0.002 = 2 ms / 1000 ms
    if (buffers) {
        buffers->setBuffersSize(newSetup.maxSamplesPerBlock + latency);
    }

    newSetup.processMode = ProcessModes::kRealtime;
    processSetup = newSetup;
    return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API
BaseProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    if (symbolicSampleSize == Vst::kSample32) {
        return kResultTrue;
    }
    if (symbolicSampleSize == Vst::kSample64) {
        return kResultTrue;
    }

    return kResultFalse;
}

tresult PLUGIN_API BaseProcessor::setState(IBStream *state) {
    if (!state) {
        return kResultFalse;
    }

    IBStreamer streamer(state, kLittleEndian);

    bool savedBypass = false;
    if (!streamer.readBool(savedBypass))
        return kResultFalse;
    bypass = savedBypass;

    float savedSaturation = 0.0f;
    if (!streamer.readFloat(savedSaturation))
        return kResultFalse;
    saturation = savedSaturation;

    int savedAlgorithm = 2;
    if (!streamer.readInt32(savedAlgorithm))
        return kResultFalse;
    algorithmMode = savedAlgorithm;

    float savedGain = 0.5f;
    if (!streamer.readFloat(savedGain))
        return kResultFalse;
    gain = savedGain;

    bool savedPhase = false;
    if (!streamer.readBool(savedPhase))
        return kResultFalse;
    phaseInverse = savedPhase;

    return kResultOk;
}

tresult PLUGIN_API BaseProcessor::getState(IBStream *state) {
    if (!state) return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);
    streamer.writeBool(bypass);
    streamer.writeFloat(saturation);
    streamer.writeInt32(algorithmMode);
    streamer.writeFloat(gain);
    streamer.writeBool(phaseInverse);
    return kResultOk;
}

uint32 BaseProcessor::getLatencySamples()
{
    return algorithmMode == BEAT_MODE ? static_cast<uint32>(latency) : 0;
}

bool BaseProcessor::restartMessage()
{
    IPtr<IMessage> message = shared(allocateMessage());
    if (message != nullptr) {
        message->setMessageID(RESTART_MESSAGE);
        if (message != nullptr && getPeer() != nullptr) {
            return getPeer()->notify(message);
        }
    }
    return kResultTrue;
}

tresult BaseProcessor::notify(IMessage *message)
{
    return ComponentBase::notify(message);
}

tresult BaseProcessor::setBusArrangements(SpeakerArrangement *inputs, int32 numIns, SpeakerArrangement *outputs, int32 numOuts)
{
        if (numIns == 1 && numOuts == 1 && inputs[0] == Vst::SpeakerArr::kStereo && outputs[0] == Vst::SpeakerArr::kStereo)
        {
            return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
	    }
        return kResultFalse;
}

template<typename SampleType>
SampleType BaseProcessor::cosCos(SampleType val, double coef) {
    if (coef > 1.0) coef = 1.0;
    if (coef < 0.0) coef = 0.0;
    double invCoef = 1.0 - coef;
    invCoef = 0.2 + (invCoef * 0.45);
    return fasterpow(invCoef, 4.35) + fastercos(fastercos(val) * invCoef * 2.65);
}

void BaseProcessor::recalculateBlendRatio(double ratio) {
    calculatedRatio = fasterpow(ratio, 0.2);
}

template<typename SampleType>
SampleType BaseProcessor::blend(SampleType signal1, SampleType signal2) {
    return (1.0 - calculatedRatio) * signal1 + calculatedRatio * signal2;
}

float BaseProcessor::randFloat()
{
    return (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) / 2.0f;
}
