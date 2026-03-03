#define _USE_MATH_DEFINES
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "Widgets.hpp"
#include "Locale.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gui::widgets {

// палитра цветов
static constexpr ImU32 kBgDark       = IM_COL32(30, 28, 26, 255);
static constexpr ImU32 kBgPanel      = IM_COL32(38, 36, 33, 255);
static constexpr ImU32 kMetal        = IM_COL32(80, 76, 68, 255);
static constexpr ImU32 kMetalLight   = IM_COL32(110, 105, 92, 255);
static constexpr ImU32 kMetalDark    = IM_COL32(45, 42, 38, 255);
static constexpr ImU32 kAccent       = IM_COL32(210, 130, 50, 255);
static constexpr ImU32 kAccentBright = IM_COL32(245, 170, 60, 255);
static constexpr ImU32 kAccentDim    = IM_COL32(160, 90, 30, 255);
static constexpr ImU32 kCream        = IM_COL32(200, 192, 170, 255);
static constexpr ImU32 kCreamDim     = IM_COL32(140, 132, 115, 255);
static constexpr ImU32 kGreenDim     = IM_COL32(60, 120, 60, 130);
static constexpr ImU32 kRed          = IM_COL32(200, 60, 40, 255);

void DrawBackground(ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(36, 34, 30, 255), IM_COL32(32, 30, 27, 255),
        IM_COL32(28, 26, 24, 255), IM_COL32(34, 32, 28, 255));

    for (int y = 0; y < static_cast<int>(size.y); y += 3) {
        int alpha = 4 + (y * 7) % 8;
        dl->AddLine(ImVec2(pos.x, pos.y + y), ImVec2(pos.x + size.x, pos.y + y),
                    IM_COL32(60, 56, 48, alpha));
    }

    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(55, 52, 45, 255), 0, 0, 1.0f);
    dl->AddRect(ImVec2(pos.x + 1, pos.y + 1),
                ImVec2(pos.x + size.x - 1, pos.y + size.y - 1),
                IM_COL32(20, 18, 16, 200), 0, 0, 1.0f);

    float margin = 12.0f;
    ImVec2 screws[] = {
        {pos.x + margin, pos.y + margin},
        {pos.x + size.x - margin, pos.y + margin},
        {pos.x + margin, pos.y + size.y - margin},
        {pos.x + size.x - margin, pos.y + size.y - margin}
    };
    for (auto& s : screws) {
        dl->AddCircleFilled(s, 4.0f, kMetalLight, 16);
        dl->AddCircle(s, 4.0f, kMetalDark, 16, 1.0f);
        dl->AddLine(ImVec2(s.x - 2, s.y), ImVec2(s.x + 2, s.y), kMetalDark, 1.0f);
        dl->AddLine(ImVec2(s.x, s.y - 2), ImVec2(s.x, s.y + 2), kMetalDark, 1.0f);
    }
}

