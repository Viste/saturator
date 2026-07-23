#define _USE_MATH_DEFINES
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include "Widgets.hpp"
#include "Fonts.hpp"
#include "Locale.hpp"
#include "TextureManager.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_map>

namespace gui::widgets {

static constexpr ImU32 kPanelFill  = IM_COL32(10, 9, 8, 255);      // #0a0908
static constexpr ImU32 kStrokeTop  = IM_COL32(26, 26, 25, 255);    // #1a1a19
static constexpr ImU32 kStrokeBot  = IM_COL32(52, 52, 51, 255);    // #343433
static constexpr ImU32 kAccent     = IM_COL32(72, 74, 66, 255);    // #484a42
static constexpr ImU32 kLabelGray  = IM_COL32(148, 148, 148, 255); // #949494
static constexpr ImU32 kTitleDim   = IM_COL32(128, 118, 96, 255);  // #807660
static constexpr ImU32 kWhite      = IM_COL32(255, 255, 255, 255);
static constexpr ImU32 kLegendIn   = IM_COL32(71, 71, 71, 255);    // #474747
static constexpr ImU32 kLegendOut  = IM_COL32(81, 138, 161, 255);  // #518aa1

static float sScale = 1.0f;

void SetUIScale(float scale) { sScale = scale; }
float UIScale() { return sScale; }

ImVec2 MeasureText(ImFont* font, float sizePx, const char* text) {
    if (!font) font = ImGui::GetFont();
    return font->CalcTextSizeA(sizePx * sScale, FLT_MAX, 0.0f, text);
}

void DrawTextPx(ImFont* font, float sizePx, ImVec2 pos, ImU32 col, const char* text) {
    if (!font) font = ImGui::GetFont();
    ImGui::GetWindowDrawList()->AddText(font, sizePx * sScale, pos, col, text);
}

void PanelFrame(ImVec2 mn, ImVec2 mx) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float sw = std::max(1.0f, 2.0f * sScale);
    dl->AddRectFilled(mn, mx, kPanelFill);
    dl->AddRectFilled(ImVec2(mn.x, mn.y), ImVec2(mx.x, mn.y + sw), kStrokeTop);
    dl->AddRectFilled(ImVec2(mn.x, mx.y - sw), ImVec2(mx.x, mx.y), kStrokeBot);
    dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y), ImVec2(mn.x + sw, mx.y),
                                kStrokeTop, kStrokeTop, kStrokeBot, kStrokeBot);
    dl->AddRectFilledMultiColor(ImVec2(mx.x - sw, mn.y), ImVec2(mx.x, mx.y),
                                kStrokeTop, kStrokeTop, kStrokeBot, kStrokeBot);
}

void DrawBackground(ImVec2 pos, ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    auto bg = TextureManager::get().find("bg");
    if (bg.id != 0) {
        dl->AddImage(bg.id, pos, ImVec2(pos.x + size.x, pos.y + size.y));
        return;
    }

    dl->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(36, 34, 30, 255), IM_COL32(32, 30, 27, 255),
        IM_COL32(28, 26, 24, 255), IM_COL32(34, 32, 28, 255));
}

