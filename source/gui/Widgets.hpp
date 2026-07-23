#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

struct ImFont;

namespace gui::widgets {

void SetUIScale(float scale);
float UIScale();

ImVec2 MeasureText(ImFont* font, float sizePx, const char* text);
void DrawTextPx(ImFont* font, float sizePx, ImVec2 pos, ImU32 col, const char* text);

void PanelFrame(ImVec2 min, ImVec2 max);

bool Knob(const char* label, float* value, float minVal = 0.0f, float maxVal = 1.0f,
          float defaultVal = 0.0f, float diameter = 90.0f, bool showValue = true,
          const char* spriteName = "knob_big", float labelGapPx = 0.0f);

void WaveformOverlay(const char* label,
                     const float* inputSamples, const float* outputSamples,
                     int numSamples, int inputWritePos, int outputWritePos,
                     ImVec2 size);

bool ModeSelectorButton(const char* label, bool active, float width, float height);

bool ToggleButton(const char* label, bool* active, float width, float height);

void InOutMeter(float inLevel, float outLevel, float width, float height);

void SpectrumDisplay(const char* label,
                     const float* samples, int numSamples, int writePos,
                     ImVec2 size, float sampleRate = 44100.0f);

void DrawBackground(ImVec2 pos, ImVec2 size);

} // namespace gui::widgets