bool Knob(const char* label, float* value, float minVal, float maxVal,
          float defaultVal, float diameter, bool showValue) {
    ImGuiIO& io = ImGui::GetIO();
    float radius = diameter * 0.5f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    float labelH = 18.0f;
    float valueH = showValue ? 14.0f : 0.0f;
    float totalH = diameter + labelH + valueH;

    ImGui::InvisibleButton(label, ImVec2(diameter, totalH));
    bool isActive = ImGui::IsItemActive();
    bool isHovered = ImGui::IsItemHovered();
    bool changed = false;

    // двойной клик — сброс
    if (isHovered && ImGui::IsMouseDoubleClicked(0)) {
        *value = defaultVal;
        changed = true;
    }

    // вертикальное перетаскивание
    if (isActive && io.MouseDelta.y != 0.0f) {
        float speed = (maxVal - minVal) / 200.0f;
        if (io.KeyShift) speed *= 0.1f;
        *value -= io.MouseDelta.y * speed;
        *value = std::clamp(*value, minVal, maxVal);
        changed = true;
    }

    if (isHovered && io.MouseWheel != 0.0f) {
        float speed = (maxVal - minVal) / 100.0f;
        if (io.KeyShift) speed *= 0.1f;
        *value += io.MouseWheel * speed;
        *value = std::clamp(*value, minVal, maxVal);
        changed = true;
    }

    float norm = (*value - minVal) / (maxVal - minVal);
    float angleMin = static_cast<float>(M_PI) * 0.75f;
    float angleMax = static_cast<float>(M_PI) * 2.25f;
    float angle = angleMin + norm * (angleMax - angleMin);

    dl->AddCircleFilled(ImVec2(center.x + 1, center.y + 2), radius + 1,
                        IM_COL32(0, 0, 0, 60), 32);

    dl->AddCircleFilled(center, radius, kMetal, 32);
    dl->AddCircle(center, radius, kMetalDark, 32, 2.0f);

    float faceR = radius * 0.75f;
    ImU32 faceCol = isActive  ? IM_COL32(72, 68, 60, 255)
                  : isHovered ? IM_COL32(65, 62, 55, 255)
                              : IM_COL32(55, 52, 46, 255);
    dl->AddCircleFilled(center, faceR, faceCol, 32);
    dl->AddCircle(center, faceR, IM_COL32(40, 38, 34, 255), 32, 1.0f);

    int tickCount = static_cast<int>(diameter * 0.5f);
    for (int k = 0; k < tickCount; ++k) {
        float a = static_cast<float>(k) / static_cast<float>(tickCount) * 2.0f * static_cast<float>(M_PI);
        float r1 = radius * 0.78f;
        float r2 = radius * 0.96f;
        dl->AddLine(
            ImVec2(center.x + std::cos(a) * r1, center.y + std::sin(a) * r1),
            ImVec2(center.x + std::cos(a) * r2, center.y + std::sin(a) * r2),
            IM_COL32(35, 33, 30, 150), 1.0f);
    }

    float trackR = faceR * 0.72f;
    float trackW = 3.0f;
    for (float a = angleMin; a < angleMax; a += 0.03f) {
        float x1 = center.x + std::cos(a) * trackR;
        float y1 = center.y + std::sin(a) * trackR;
        float x2 = center.x + std::cos(a + 0.03f) * trackR;
        float y2 = center.y + std::sin(a + 0.03f) * trackR;
        dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(35, 33, 30, 200), trackW);
    }

    for (float a = angleMin; a < angle; a += 0.03f) {
        float x1 = center.x + std::cos(a) * trackR;
        float y1 = center.y + std::sin(a) * trackR;
        float x2 = center.x + std::cos(a + 0.03f) * trackR;
        float y2 = center.y + std::sin(a + 0.03f) * trackR;

        float t = (a - angleMin) / (angleMax - angleMin);
        ImU32 arcCol = (t < 0.6f) ? kAccent : (t < 0.85f) ? kAccentBright : kRed;
        dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), arcCol, trackW);
    }

    float ptrInner = 4.0f;
    float ptrOuter = faceR * 0.9f;
    float px1 = center.x + std::cos(angle) * ptrInner;
    float py1 = center.y + std::sin(angle) * ptrInner;
    float px2 = center.x + std::cos(angle) * ptrOuter;
    float py2 = center.y + std::sin(angle) * ptrOuter;
    dl->AddLine(ImVec2(px1, py1), ImVec2(px2, py2), kCream, 2.5f);

    dl->AddCircleFilled(center, 3.0f, kMetalLight, 12);

    ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(center.x - textSize.x * 0.5f, pos.y + diameter + 2.0f),
                kCream, label);

    if (showValue) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(norm * 100));
        ImVec2 valSize = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(center.x - valSize.x * 0.5f, pos.y + diameter + labelH),
                    kCreamDim, buf);
    }

    return changed;
}

void LevelMeter(const char* label, float level, float width, float height) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height),
                      IM_COL32(22, 20, 18, 255));
    dl->AddRect(pos, ImVec2(pos.x + width, pos.y + height),
                kMetalDark, 0, 0, 1.0f);

    float clamped = std::clamp(level, 0.0f, 1.0f);
    float barH = clamped * height;
    float barTop = pos.y + height - barH;

    for (float y = barTop; y < pos.y + height; y += 1.0f) {
        float t = (pos.y + height - y) / height;
        ImU32 color;
        if (t < 0.6f)
            color = IM_COL32(40, static_cast<int>(130 + t * 100), 40, 255);
        else if (t < 0.85f)
            color = IM_COL32(static_cast<int>(40 + (t - 0.6f) / 0.25f * 180), 160, 30, 255);
        else
            color = IM_COL32(200, static_cast<int>(100 * (1.0f - (t - 0.85f) / 0.15f)), 20, 255);
        dl->AddRectFilled(ImVec2(pos.x + 1, y), ImVec2(pos.x + width - 1, y + 1.0f), color);
    }

    ImGui::Dummy(ImVec2(width, height));
}