bool Knob(const char* label, float* value, float minVal, float maxVal,
          float defaultVal, float diameter, bool showValue,
          const char* spriteName, float labelGapPx) {
    ImGuiIO& io = ImGui::GetIO();
    float radius = diameter * 0.5f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float kKnobFontPx = 16.5f;
    const float lineH = 18.0f * sScale;
    float labelBlockH = labelGapPx * sScale + lineH + (showValue ? lineH : 0.0f);
    float totalH = diameter + labelBlockH;

    ImGui::InvisibleButton(label, ImVec2(diameter, totalH));
    bool isActive = ImGui::IsItemActive();
    bool isHovered = ImGui::IsItemHovered();
    bool changed = false;

    if (isHovered && ImGui::IsMouseDoubleClicked(0)) {
        *value = defaultVal;
        changed = true;
    }

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
    norm = std::clamp(norm, 0.0f, 1.0f);

    // спрайт задаёт вызывающий: по diameter выбирать нельзя, он зависит от scale
    auto sprite = TextureManager::get().find(spriteName);

    if (sprite.id != 0 && sprite.width > 0 && sprite.height >= sprite.width) {
        int frameSize = sprite.width;
        int frameCount = sprite.height / frameSize;
        if (frameCount > 1) {
            int frame = static_cast<int>(norm * (frameCount - 1) + 0.5f);
            frame = std::clamp(frame, 0, frameCount - 1);

            float v0 = static_cast<float>(frame * frameSize) / static_cast<float>(sprite.height);
            float v1 = static_cast<float>((frame + 1) * frameSize) / static_cast<float>(sprite.height);

            dl->AddImage(sprite.id, pos, ImVec2(pos.x + diameter, pos.y + diameter),
                          ImVec2(0.0f, v0), ImVec2(1.0f, v1));
        }
    } else {
        float angleMin = static_cast<float>(M_PI) * 0.75f;
        float angleMax = static_cast<float>(M_PI) * 2.25f;
        float angle = angleMin + norm * (angleMax - angleMin);
        dl->AddCircleFilled(center, radius, IM_COL32(80, 76, 68, 255), 32);
        dl->AddCircle(center, radius, IM_COL32(45, 42, 38, 255), 32, 2.0f);
        float ptrOuter = radius * 0.85f;
        dl->AddLine(center,
                    ImVec2(center.x + std::cos(angle) * ptrOuter,
                           center.y + std::sin(angle) * ptrOuter),
                    kLabelGray, 2.5f);
    }

    ImFont* f = fonts().label;
    float labelY = pos.y + diameter + labelGapPx * sScale;
    ImVec2 ts = MeasureText(f, kKnobFontPx, label);
    DrawTextPx(f, kKnobFontPx, ImVec2(center.x - ts.x * 0.5f, labelY), kLabelGray, label);

    if (showValue) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(norm * 100.0f + 0.5f));
        ImVec2 vs = MeasureText(f, kKnobFontPx, buf);
        DrawTextPx(f, kKnobFontPx, ImVec2(center.x - vs.x * 0.5f, labelY + lineH), kLabelGray, buf);
    }

    return changed;
}

void InOutMeter(float inLevel, float outLevel, float width, float height) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float barW = width * (12.0f / 32.0f);
    const float gap  = width * (8.0f / 32.0f);
    const float bar1X = pos.x;
    const float bar2X = pos.x + barW + gap;

    auto drawBar = [&](float x, float level) {
        ImVec2 bMin(x, pos.y);
        ImVec2 bMax(x + barW, pos.y + height);
        PanelFrame(bMin, bMax);

        float clamped = std::clamp(level, 0.0f, 1.0f);
        if (clamped <= 0.0f) return;

        float inset = std::max(1.0f, 2.0f * sScale);
        float usableH = height - inset * 2.0f;
        float fillH = clamped * usableH;
        float fillBot = pos.y + height - inset;
        float fillTop = fillBot - fillH;

        dl->PushClipRect(ImVec2(x + inset, fillTop),
                         ImVec2(x + barW - inset, fillBot), true);
        for (float y = fillTop; y < fillBot; y += 1.0f) {
            float t = (fillBot - y) / usableH;
            ImU32 color;
            if (t < 0.6f)
                color = IM_COL32(40, static_cast<int>(130 + t * 100), 40, 230);
            else if (t < 0.85f)
                color = IM_COL32(static_cast<int>(40 + (t - 0.6f) / 0.25f * 180), 160, 30, 230);
            else
                color = IM_COL32(200, static_cast<int>(100 * (1.0f - (t - 0.85f) / 0.15f)), 20, 230);
            dl->AddRectFilled(ImVec2(x + inset, y),
                              ImVec2(x + barW - inset, y + 1.0f), color);
        }
        dl->PopClipRect();
    };

    drawBar(bar1X, inLevel);
    drawBar(bar2X, outLevel);

    const auto& loc = gui::locale();
    ImFont* f = fonts().label;
    float labelY = pos.y + height + 6.0f * sScale;
    ImVec2 inSize  = MeasureText(f, 11.0f, loc.wfIn);
    ImVec2 outSize = MeasureText(f, 11.0f, loc.wfOut);
    DrawTextPx(f, 11.0f, ImVec2(bar1X + (barW - inSize.x) * 0.5f, labelY), kWhite, loc.wfIn);
    DrawTextPx(f, 11.0f, ImVec2(bar2X + (barW - outSize.x) * 0.5f, labelY), kWhite, loc.wfOut);

    ImGui::Dummy(ImVec2(width, height + 6.0f * sScale + inSize.y));
}

