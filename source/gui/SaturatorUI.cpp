#include "SaturatorUI.hpp"
#include "Widgets.hpp"
#include "../PluginIds.hpp"
#include <imgui.h>
#include <cstdio>

namespace gui {

static constexpr int kClosedHeight = 390;
static constexpr int kOpenHeight   = 500;

static float getParam(UIState& state, Steinberg::Vst::ParamID id) {
    if (!state.controller) return 0.0f;
    return static_cast<float>(state.controller->getParamNormalized(id));
}

static void setParam(UIState& state, Steinberg::Vst::ParamID id, float value) {
    if (!state.controller) return;
    state.controller->beginEdit(id);
    state.controller->setParamNormalized(id, value);
    state.controller->performEdit(id, value);
    state.controller->endEdit(id);
}

void SaturatorUI::render(UIState& state) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 ws = io.DisplaySize;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ws);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Saturator", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                 ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);

    // запрет скролла — иначе хитбоксы смещаются относительно drawlist
    ImGui::SetScrollY(0);

    widgets::DrawBackground(ImVec2(0, 0), ws);

    float pad = 22.0f;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    {
        float plateW = ws.x - pad * 2;
        ImVec2 pp(pad, pad);
        dl->AddRectFilled(pp, ImVec2(pp.x + plateW, pp.y + 28),
                          IM_COL32(48, 45, 40, 255), 2.0f);
        dl->AddRect(pp, ImVec2(pp.x + plateW, pp.y + 28),
                    IM_COL32(65, 62, 55, 255), 2.0f, 0, 1.0f);

        dl->AddText(ImVec2(pp.x + 10, pp.y + 6),
                    IM_COL32(195, 180, 145, 255), "\xd0\xa1\xd0\x90\xd0\xa2\xd0\xa3\xd0\xa0\xd0\x90\xd0\xa2\xd0\x9e\xd0\xa0");

        const char* brand = "v2.0  VISTE";
        ImVec2 bs = ImGui::CalcTextSize(brand);
        dl->AddText(ImVec2(pp.x + plateW - bs.x - 10, pp.y + 6),
                    IM_COL32(110, 105, 92, 255), brand);
    }

    float modeY = pad + 34;
    // ИНСТРУМЕНТ, УДАРНЫЕ, ВОКАЛ/ЛЕНТА
    static const char* modeNames[] = {
        "\xd0\x98\xd0\x9d\xd0\xa1\xd0\xa2\xd0\xa0\xd0\xa3\xd0\x9c\xd0\x95\xd0\x9d\xd0\xa2",
        "\xd0\xa3\xd0\x94\xd0\x90\xd0\xa0\xd0\x9d\xd0\xab\xd0\x95",
        "\xd0\x92\xd0\x9e\xd0\x9a\xd0\x90\xd0\x9b/\xd0\x9b\xd0\x95\xd0\x9d\xd0\xa2\xd0\x90"
    };
    int mode = state.currentMode();
    float modeW = 110 * 3 + 3 * 2;
    float modeCenterX = (ws.x - modeW) * 0.5f;
    ImGui::SetCursorPos(ImVec2(modeCenterX, modeY));
    if (widgets::ModeSelector("Mode", &mode, modeNames, 3)) {
        setParam(state, kMode, static_cast<float>(mode) / 2.0f);
    }

    // ДОП
    float advX = modeCenterX + modeW + 10;
    ImGui::SetCursorPos(ImVec2(advX, modeY));
    bool prevAdv = state.advancedOpen;
    widgets::ToggleButton("\xd0\x94\xd0\x9e\xd0\x9f", &state.advancedOpen, 50.0f, 28.0f);

    if (state.advancedOpen != prevAdv) {
        state.requestedHeight = state.advancedOpen ? kOpenHeight : kClosedHeight;
    }

    float ctrlY = modeY + 34;
    float knobSize = 90.0f;

    // ДРАЙВ
    float driveX = pad + 20;
    ImGui::SetCursorPos(ImVec2(driveX, ctrlY));
    float saturation = getParam(state, kSaturation);
    if (widgets::Knob("\xd0\x94\xd0\xa0\xd0\x90\xd0\x99\xd0\x92", &saturation, 0.0f, 1.0f, 0.0f, knobSize, true)) {
        setParam(state, kSaturation, saturation);
    }

    // МИКС
    float mixX = driveX + knobSize + 30;
    ImGui::SetCursorPos(ImVec2(mixX, ctrlY));
    float dryWet = getParam(state, kDryWet);
    if (widgets::Knob("\xd0\x9c\xd0\x98\xd0\x9a\xd0\xa1", &dryWet, 0.0f, 1.0f, 1.0f, knobSize, true)) {
        setParam(state, kDryWet, dryWet);
    }

    float meterX = mixX + knobSize + 20;
    float meterH = knobSize;
    {
        float inLvl = 0.0f, outLvl = 0.0f;
        if (state.metering) {
            inLvl = state.metering->inputPeak.load(std::memory_order_relaxed);
            outLvl = state.metering->outputPeak.load(std::memory_order_relaxed);
            state.metering->decayPeaks();
        }
        ImGui::SetCursorPos(ImVec2(meterX, ctrlY));
        widgets::LevelMeter("In", inLvl, 10.0f, meterH);

        ImGui::SetCursorPos(ImVec2(meterX + 16, ctrlY));
        widgets::LevelMeter("Out", outLvl, 10.0f, meterH);

        // Вх Вых
        dl->AddText(ImVec2(meterX - 4, ctrlY + meterH + 2),
                    IM_COL32(110, 105, 92, 255),
                    "\xd0\x92\xd1\x85 \xd0\x92\xd1\x8b\xd1\x85");
    }

    float specX = meterX + 40;
    float specW = ws.x - specX - pad;
    if (specW < 80) specW = 80;
    float specH = knobSize + 20;
    ImGui::SetCursorPos(ImVec2(specX, ctrlY));
    if (state.metering) {
        widgets::SpectrumDisplay("\xd0\xa1\xd0\x9f\xd0\x95\xd0\x9a\xd0\xa2\xd0\xa0",
            state.metering->outputWaveform.samples.data(),
            dsp::MeteringData::kWaveformSize,
            state.metering->outputWaveform.writePos.load(std::memory_order_relaxed),
            ImVec2(specW, specH));
    } else {
        widgets::SpectrumDisplay("\xd0\xa1\xd0\x9f\xd0\x95\xd0\x9a\xd0\xa2\xd0\xa0",
            nullptr, 0, 0, ImVec2(specW, specH));
    }

    float advPanelH = 0.0f;
    float advPanelY = ctrlY + knobSize + 36;

    if (state.advancedOpen) {
        advPanelH = 104.0f;
        float advKnob = 60.0f;

        ImVec2 panelPos(pad, advPanelY);
        ImVec2 panelEnd(ws.x - pad, advPanelY + advPanelH);
        dl->AddRectFilled(panelPos, panelEnd, IM_COL32(28, 26, 23, 255), 4.0f);
        dl->AddRect(panelPos, panelEnd, IM_COL32(50, 47, 42, 255), 4.0f, 0, 1.0f);

        float knobY = advPanelY + 4.0f;
        float knobSpacing = advKnob + 16.0f;

        if (mode == 0) {
            // инструмент: НЧ, СЧ, ВЧ, ТЕМБР
            float startX = pad + 20;

            ImGui::SetCursorPos(ImVec2(startX, knobY));
            float low = getParam(state, kInstLowSat);
            if (widgets::Knob("\xd0\x9d\xd0\xa7", &low, 0.0f, 1.0f, 0.55f, advKnob, true)) {
                setParam(state, kInstLowSat, low);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing, knobY));
            float mid = getParam(state, kInstMidSat);
            if (widgets::Knob("\xd0\xa1\xd0\xa7", &mid, 0.0f, 1.0f, 0.7f, advKnob, true)) {
                setParam(state, kInstMidSat, mid);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 2, knobY));
            float high = getParam(state, kInstHighSat);
            if (widgets::Knob("\xd0\x92\xd0\xa7", &high, 0.0f, 1.0f, 0.35f, advKnob, true)) {
                setParam(state, kInstHighSat, high);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 3, knobY));
            float character = getParam(state, kInstCharacter);
            if (widgets::Knob("\xd0\xa2\xd0\x95\xd0\x9c\xd0\x91\xd0\xa0", &character, 0.0f, 1.0f, 0.6f, advKnob, true)) {
                setParam(state, kInstCharacter, character);
            }

            float descX = startX + knobSpacing * 4 + 10;
            dl->AddText(ImVec2(descX, advPanelY + 6),
                        IM_COL32(110, 105, 92, 255),
                        "LR4 crossover + tube sat");
            dl->AddText(ImVec2(descX, advPanelY + 22),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\x9d\xd0\xa7/\xd0\xa1\xd0\xa7/\xd0\x92\xd0\xa7: \xd1\x81\xd0\xb8\xd0\xbb\xd0\xb0 \xd0\xbf\xd0\xbe \xd0\xbf\xd0\xbe\xd0\xbb\xd0\xbe\xd1\x81\xd0\xb0\xd0\xbc");
            dl->AddText(ImVec2(descX, advPanelY + 38),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\xa2\xd0\x95\xd0\x9c\xd0\x91\xd0\xa0: \xd1\x87\xd0\xb5\xd1\x82/\xd0\xbd\xd0\xb5\xd1\x87\xd0\xb5\xd1\x82 \xd0\xb3\xd0\xb0\xd1\x80\xd0\xbc\xd0\xbe\xd0\xbd\xd0\xb8\xd0\xba\xd0\xb8");
            dl->AddText(ImVec2(descX, advPanelY + 54),
                        IM_COL32(65, 62, 55, 255),
                        "y = tube(x*d) + T2..T5");
        } else if (mode == 1) {
            // ударные: ЧУВСТ, УДАР, СУСТЕЙН
            float startX = pad + 20;

            ImGui::SetCursorPos(ImVec2(startX, knobY));
            float sens = getParam(state, kDrumTransientSens);
            if (widgets::Knob("\xd0\xa7\xd0\xa3\xd0\x92\xd0\xa1\xd0\xa2", &sens, 0.0f, 1.0f, 0.6f, advKnob, true)) {
                setParam(state, kDrumTransientSens, sens);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing, knobY));
            float punch = getParam(state, kDrumPunch);
            if (widgets::Knob("\xd0\xa3\xd0\x94\xd0\x90\xd0\xa0", &punch, 0.0f, 1.0f, 0.6f, advKnob, true)) {
                setParam(state, kDrumPunch, punch);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 2, knobY));
            float sustain = getParam(state, kDrumSustainSat);
            if (widgets::Knob("\xd0\xa1\xd0\xa3\xd0\xa1\xd0\xa2", &sustain, 0.0f, 1.0f, 0.5f, advKnob, true)) {
                setParam(state, kDrumSustainSat, sustain);
            }

            float descX = startX + knobSpacing * 3 + 10;
            dl->AddText(ImVec2(descX, advPanelY + 6),
                        IM_COL32(110, 105, 92, 255),
                        "transient + sustain split");
            dl->AddText(ImVec2(descX, advPanelY + 22),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\xa7\xd0\xa3\xd0\x92\xd0\xa1\xd0\xa2: \xd0\xbf\xd0\xbe\xd1\x80\xd0\xbe\xd0\xb3 \xd1\x82\xd1\x80\xd0\xb0\xd0\xbd\xd0\xb7.");
            dl->AddText(ImVec2(descX, advPanelY + 38),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\xa3\xd0\x94\xd0\x90\xd0\xa0: \xd0\xb0\xd0\xba\xd1\x86\xd0\xb5\xd0\xbd\xd1\x82, "
                        "\xd0\xa1\xd0\xa3\xd0\xa1\xd0\xa2: \xd1\x82\xd0\xb5\xd0\xbb\xd0\xbe");
            dl->AddText(ImVec2(descX, advPanelY + 54),
                        IM_COL32(65, 62, 55, 255),
                        "y = lerp(sat, clean, gate)");
        }
        else {
            // вокал/лента: СМЕЩ, ВАУ, ДЕТОН, СРЕЗ
            float startX = pad + 20;

            ImGui::SetCursorPos(ImVec2(startX, knobY));
            float bias = getParam(state, kTapeBias);
            if (widgets::Knob("\xd0\xa1\xd0\x9c\xd0\x95\xd0\xa9", &bias, 0.0f, 1.0f, 0.3f, advKnob, true)) {
                setParam(state, kTapeBias, bias);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing, knobY));
            float wow = getParam(state, kTapeWow);
            if (widgets::Knob("\xd0\x92\xd0\x90\xd0\xa3", &wow, 0.0f, 1.0f, 0.15f, advKnob, true)) {
                setParam(state, kTapeWow, wow);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 2, knobY));
            float flutter = getParam(state, kTapeFlutter);
            if (widgets::Knob("\xd0\x94\xd0\x95\xd0\xa2\xd0\x9e\xd0\x9d", &flutter, 0.0f, 1.0f, 0.1f, advKnob, true)) {
                setParam(state, kTapeFlutter, flutter);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 3, knobY));
            float cutoff = getParam(state, kTapeHeadCutoff);
            if (widgets::Knob("\xd0\xa1\xd0\xa0\xd0\x95\xd0\x97", &cutoff, 0.0f, 1.0f, 0.75f, advKnob, true)) {
                setParam(state, kTapeHeadCutoff, cutoff);
            }

            float descX = startX + knobSpacing * 4 + 10;
            dl->AddText(ImVec2(descX, advPanelY + 6),
                        IM_COL32(110, 105, 92, 255),
                        "tape hysteresis + LFO");
            dl->AddText(ImVec2(descX, advPanelY + 22),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\xa1\xd0\x9c\xd0\x95\xd0\xa9: \xd1\x82\xd0\xbe\xd1\x87\xd0\xba\xd0\xb0 \xd0\xbb\xd0\xb5\xd0\xbd\xd1\x82\xd1\x8b");
            dl->AddText(ImVec2(descX, advPanelY + 38),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\x92\xd0\x90\xd0\xa3/\xd0\x94\xd0\x95\xd0\xa2: \xd0\xbc\xd0\xbe\xd0\xb4. \xd1\x81\xd0\xba\xd0\xbe\xd1\x80\xd0\xbe\xd1\x81\xd1\x82\xd0\xb8");
            dl->AddText(ImVec2(descX, advPanelY + 54),
                        IM_COL32(80, 76, 68, 255),
                        "\xd0\xa1\xd0\xa0\xd0\x95\xd0\x97: \xd1\x84\xd0\xb8\xd0\xbb\xd1\x8c\xd1\x82\xd1\x80 \xd0\xb3\xd0\xbe\xd0\xbb\xd0\xbe\xd0\xb2\xd0\xba\xd0\xb8");
            dl->AddText(ImVec2(descX, advPanelY + 70),
                        IM_COL32(65, 62, 55, 255),
                        "y = hyst(x, bias, fb)");
        }

        // селектор оверсемплинга — глобальный (—/2x/4x)
        {
            float osX = ws.x - pad - 140;
            float osY = advPanelY + advPanelH - 34;

            dl->AddText(ImVec2(osX - 22, osY + 7),
                        IM_COL32(80, 76, 68, 255), "\xd0\x9e\xd0\xa1");

            float osNorm = getParam(state, kOversampling);
            int osMode = static_cast<int>(osNorm * 2.0f + 0.5f);

            static const char* osLabels[] = {
                "\xe2\x80\x94",
                "2x",
                "4x"
            };

            ImGui::PushID("OsSelector");
            for (int oi = 0; oi < 3; ++oi) {
                bool osSel = (osMode == oi);
                if (osSel) {
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

                ImGui::SetCursorPos(ImVec2(osX + oi * 38, osY));
                ImGui::PushID(oi);
                if (ImGui::Button(osLabels[oi], ImVec2(36, 24))) {
                    osMode = oi;
                    setParam(state, kOversampling, static_cast<float>(oi) / 2.0f);
                }
                ImGui::PopID();

                ImGui::PopStyleColor(4);
                ImGui::PopStyleVar(2);
            }
            ImGui::PopID();
        }
    }

    // СИГНАЛ
    float wfY = advPanelY + (state.advancedOpen ? advPanelH + 8 : 0);
    float wfH = ws.y - wfY - pad;
    if (wfH < 40) wfH = 40;
    float wfW = ws.x - pad * 2;

    ImGui::SetCursorPos(ImVec2(pad, wfY));
    if (state.metering) {
        widgets::WaveformOverlay("\xd0\xa1\xd0\x98\xd0\x93\xd0\x9d\xd0\x90\xd0\x9b",
            state.metering->inputWaveform.samples.data(),
            state.metering->outputWaveform.samples.data(),
            dsp::MeteringData::kWaveformSize,
            state.metering->inputWaveform.writePos.load(std::memory_order_relaxed),
            state.metering->outputWaveform.writePos.load(std::memory_order_relaxed),
            ImVec2(wfW, wfH));
    } else {
        widgets::WaveformOverlay("\xd0\xa1\xd0\x98\xd0\x93\xd0\x9d\xd0\x90\xd0\x9b",
            nullptr, nullptr, 0, 0, 0, ImVec2(wfW, wfH));
    }

    ImGui::End();
}

} // namespace gui
