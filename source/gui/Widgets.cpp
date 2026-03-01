#define _USE_MATH_DEFINES
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "Widgets.hpp"
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

    float centerY = pos.y + size.y * 0.5f;

    dl->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                IM_COL32(45, 42, 36, 80));
    float qH = size.y * 0.25f;
    dl->AddLine(ImVec2(pos.x, centerY - qH), ImVec2(pos.x + size.x, centerY - qH),
                IM_COL32(35, 33, 28, 50));
    dl->AddLine(ImVec2(pos.x, centerY + qH), ImVec2(pos.x + size.x, centerY + qH),
                IM_COL32(35, 33, 28, 50));

    if (!inputSamples || !outputSamples || numSamples == 0) {
        ImGui::Dummy(size);
        return;
    }

    int dispW = static_cast<int>(size.x);
    float step = static_cast<float>(numSamples) / static_cast<float>(dispW);

    // входной сигнал (приглушённый зелёный)
    ImVec2 prevIn(0, 0);
    for (int i = 0; i < dispW; ++i) {
        int idx = (inputWritePos - numSamples + static_cast<int>(i * step) + numSamples * 2) % numSamples;
        float s = std::clamp(inputSamples[idx], -1.0f, 1.0f);
        ImVec2 pt(pos.x + i, centerY - s * size.y * 0.42f);
        if (i > 0) dl->AddLine(prevIn, pt, kGreenDim, 1.0f);
        prevIn = pt;
    }

    // выходной сигнал (яркий оранжевый — характер сатурации)
    ImVec2 prevOut(0, 0);
    for (int i = 0; i < dispW; ++i) {
        int idx = (outputWritePos - numSamples + static_cast<int>(i * step) + numSamples * 2) % numSamples;
        float s = std::clamp(outputSamples[idx], -1.0f, 1.0f);
        ImVec2 pt(pos.x + i, centerY - s * size.y * 0.42f);
        if (i > 0) dl->AddLine(prevOut, pt, kAccent, 1.5f);
        prevOut = pt;
    }

    dl->AddText(ImVec2(pos.x + 5, pos.y + 3), kCreamDim, label);
    float lx = pos.x + size.x - 90;
    dl->AddLine(ImVec2(lx, pos.y + 9), ImVec2(lx + 12, pos.y + 9), kGreenDim, 1.0f);
    dl->AddText(ImVec2(lx + 15, pos.y + 3), kCreamDim, "\xd0\xb2\xd1\x85");
    dl->AddLine(ImVec2(lx + 32, pos.y + 9), ImVec2(lx + 44, pos.y + 9), kAccent, 1.5f);
    dl->AddText(ImVec2(lx + 47, pos.y + 3), kCreamDim, "\xd0\xb2\xd1\x8b\xd1\x85");

    ImGui::Dummy(size);
}