void WaveformOverlay(const char* label,
                     const float* inputSamples, const float* outputSamples,
                     int numSamples, int inputWritePos, int outputWritePos,
                     ImVec2 size) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      IM_COL32(16, 15, 14, 255));
    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(48, 45, 40, 255), 0, 0, 1.5f);

    // клип — не даём линиям выходить за границы виджета
    dl->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);

    float centerY = pos.y + size.y * 0.5f;
    float halfH = size.y * 0.5f;

    // горизонтальная сетка
    dl->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                IM_COL32(45, 42, 36, 80));
    for (float frac : {0.25f, 0.50f}) {
        float dy = halfH * frac;
        dl->AddLine(ImVec2(pos.x, centerY - dy), ImVec2(pos.x + size.x, centerY - dy),
                    IM_COL32(35, 33, 28, 40));
        dl->AddLine(ImVec2(pos.x, centerY + dy), ImVec2(pos.x + size.x, centerY + dy),
                    IM_COL32(35, 33, 28, 40));
    }

    // вертикальная сетка
    for (int q = 1; q < 4; ++q) {
        float gx = pos.x + size.x * (static_cast<float>(q) / 4.0f);
        dl->AddLine(ImVec2(gx, pos.y), ImVec2(gx, pos.y + size.y),
                    IM_COL32(35, 33, 28, 30));
    }

    if (!inputSamples || !outputSamples || numSamples == 0) {
        dl->PopClipRect();
        ImGui::Dummy(size);
        return;
    }

    int dispW = static_cast<int>(size.x);
    float samplesPerPx = static_cast<float>(numSamples) / static_cast<float>(dispW);
    float amp = halfH * 0.84f;
    bool useMinMax = samplesPerPx > 1.5f;

    auto getSample = [&](const float* buf, int wp, int px) -> float {
        int idx = (wp - numSamples + static_cast<int>(px * samplesPerPx) + numSamples * 2) % numSamples;
        return std::clamp(buf[idx], -1.0f, 1.0f);
    };

    auto getMinMax = [&](const float* buf, int wp, int px, float& sMin, float& sMax) {
        int start = static_cast<int>(px * samplesPerPx);
        int end = static_cast<int>((px + 1) * samplesPerPx);
        sMin = 1.0f; sMax = -1.0f;
        for (int j = start; j < end; ++j) {
            int idx = (wp - numSamples + j + numSamples * 2) % numSamples;
            float s = std::clamp(buf[idx], -1.0f, 1.0f);
            sMin = std::min(sMin, s);
            sMax = std::max(sMax, s);
        }
    };

    // входной сигнал — тонкий, серый фон
    ImU32 inCol = IM_COL32(90, 85, 75, 70);
    if (useMinMax) {
        for (int i = 0; i < dispW; ++i) {
            float sMin, sMax;
            getMinMax(inputSamples, inputWritePos, i, sMin, sMax);
            float y0 = centerY - sMax * amp;
            float y1 = centerY - sMin * amp;
            if (y1 - y0 < 1.0f) y1 = y0 + 1.0f;
            dl->AddLine(ImVec2(pos.x + i, y0), ImVec2(pos.x + i, y1), inCol, 1.0f);
        }
    } else {
        ImVec2 prev(0, 0);
        for (int i = 0; i < dispW; ++i) {
            float s = getSample(inputSamples, inputWritePos, i);
            ImVec2 pt(pos.x + i, centerY - s * amp);
            if (i > 0) dl->AddLine(prev, pt, inCol, 0.8f);
            prev = pt;
        }
    }

    // выходной сигнал — бирюзовый с заливкой
    ImU32 outLine = IM_COL32(60, 190, 180, 220);
    ImU32 fillCol = IM_COL32(40, 160, 150, 24);
    if (useMinMax) {
        for (int i = 0; i < dispW; ++i) {
            float sMin, sMax;
            getMinMax(outputSamples, outputWritePos, i, sMin, sMax);
            float yTop = centerY - sMax * amp;
            float yBot = centerY - sMin * amp;
            float fillTop = std::min(yTop, centerY);
            float fillBot = std::max(yBot, centerY);
            dl->AddRectFilled(ImVec2(pos.x + i, fillTop), ImVec2(pos.x + i + 1, fillBot), fillCol);
        }
        for (int i = 0; i < dispW; ++i) {
            float sMin, sMax;
            getMinMax(outputSamples, outputWritePos, i, sMin, sMax);
            float y0 = centerY - sMax * amp;
            float y1 = centerY - sMin * amp;
            if (y1 - y0 < 1.0f) y1 = y0 + 1.0f;
            dl->AddLine(ImVec2(pos.x + i, y0), ImVec2(pos.x + i, y1), outLine, 1.5f);
        }
    } else {
        for (int i = 0; i < dispW; ++i) {
            float s = getSample(outputSamples, outputWritePos, i);
            float yS = centerY - s * amp;
            float top = std::min(yS, centerY);
            float bot = std::max(yS, centerY);
            dl->AddRectFilled(ImVec2(pos.x + i, top), ImVec2(pos.x + i + 1, bot), fillCol);
        }
        ImVec2 prev(0, 0);
        for (int i = 0; i < dispW; ++i) {
            float s = getSample(outputSamples, outputWritePos, i);
            ImVec2 pt(pos.x + i, centerY - s * amp);
            if (i > 0) dl->AddLine(prev, pt, outLine, 1.8f);
            prev = pt;
        }
    }

    dl->PopClipRect();

    dl->AddText(ImVec2(pos.x + 5, pos.y + 3), kCreamDim, label);
    float lx = pos.x + size.x - 90;
    dl->AddLine(ImVec2(lx, pos.y + 9), ImVec2(lx + 12, pos.y + 9), inCol, 1.0f);
    const auto& loc = gui::locale();
    dl->AddText(ImVec2(lx + 15, pos.y + 3), kCreamDim, loc.wfIn);
    dl->AddLine(ImVec2(lx + 32, pos.y + 9), ImVec2(lx + 44, pos.y + 9), outLine, 1.8f);
    dl->AddText(ImVec2(lx + 47, pos.y + 3), kCreamDim, loc.wfOut);

    ImGui::Dummy(size);
}