void WaveformOverlay(const char* label,
                     const float* inputSamples, const float* outputSamples,
                     int numSamples, int inputWritePos, int outputWritePos,
                     ImVec2 size) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    PanelFrame(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    float inset = std::max(1.0f, 2.0f * sScale);
    dl->PushClipRect(ImVec2(pos.x + inset, pos.y + inset),
                     ImVec2(pos.x + size.x - inset, pos.y + size.y - inset), true);

    float centerY = pos.y + size.y * 0.5f;
    float halfH = size.y * 0.5f;

    dl->AddLine(ImVec2(pos.x, centerY), ImVec2(pos.x + size.x, centerY),
                IM_COL32(45, 42, 36, 80));
    for (float frac : {0.25f, 0.50f}) {
        float dy = halfH * frac;
        dl->AddLine(ImVec2(pos.x, centerY - dy), ImVec2(pos.x + size.x, centerY - dy),
                    IM_COL32(35, 33, 28, 40));
        dl->AddLine(ImVec2(pos.x, centerY + dy), ImVec2(pos.x + size.x, centerY + dy),
                    IM_COL32(35, 33, 28, 40));
    }

    for (int q = 1; q < 4; ++q) {
        float gx = pos.x + size.x * (static_cast<float>(q) / 4.0f);
        dl->AddLine(ImVec2(gx, pos.y), ImVec2(gx, pos.y + size.y),
                    IM_COL32(35, 33, 28, 30));
    }

    ImU32 inCol = IM_COL32(71, 71, 71, 200);       // #474747
    ImU32 outLine = IM_COL32(81, 138, 161, 230);   // #518aa1
    ImU32 fillCol = IM_COL32(81, 138, 161, 26);

    if (inputSamples && outputSamples && numSamples > 0) {
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
    }

    dl->PopClipRect();

    DrawTextPx(fonts().label, 14.0f, ImVec2(pos.x + 10.0f * sScale, pos.y + 6.0f * sScale),
               kTitleDim, label);

    {
        const auto& loc = gui::locale();
        ImFont* f = fonts().label;
        float right = pos.x + size.x;
        float swY = pos.y + 12.0f * sScale;
        float swH = std::max(1.0f, 1.5f * sScale);
        float txtY = swY - 7.0f * sScale;
        dl->AddRectFilled(ImVec2(right - 102.0f * sScale, swY),
                          ImVec2(right - 90.0f * sScale, swY + swH), kLegendIn);
        DrawTextPx(f, 10.5f, ImVec2(right - 86.0f * sScale, txtY), kTitleDim, loc.wfIn);
        dl->AddRectFilled(ImVec2(right - 62.0f * sScale, swY),
                          ImVec2(right - 50.0f * sScale, swY + swH), kLegendOut);
        DrawTextPx(f, 10.5f, ImVec2(right - 46.0f * sScale, txtY), kTitleDim, loc.wfOut);
    }

    ImGui::Dummy(size);
}

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

    PanelFrame(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    float inset = std::max(1.0f, 2.0f * sScale);
    dl->PushClipRect(ImVec2(pos.x + inset, pos.y + inset),
                     ImVec2(pos.x + size.x - inset, pos.y + size.y - inset), true);

    ImFont* gridFont = fonts().label;
    const ImU32 kGridText = IM_COL32(128, 118, 96, 150);

    float minDb = -66.0f;
    float maxDb = 0.0f;
    float padBottom = 16.0f * sScale;
    float padLeft = 28.0f * sScale;
    float padRight = 18.0f * sScale;
    float drawW = size.x - padLeft - padRight;
    float drawH = size.y - padBottom - 16.0f * sScale;
    float drawLeft = pos.x + padLeft;
    float drawTop = pos.y + 14.0f * sScale;
    float bottom = drawTop + drawH;

    float minFreq = 30.0f;
    float nyquist = sampleRate * 0.5f;
    float maxFreq = std::min(nyquist, 20000.0f);

    struct DbLabel { float db; const char* text; };
    DbLabel dbLabels[] = {{-12, "-12"}, {-24, "-24"}, {-48, "-48"}};
    for (auto& dbl : dbLabels) {
        float norm = (dbl.db - minDb) / (maxDb - minDb);
        float y = bottom - norm * drawH;
        dl->AddLine(ImVec2(drawLeft, y), ImVec2(drawLeft + drawW, y),
                    IM_COL32(40, 38, 34, 50));
        ImVec2 ts = MeasureText(gridFont, 12.0f, dbl.text);
        DrawTextPx(gridFont, 12.0f, ImVec2(drawLeft - ts.x - 4.0f * sScale, y - ts.y * 0.5f),
                   kGridText, dbl.text);
    }

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
        ImVec2 ts = MeasureText(gridFont, 12.0f, fl.text);
        DrawTextPx(gridFont, 12.0f, ImVec2(x - ts.x * 0.5f, bottom + 2.0f * sScale),
                   kGridText, fl.text);
    }

    if (!samples || numSamples == 0) {
        dl->PopClipRect();
        DrawTextPx(fonts().label, 14.0f,
                   ImVec2(pos.x + 10.0f * sScale, pos.y + 6.0f * sScale), kTitleDim, label);
        ImGui::Dummy(size);
        return;
    }

    static float fftBuf[kFFTSize * 2];
    int available = std::min(numSamples, kFFTSize);
    for (int i = 0; i < kFFTSize; ++i) {
        int idx = (writePos - available + i + numSamples * 2) % numSamples;
        float w = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / (kFFTSize - 1)));
        fftBuf[2 * i]     = (i < available) ? samples[idx] * w : 0.0f;
        fftBuf[2 * i + 1] = 0.0f;
    }
    simpleFFT(fftBuf, kFFTSize);

    float magDb[kBins + 1];
    for (int i = 0; i <= kBins; ++i) {
        float re = fftBuf[2 * i];
        float im = fftBuf[2 * i + 1];
        float mag = std::sqrt(re * re + im * im) / static_cast<float>(kFFTSize);
        magDb[i] = std::max(20.0f * std::log10(std::max(mag, 1e-10f)), -80.0f);
    }

    // сглаживание между кадрами — состояние per-контекст (мультиинстансы)
    struct SpecState {
        float smoothed[kBins + 1];
        bool init = false;
    };
    static std::unordered_map<ImGuiContext*, SpecState> sStates;
    auto& st = sStates[ImGui::GetCurrentContext()];
    float* smoothed = st.smoothed;
    if (!st.init) {
        for (int i = 0; i <= kBins; ++i) smoothed[i] = -80.0f;
        st.init = true;
    }
    for (int i = 0; i <= kBins; ++i) {
        if (magDb[i] > smoothed[i])
            smoothed[i] = magDb[i];
        else
            smoothed[i] = smoothed[i] * 0.82f + magDb[i] * 0.18f;
    }

    int dispW = static_cast<int>(drawW);
    float logMin = std::log2(minFreq);
    float logMax = std::log2(maxFreq);
    float hzPerBin = sampleRate / static_cast<float>(kFFTSize);

    ImVec2 prevPt(drawLeft, bottom);
    for (int px = 0; px <= dispW; ++px) {
        float t = static_cast<float>(px) / static_cast<float>(dispW);
        float freq = std::pow(2.0f, logMin + t * (logMax - logMin));
        float binF = freq / hzPerBin;

        int binLo = std::clamp(static_cast<int>(binF), 1, kBins - 1);
        int binHi = std::min(binLo + 1, kBins);
        float frac = binF - static_cast<float>(binLo);

        float db;
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
            auto freqColor = [](float freqT, float alpha) -> ImU32 {
                float r, g, b;
                if (freqT < 0.33f) {
                    float u = freqT / 0.33f;
                    r = 0.85f + u * 0.15f;
                    g = 0.30f + u * 0.50f;
                    b = 0.10f;
                } else if (freqT < 0.66f) {
                    float u = (freqT - 0.33f) / 0.33f;
                    r = 1.0f - u * 0.65f;
                    g = 0.80f + u * 0.10f;
                    b = 0.10f + u * 0.25f;
                } else {
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

            ImU32 fillBot = freqColor(t, 0.04f);
            ImU32 fillTop = freqColor(t, 0.22f);
            dl->AddQuadFilled(prevPt, curPt,
                              ImVec2(x, bottom), ImVec2(prevPt.x, bottom), fillBot);
            float midY = curPt.y + (bottom - curPt.y) * 0.35f;
            float midYprev = prevPt.y + (bottom - prevPt.y) * 0.35f;
            dl->AddQuadFilled(prevPt, curPt,
                              ImVec2(x, midY), ImVec2(prevPt.x, midYprev), fillTop);

            ImU32 lineCol = freqColor(t, 0.88f);
            dl->AddLine(prevPt, curPt, lineCol, 1.5f);
        }
        prevPt = curPt;
    }

    dl->PopClipRect();

    DrawTextPx(fonts().label, 14.0f,
               ImVec2(pos.x + 10.0f * sScale, pos.y + 6.0f * sScale), kTitleDim, label);

    ImGui::Dummy(size);
}

