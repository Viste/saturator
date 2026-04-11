#include "SaturatorUI.hpp"
#include "Widgets.hpp"
#include "Locale.hpp"
#include "UpdateChecker.hpp"
#include "../PluginIds.hpp"
#include "../Version.h"
#include <imgui.h>
#include <cstdio>

namespace gui {

static constexpr int kClosedHeight = 500;
static constexpr int kOpenHeight   = 700;

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

    setLocale(state.lang);
    const auto& s = locale();

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
                    IM_COL32(195, 180, 145, 255), s.title);

        const char* brand = "v" FULL_VERSION_STR " Viste";
        ImVec2 bs = ImGui::CalcTextSize(brand);
        float brandX = pp.x + plateW - bs.x - 10;
        dl->AddText(ImVec2(brandX, pp.y + 6),
                    IM_COL32(110, 105, 92, 255), brand);

        // проверка обновлений (один раз)
        static bool updateCheckStarted = false;
        if (!updateCheckStarted) {
            UpdateChecker::checkForUpdate(FULL_VERSION_STR);
            updateCheckStarted = true;
        }

        // плашка обновления
        auto updateInfo = UpdateChecker::getUpdateInfo();
        if (updateInfo.hasUpdate) {
            const char* updateIcon = "\xe2\xac\x86"; // ⬆
            char updateLabel[64];
            std::snprintf(updateLabel, sizeof(updateLabel), "%s %s",
                          updateIcon, updateInfo.latestVersion.c_str());
            ImVec2 updateSize = ImGui::CalcTextSize(updateLabel);
            float updateX = brandX - updateSize.x - 16;
            ImVec2 updateMin(updateX - 4, pp.y + 3);
            ImVec2 updateMax(updateX + updateSize.x + 4, pp.y + 25);

            bool hovered = ImGui::IsMouseHoveringRect(updateMin, updateMax);
            ImU32 bgCol = hovered ? IM_COL32(210, 130, 50, 200)
                                  : IM_COL32(210, 130, 50, 140);
            dl->AddRectFilled(updateMin, updateMax, bgCol, 3.0f);
            dl->AddText(ImVec2(updateX, pp.y + 6),
                        IM_COL32(255, 255, 255, 255), updateLabel);

            if (hovered && ImGui::IsMouseClicked(0) && !updateInfo.downloadUrl.empty()) {
                UpdateChecker::openInBrowser(updateInfo.downloadUrl.c_str());
            }
        }

        // RU / EN toggle
        float langX = updateInfo.hasUpdate
            ? brandX - 52 - ImGui::CalcTextSize(updateInfo.latestVersion.c_str()).x - 30
            : brandX - 52;
        bool isRu = (state.lang == Lang::RU);
        ImU32 ruCol = isRu ? IM_COL32(195, 180, 145, 255) : IM_COL32(80, 76, 68, 255);
        ImU32 enCol = isRu ? IM_COL32(80, 76, 68, 255) : IM_COL32(195, 180, 145, 255);
        dl->AddText(ImVec2(langX, pp.y + 6), ruCol, "RU");
        dl->AddText(ImVec2(langX + 18, pp.y + 6), IM_COL32(60, 57, 50, 255), "/");
        dl->AddText(ImVec2(langX + 26, pp.y + 6), enCol, "EN");

        ImVec2 langMin(langX, pp.y);
        ImVec2 langMax(langX + 44, pp.y + 28);
        if (ImGui::IsMouseHoveringRect(langMin, langMax) && ImGui::IsMouseClicked(0)) {
            state.lang = isRu ? Lang::EN : Lang::RU;
        }
    }

    float modeY = pad + 34;
    const char* modeNames[] = {
        s.modeInstrument, s.modeDrums, s.modeVocal
    };
    int mode = state.currentMode();
    float modeW = 110 * 3 + 3 * 2;
    float modeCenterX = (ws.x - modeW) * 0.5f;
    ImGui::SetCursorPos(ImVec2(modeCenterX, modeY));
    if (widgets::ModeSelector("Mode", &mode, modeNames, 3)) {
        setParam(state, kMode, static_cast<float>(mode) / 2.0f);
    }

    // кнопки ADV и CLIPPER — ширина по тексту
    float advBtnW = ImGui::CalcTextSize(s.advToggle).x + 20;
    float clipBtnW = ImGui::CalcTextSize(s.clipLabel).x + 20;

    float advX = modeCenterX + modeW + 10;
    ImGui::SetCursorPos(ImVec2(advX, modeY));
    bool prevAdv = state.advancedOpen;
    widgets::ToggleButton(s.advToggle, &state.advancedOpen, advBtnW, 28.0f);

    if (state.advancedOpen != prevAdv) {
        state.requestedHeight = state.advancedOpen ? kOpenHeight : kClosedHeight;
    }

    // CLIP toggle (рядом с ADV)
    float clipToggleX = advX + advBtnW + 6;
    ImGui::SetCursorPos(ImVec2(clipToggleX, modeY));
    bool clipOn = (getParam(state, kClipEnabled) > 0.5f);
    bool prevClip = clipOn;
    widgets::ToggleButton(s.clipLabel, &clipOn, clipBtnW, 28.0f);
    if (clipOn != prevClip) {
        setParam(state, kClipEnabled, clipOn ? 1.0f : 0.0f);
    }

    float ctrlY = modeY + 34;
    float knobSize = 90.0f;

    float driveX = pad + 20;
    ImGui::SetCursorPos(ImVec2(driveX, ctrlY));
    float saturation = getParam(state, kSaturation);
    if (widgets::Knob(s.drive, &saturation, 0.0f, 1.0f, 0.0f, knobSize, true)) {
        setParam(state, kSaturation, saturation);
    }

    float mixX = driveX + knobSize + 20;
    ImGui::SetCursorPos(ImVec2(mixX, ctrlY));
    float dryWet = getParam(state, kDryWet);
    if (widgets::Knob(s.mix, &dryWet, 0.0f, 1.0f, 1.0f, knobSize, true)) {
        setParam(state, kDryWet, dryWet);
    }

    // CLIP knob — показываем только когда клипер включён
    float meterX;
    if (clipOn) {
        float clipKnobX = mixX + knobSize + 20;
        ImGui::SetCursorPos(ImVec2(clipKnobX, ctrlY));
        float clipAmount = getParam(state, kClipAmount);
        if (widgets::Knob(s.clipLabel, &clipAmount, 0.0f, 1.0f, 0.0f, knobSize, true)) {
            setParam(state, kClipAmount, clipAmount);
        }
        meterX = clipKnobX + knobSize + 20;
    } else {
        meterX = mixX + knobSize + 20;
    }
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

        dl->AddText(ImVec2(meterX - 4, ctrlY + meterH + 2),
                    IM_COL32(110, 105, 92, 255), s.inOut);
    }

    // осциллограмма
    float wfSmallX = meterX + 60;
    float wfSmallW = ws.x - wfSmallX - pad;
    if (wfSmallW < 80) wfSmallW = 80;
    float wfSmallH = knobSize + 20;
    ImGui::SetCursorPos(ImVec2(wfSmallX, ctrlY));
    if (state.metering) {
        widgets::WaveformOverlay(s.signal,
            state.metering->inputWaveform.samples.data(),
            state.metering->outputWaveform.samples.data(),
            dsp::MeteringData::kWaveformSize,
            state.metering->inputWaveform.writePos.load(std::memory_order_relaxed),
            state.metering->outputWaveform.writePos.load(std::memory_order_relaxed),
            ImVec2(wfSmallW, wfSmallH));
    } else {
        widgets::WaveformOverlay(s.signal,
            nullptr, nullptr, 0, 0, 0, ImVec2(wfSmallW, wfSmallH));
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
            float startX = pad + 20;

            ImGui::SetCursorPos(ImVec2(startX, knobY));
            float low = getParam(state, kInstLowSat);
            if (widgets::Knob(s.instLow, &low, 0.0f, 1.0f, 0.55f, advKnob, true)) {
                setParam(state, kInstLowSat, low);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing, knobY));
            float mid = getParam(state, kInstMidSat);
            if (widgets::Knob(s.instMid, &mid, 0.0f, 1.0f, 0.7f, advKnob, true)) {
                setParam(state, kInstMidSat, mid);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 2, knobY));
            float high = getParam(state, kInstHighSat);
            if (widgets::Knob(s.instHigh, &high, 0.0f, 1.0f, 0.35f, advKnob, true)) {
                setParam(state, kInstHighSat, high);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 3, knobY));
            float character = getParam(state, kInstCharacter);
            if (widgets::Knob(s.instChar, &character, 0.0f, 1.0f, 0.6f, advKnob, true)) {
                setParam(state, kInstCharacter, character);
            }

            float descX = startX + knobSpacing * 4 + 10;
            dl->AddText(ImVec2(descX, advPanelY + 6),
                        IM_COL32(110, 105, 92, 255), s.instDesc1);
            dl->AddText(ImVec2(descX, advPanelY + 22),
                        IM_COL32(80, 76, 68, 255), s.instDesc2);
            dl->AddText(ImVec2(descX, advPanelY + 38),
                        IM_COL32(80, 76, 68, 255), s.instDesc3);
            dl->AddText(ImVec2(descX, advPanelY + 54),
                        IM_COL32(65, 62, 55, 255),
                        "y = lerp(tanh, tanh+T2, char)");
        } else if (mode == 1) {
            float startX = pad + 20;

            ImGui::SetCursorPos(ImVec2(startX, knobY));
            float sens = getParam(state, kDrumTransientSens);
            if (widgets::Knob(s.drumSens, &sens, 0.0f, 1.0f, 0.6f, advKnob, true)) {
                setParam(state, kDrumTransientSens, sens);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing, knobY));
            float punch = getParam(state, kDrumPunch);
            if (widgets::Knob(s.drumPunch, &punch, 0.0f, 1.0f, 0.6f, advKnob, true)) {
                setParam(state, kDrumPunch, punch);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 2, knobY));
            float sustain = getParam(state, kDrumSustainSat);
            if (widgets::Knob(s.drumSustain, &sustain, 0.0f, 1.0f, 0.5f, advKnob, true)) {
                setParam(state, kDrumSustainSat, sustain);
            }

            float descX = startX + knobSpacing * 3 + 10;
            dl->AddText(ImVec2(descX, advPanelY + 6),
                        IM_COL32(110, 105, 92, 255), s.drumDesc1);
            dl->AddText(ImVec2(descX, advPanelY + 22),
                        IM_COL32(80, 76, 68, 255), s.drumDesc2);
            dl->AddText(ImVec2(descX, advPanelY + 38),
                        IM_COL32(80, 76, 68, 255), s.drumDesc3);
            dl->AddText(ImVec2(descX, advPanelY + 54),
                        IM_COL32(65, 62, 55, 255),
                        "y = lerp(sat, dry*boost, punch*gate\xc2\xb2)");
        }
        else {
            float startX = pad + 20;

            ImGui::SetCursorPos(ImVec2(startX, knobY));
            float bias = getParam(state, kTapeBias);
            if (widgets::Knob(s.vocalBias, &bias, 0.0f, 1.0f, 0.3f, advKnob, true)) {
                setParam(state, kTapeBias, bias);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing, knobY));
            float wow = getParam(state, kTapeWow);
            if (widgets::Knob(s.vocalWow, &wow, 0.0f, 1.0f, 0.15f, advKnob, true)) {
                setParam(state, kTapeWow, wow);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 2, knobY));
            float flutter = getParam(state, kTapeFlutter);
            if (widgets::Knob(s.vocalFlutter, &flutter, 0.0f, 1.0f, 0.1f, advKnob, true)) {
                setParam(state, kTapeFlutter, flutter);
            }

            ImGui::SetCursorPos(ImVec2(startX + knobSpacing * 3, knobY));
            float cutoff = getParam(state, kTapeHeadCutoff);
            if (widgets::Knob(s.vocalCutoff, &cutoff, 0.0f, 1.0f, 0.75f, advKnob, true)) {
                setParam(state, kTapeHeadCutoff, cutoff);
            }

            float descX = startX + knobSpacing * 4 + 10;
            dl->AddText(ImVec2(descX, advPanelY + 6),
                        IM_COL32(110, 105, 92, 255), s.vocalDesc1);
            dl->AddText(ImVec2(descX, advPanelY + 22),
                        IM_COL32(80, 76, 68, 255), s.vocalDesc2);
            dl->AddText(ImVec2(descX, advPanelY + 38),
                        IM_COL32(80, 76, 68, 255), s.vocalDesc3);
            dl->AddText(ImVec2(descX, advPanelY + 54),
                        IM_COL32(80, 76, 68, 255), s.vocalDesc4);
            dl->AddText(ImVec2(descX, advPanelY + 70),
                        IM_COL32(65, 62, 55, 255),
                        "y = tanh(d*(x + fb*state))");
        }

        // селектор оверсемплинга — глобальный (—/2x/4x)
        {
            float osX = ws.x - pad - 140;
            float osY = advPanelY + advPanelH - 34;

            dl->AddText(ImVec2(osX - 22, osY + 7),
                        IM_COL32(80, 76, 68, 255), s.osLabel);

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

    // СПЕКТР
    float specY = advPanelY + (state.advancedOpen ? advPanelH + 8 : 0);
    float specH = ws.y - specY - pad;
    if (specH < 40) specH = 40;
    float specW = ws.x - pad * 2;

    ImGui::SetCursorPos(ImVec2(pad, specY));
    if (state.metering) {
        float sr = state.metering->sampleRate.load(std::memory_order_relaxed);
        widgets::SpectrumDisplay(s.spectrum,
            state.metering->outputWaveform.samples.data(),
            dsp::MeteringData::kWaveformSize,
            state.metering->outputWaveform.writePos.load(std::memory_order_relaxed),
            ImVec2(specW, specH), sr);
    } else {
        widgets::SpectrumDisplay(s.spectrum,
            nullptr, 0, 0, ImVec2(specW, specH));
    }

    ImGui::End();
}

} // namespace gui