// fft cooley-tukey radix-2, data: [re, im, re, im, ...], длина = 2*n
static void simpleFFT(float* data, int n) {
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j ^= bit;
        if (i < j) {
            std::swap(data[2*i],   data[2*j]);
            std::swap(data[2*i+1], data[2*j+1]);
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * static_cast<float>(M_PI) / static_cast<float>(len);
        float wRe = std::cos(ang), wIm = std::sin(ang);
        for (int i = 0; i < n; i += len) {
            float curRe = 1.0f, curIm = 0.0f;
            for (int j = 0; j < len / 2; ++j) {
                int u = i + j, v = i + j + len / 2;
                float tRe = data[2*v] * curRe - data[2*v+1] * curIm;
                float tIm = data[2*v] * curIm + data[2*v+1] * curRe;
                data[2*v]   = data[2*u]   - tRe;
                data[2*v+1] = data[2*u+1] - tIm;
                data[2*u]   += tRe;
                data[2*u+1] += tIm;
                float tmp = curRe * wRe - curIm * wIm;
                curIm = curRe * wIm + curIm * wRe;
                curRe = tmp;
            }
        }
    }
}

// частота бина → позиция X (логарифмическая шкала 20Hz..nyquist)
static float freqToX(float freq, float minFreq, float maxFreq, float width) {
    float logMin = std::log2(minFreq);
    float logMax = std::log2(maxFreq);
    float t = (std::log2(std::max(freq, minFreq)) - logMin) / (logMax - logMin);
    return std::clamp(t, 0.0f, 1.0f) * width;
}