// hsv → ImU32 для радужной раскраски спектра
static ImU32 hsvColor(float h, float s, float v, float a) {
    float c = v * s;
    float hp = h / 60.0f;
    float x = c * (1.0f - std::abs(std::fmod(hp, 2.0f) - 1.0f));
    float r1, g1, b1;
    if      (hp < 1.0f) { r1 = c; g1 = x; b1 = 0; }
    else if (hp < 2.0f) { r1 = x; g1 = c; b1 = 0; }
    else if (hp < 3.0f) { r1 = 0; g1 = c; b1 = x; }
    else if (hp < 4.0f) { r1 = 0; g1 = x; b1 = c; }
    else if (hp < 5.0f) { r1 = x; g1 = 0; b1 = c; }
    else                 { r1 = c; g1 = 0; b1 = x; }
    float m = v - c;
    return IM_COL32(static_cast<int>((r1+m)*255),
                    static_cast<int>((g1+m)*255),
                    static_cast<int>((b1+m)*255),
                    static_cast<int>(a*255));
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

void SpectrumDisplay(const char* label,
                     const float* samples, int numSamples, int writePos,
                     ImVec2 size) {
    static constexpr int kFFTSize = 256;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                      IM_COL32(16, 15, 14, 255));
    dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(48, 45, 40, 255), 0, 0, 1.5f);

    if (!samples || numSamples == 0) {
        ImGui::Dummy(size);
        return;
    }

    // последние kFFTSize семплов с окном ханна
    float fftBuf[kFFTSize * 2];
    for (int i = 0; i < kFFTSize; ++i) {
        int idx = (writePos - kFFTSize + i + numSamples * 2) % numSamples;
        float w = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (kFFTSize - 1)));
        fftBuf[2 * i]     = samples[idx] * w;
        fftBuf[2 * i + 1] = 0.0f;
    }

    simpleFFT(fftBuf, kFFTSize);

    // магнитудный спектр в дб
    static constexpr int kBins = kFFTSize / 2;
    float magDb[kBins + 1];
    for (int i = 0; i <= kBins; ++i) {
        float re = fftBuf[2 * i];
        float im = fftBuf[2 * i + 1];
        float mag = std::sqrt(re * re + im * im) / static_cast<float>(kFFTSize);
        magDb[i] = std::max(20.0f * std::log10(std::max(mag, 1e-10f)), -80.0f);
    }

    float bottom = pos.y + size.y - 1;
    float drawH = size.y - 16;
    float minDb = -66.0f;
    float maxDb = 0.0f;
    int dispW = static_cast<int>(size.x) - 2;

    float logMin = std::log2(1.0f);
    float logMax = std::log2(static_cast<float>(kBins));

    ImVec2 prevPt(pos.x + 1, bottom);
    for (int px = 0; px <= dispW; ++px) {
        float t = static_cast<float>(px) / static_cast<float>(dispW);
        float logBin = logMin + t * (logMax - logMin);
        float binF = std::pow(2.0f, logBin);

        int binLo = std::clamp(static_cast<int>(binF), 0, kBins - 1);
        int binHi = std::min(binLo + 1, kBins);
        float frac = binF - static_cast<float>(binLo);

        float db;
        int spread = std::max(1, static_cast<int>(binF * 0.05f));
        if (spread <= 1) {
            db = magDb[binLo] * (1.0f - frac) + magDb[binHi] * frac;
        } else {
            float sum = 0.0f;
            int count = 0;
            for (int b = std::max(0, binLo - spread); b <= std::min(kBins, binLo + spread); ++b) {
                sum += magDb[b];
                count++;
            }
            db = sum / static_cast<float>(count);
        }

        float norm = std::clamp((db - minDb) / (maxDb - minDb), 0.0f, 1.0f);
        float y = bottom - norm * drawH;
        float x = pos.x + 1 + px;
        ImVec2 curPt(x, y);

        if (px > 0) {
            float freqT = static_cast<float>(px) / static_cast<float>(dispW);
            float hue = freqT * 270.0f;

            float avgNorm = std::clamp(((bottom - prevPt.y) + (bottom - y)) / (2.0f * drawH), 0.0f, 1.0f);
            float fillV = 0.4f + avgNorm * 0.3f;

            ImU32 fillCol = hsvColor(hue, 0.75f, fillV, 0.45f);
            dl->AddQuadFilled(prevPt, curPt,
                              ImVec2(x, bottom), ImVec2(prevPt.x, bottom), fillCol);

            ImU32 lineCol = hsvColor(hue, 0.8f, 0.7f + avgNorm * 0.3f, 0.9f);
            dl->AddLine(prevPt, curPt, lineCol, 1.2f);
        }
        prevPt = curPt;
    }

    for (float db : {-12.0f, -24.0f, -48.0f}) {
        float norm = (db - minDb) / (maxDb - minDb);
        float y = bottom - norm * drawH;
        dl->AddLine(ImVec2(pos.x + 1, y), ImVec2(pos.x + size.x - 1, y),
                    IM_COL32(45, 42, 36, 50));
    }

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
