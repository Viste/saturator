#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <array>
#include <cmath>

namespace gui::widgets {

// поворотный регулятор
bool Knob(const char* label, float* value, float minVal = 0.0f, float maxVal = 1.0f,
          float defaultVal = 0.0f, float diameter = 80.0f, bool showValue = true);

// осциллограмма вход/выход
void WaveformOverlay(const char* label,
                     const float* inputSamples, const float* outputSamples,
                     int numSamples, int inputWritePos, int outputWritePos,
                     ImVec2 size);

// переключатель режимов
bool ModeSelector(const char* label, int* mode, const char* const* names, int count);

// кнопка вкл/выкл
bool ToggleButton(const char* label, bool* active, float width = 60.0f, float height = 28.0f);

// вертикальный индикатор уровня
void LevelMeter(const char* label, float level, float width = 8.0f, float height = 100.0f);

// спектроанализатор
void SpectrumDisplay(const char* label,
                     const float* samples, int numSamples, int writePos,
                     ImVec2 size, float sampleRate = 44100.0f);

// фон панели
void DrawBackground(ImVec2 pos, ImVec2 size);

} // namespace gui::widgets