void SpectrumDisplay(const char* label,
                     const float* samples, int numSamples, int writePos,
                     ImVec2 size, float sampleRate) {
    static constexpr int kFFTSize = 1024;
    static constexpr int kBins = kFFTSize / 2;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      IM_COL32(16, 15, 14, 255));
    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(48, 45, 40, 255), 0, 0, 1.5f);

    // клип — содержимое не выходит за границы виджета
    dl->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);

    float minDb = -66.0f;
    float maxDb = 0.0f;
    float padBottom = 14.0f;
    float padLeft = 28.0f;
    float padRight = 18.0f;
    float drawW = size.x - padLeft - padRight;
    float drawH = size.y - padBottom - 14.0f;
    float drawLeft = pos.x + padLeft;
    float drawTop = pos.y + 12.0f;
    float bottom = drawTop + drawH;

    float minFreq = 30.0f;
    float nyquist = sampleRate * 0.5f;
    float maxFreq = std::min(nyquist, 20000.0f);

    // сетка дБ
    struct DbLabel { float db; const char* text; };
    DbLabel dbLabels[] = {{-12, "-12"}, {-24, "-24"}, {-48, "-48"}};
    for (auto& dl_item : dbLabels) {
        float norm = (dl_item.db - minDb) / (maxDb - minDb);
        float y = bottom - norm * drawH;
        dl->AddLine(ImVec2(drawLeft, y), ImVec2(drawLeft + drawW, y),
                    IM_COL32(40, 38, 34, 50));
        ImVec2 ts = ImGui::CalcTextSize(dl_item.text);
        dl->AddText(ImVec2(drawLeft - ts.x - 4, y - ts.y * 0.5f),
                    IM_COL32(60, 57, 50, 180), dl_item.text);
    }

    // сетка частот
    struct FreqLabel { float hz; const char* text; };
    FreqLabel freqLabels[] = {
        {100, "100"}, {500, "500"}, {1000, "1k"},
        {5000, "5k"}, {10000, "10k"}, {20000, "20k"}
    };
    for (auto& fl : freqLabels) {
        if (fl.hz > maxFreq) continue;
        float x = drawLeft + freqToX(fl.hz, minFreq, maxFreq, drawW);
        dl->AddLine(ImVec2(x, drawTop), ImVec2(x, bottom),
                    IM_COL32(40, 38, 34, 40));
        ImVec2 ts = ImGui::CalcTextSize(fl.text);
        dl->AddText(ImVec2(x - ts.x * 0.5f, bottom + 2),
                    IM_COL32(60, 57, 50, 180), fl.text);
    }

    if (!samples || numSamples == 0) {
        dl->PopClipRect();
        dl->AddText(ImVec2(pos.x + 5, pos.y + 3), kCreamDim, label);
        ImGui::Dummy(size);
        return;
    }

    // fft с окном ханна
    static float fftBuf[kFFTSize * 2];
    int available = std::min(numSamples, kFFTSize);
    for (int i = 0; i < kFFTSize; ++i) {
        int idx = (writePos - available + i + numSamples * 2) % numSamples;
        float w = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (kFFTSize - 1)));
        fftBuf[2 * i]     = (i < available) ? samples[idx] * w : 0.0f;
        fftBuf[2 * i + 1] = 0.0f;
    }
    simpleFFT(fftBuf, kFFTSize);

    // магнитудный спектр → дБ
    float magDb[kBins + 1];
    for (int i = 0; i <= kBins; ++i) {
        float re = fftBuf[2 * i];
        float im = fftBuf[2 * i + 1];
        float mag = std::sqrt(re * re + im * im) / static_cast<float>(kFFTSize);
        magDb[i] = std::max(20.0f * std::log10(std::max(mag, 1e-10f)), -80.0f);
    }

    // сглаживание между кадрами
    static float smoothed[kBins + 1] = {};
    static bool smoothedInit = false;
    if (!smoothedInit) {
        for (int i = 0; i <= kBins; ++i) smoothed[i] = -80.0f;
        smoothedInit = true;
    }
    for (int i = 0; i <= kBins; ++i) {
        if (magDb[i] > smoothed[i])
            smoothed[i] = magDb[i];
        else
            smoothed[i] = smoothed[i] * 0.82f + magDb[i] * 0.18f;
    }

    // рисуем спектр
    int dispW = static_cast<int>(drawW);
    float logMin = std::log2(minFreq);
    float logMax = std::log2(maxFreq);
    float hzPerBin = sampleRate / static_cast<float>(kFFTSize);

    // собираем точки кривой
    ImVec2 prevPt(drawLeft, bottom);
    for (int px = 0; px <= dispW; ++px) {
        float t = static_cast<float>(px) / static_cast<float>(dispW);
        float freq = std::pow(2.0f, logMin + t * (logMax - logMin));
        float binF = freq / hzPerBin;

        int binLo = std::clamp(static_cast<int>(binF), 1, kBins - 1);
        int binHi = std::min(binLo + 1, kBins);
        float frac = binF - static_cast<float>(binLo);

        float db;
        // усреднение соседних бинов на высоких частотах
        int spread = std::max(1, static_cast<int>(binF * 0.04f));
        if (spread <= 1) {
            db = smoothed[binLo] * (1.0f - frac) + smoothed[binHi] * frac;
        } else {
            float sum = 0.0f;
            int count = 0;
            for (int b = std::max(1, binLo - spread); b <= std::min(kBins, binLo + spread); ++b) {
                sum += smoothed[b];
                count++;
            }
            db = sum / static_cast<float>(count);
        }

        float norm = std::clamp((db - minDb) / (maxDb - minDb), 0.0f, 1.0f);
        float y = bottom - norm * drawH;
        float x = drawLeft + px;
        ImVec2 curPt(x, y);

        if (px > 0) {
            // цвет по частоте: басы красно-оранж → середина жёлто-зелёный → верха голубые
            auto freqColor = [](float freqT, float alpha) -> ImU32 {
                float r, g, b;
                if (freqT < 0.33f) {
                    // басы: красно-оранжевый → жёлтый
                    float u = freqT / 0.33f;
                    r = 0.85f + u * 0.15f;
                    g = 0.30f + u * 0.50f;
                    b = 0.10f;
                } else if (freqT < 0.66f) {
                    // середина: жёлтый → зелёный
                    float u = (freqT - 0.33f) / 0.33f;
                    r = 1.0f - u * 0.65f;
                    g = 0.80f + u * 0.10f;
                    b = 0.10f + u * 0.25f;
                } else {
                    // верха: зелёный → голубой/синий
                    float u = (freqT - 0.66f) / 0.34f;
                    r = 0.35f - u * 0.20f;
                    g = 0.90f - u * 0.30f;
                    b = 0.35f + u * 0.55f;
                }
                return IM_COL32(
                    static_cast<int>(r * 255),
                    static_cast<int>(g * 255),
                    static_cast<int>(b * 255),
                    static_cast<int>(alpha * 255));
            };

            // заливка с градиентом по частоте
            ImU32 fillBot = freqColor(t, 0.04f);
            ImU32 fillTop = freqColor(t, 0.22f);
            dl->AddQuadFilled(prevPt, curPt,
                              ImVec2(x, bottom), ImVec2(prevPt.x, bottom), fillBot);
            float midY = curPt.y + (bottom - curPt.y) * 0.35f;
            float midYprev = prevPt.y + (bottom - prevPt.y) * 0.35f;
            dl->AddQuadFilled(prevPt, curPt,
                              ImVec2(x, midY), ImVec2(prevPt.x, midYprev), fillTop);

            // линия спектра — ярче
            ImU32 lineCol = freqColor(t, 0.88f);
            dl->AddLine(prevPt, curPt, lineCol, 1.5f);
        }
        prevPt = curPt;
    }

    dl->PopClipRect();

    dl->AddText(ImVec2(pos.x + 5, pos.y + 3), kCreamDim, label);

    ImGui::Dummy(size);
}

bool ModeSelector(const char* label, int* mode, const char* const* names, int count) {
    bool changed = false;
    ImGui::PushID(label);
    ImGui::BeginGroup();

    for (int i = 0; i < count; ++i) {
        if (i > 0) ImGui::SameLine(0, 3);

        bool selected = (*mode == i);

        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52f, 0.32f, 0.12f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.38f, 0.16f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.86f, 0.74f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.17f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.24f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.47f, 0.40f, 1.0f));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.26f, 0.22f, 1.0f));

        if (ImGui::Button(names[i], ImVec2(110, 28))) {
            *mode = i;
            changed = true;
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
    }

    ImGui::EndGroup();
    ImGui::PopID();
    return changed;
}

bool ToggleButton(const char* label, bool* active, float width, float height) {
    bool changed = false;
    ImGui::PushID(label);

    if (*active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52f, 0.32f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.58f, 0.38f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.86f, 0.74f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.17f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.24f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.47f, 0.40f, 1.0f));
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.28f, 0.26f, 0.22f, 1.0f));

    if (ImGui::Button(label, ImVec2(width, height))) {
        *active = !(*active);
        changed = true;
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    return changed;
}

} // namespace gui::widgets
