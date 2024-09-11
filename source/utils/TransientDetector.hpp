#ifndef TRANSIENT_DETECTOR_HPP
#define TRANSIENT_DETECTOR_HPP

#include <q/fx/moving_average.hpp>
#include <cmath>
#include <vector>

template<typename SampleType, std::size_t windowSize>
class TransientDetector {
public:
    // Инициализация с размером окна для Moving Average
    TransientDetector() : prevSample(0), ma() {}

    // Обработка текущего сэмпла
    bool process(SampleType currentSample, SampleType threshold) {
        // Вычисляем абсолютную разницу между текущим и предыдущим сэмплами
        SampleType delta = std::abs(currentSample - prevSample);
        // Обновляем предыдущий сэмпл
        prevSample = currentSample;
        // Применяем Moving Average к разнице
        auto smoothedDelta = ma(delta);
        // Возвращаем true, если сглаженная разница превышает порог
        return smoothedDelta > threshold;
    }

private:
    cycfi::q::exp_moving_average<windowSize> ma;
    SampleType prevSample;
};

#endif // TRANSIENT_DETECTOR_HPP