static bool DrawSpriteButton(const char* label, ImTextureID tex,
                              bool active, float drawW, float drawH) {
    ImVec2 pos = ImGui::GetCursorScreenPos();

    bool hasLabel = label && *label;
    ImFont* f = fonts().head;
    ImVec2 ts = hasLabel ? MeasureText(f, 19.0f, label) : ImVec2(0.0f, 0.0f);
    const float labelGap = 8.0f * sScale;
    float totalH = drawH + (hasLabel ? labelGap + ts.y : 0.0f);

    ImGui::InvisibleButton(hasLabel ? label : "##btn", ImVec2(drawW, totalH));
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (tex) {
        float v0 = active ? 0.5f : 0.0f;
        float v1 = active ? 1.0f : 0.5f;
        ImU32 tint = (!active && hovered) ? IM_COL32(230, 230, 230, 255) : kWhite;
        dl->AddImage(tex, pos, ImVec2(pos.x + drawW, pos.y + drawH),
                     ImVec2(0.0f, v0), ImVec2(1.0f, v1), tint);
    } else {
        ImU32 col = active ? IM_COL32(133, 81, 30, 255) : IM_COL32(46, 43, 38, 255);
        dl->AddRectFilled(pos, ImVec2(pos.x + drawW, pos.y + drawH), col, 3.0f * sScale);
    }

    if (hasLabel) {
        ImVec2 tp(pos.x + (drawW - ts.x) * 0.5f, pos.y + drawH + labelGap);
        ImU32 textCol = (hovered && !active) ? IM_COL32(255, 255, 255, 255)
                                             : IM_COL32(255, 255, 255, active ? 255 : 220);
        DrawTextPx(f, 19.0f, tp, textCol, label);
    }

    return clicked;
}

bool ModeSelectorButton(const char* label, bool active, float width, float height) {
    auto tex = TextureManager::get().find("mode_button");
    return DrawSpriteButton(label, tex.id, active, width, height);
}

bool ToggleButton(const char* label, bool* active, float width, float height) {
    bool changed = false;
    ImGui::PushID(label);

    auto tex = TextureManager::get().find("mode_button");
    if (DrawSpriteButton(label, tex.id, *active, width, height)) {
        *active = !(*active);
        changed = true;
    }

    ImGui::PopID();
    return changed;
}

} // namespace gui::widgets
