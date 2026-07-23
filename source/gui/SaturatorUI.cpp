#include "SaturatorUI.hpp"
#include "Widgets.hpp"
#include "Fonts.hpp"
#include "Locale.hpp"
#include "UpdateChecker.hpp"
#include "TextureManager.hpp"
#include "../PluginIds.hpp"
#include "../Version.h"
#include <imgui.h>
#include <algorithm>

namespace gui {

static constexpr float kBaseW       = 900.0f;
static constexpr float kBaseHClosed = 500.0f;
static constexpr float kBaseHOpen   = 700.0f;

static constexpr ImU32 kPanelFill = IM_COL32(10, 9, 8, 255);      // #0a0908
static constexpr ImU32 kAccent    = IM_COL32(72, 74, 66, 255);    // #484a42
static constexpr ImU32 kLabelGray = IM_COL32(148, 148, 148, 255); // #949494
static constexpr ImU32 kTabFill   = IM_COL32(34, 34, 34, 255);    // #222222
static constexpr ImU32 kWhite     = IM_COL32(255, 255, 255, 255);

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
    const Fonts F = fonts();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float baseH = state.advancedOpen ? kBaseHOpen : kBaseHClosed;
    const float scale = std::max(0.5f, std::min(ws.x / kBaseW, ws.y / baseH));
    widgets::SetUIScale(scale);
    auto S = [scale](float v) { return v * scale; };

    const float panelRight = ws.x - S(24.0f);

    {
        auto logoTex = TextureManager::get().find("logo");
        if (logoTex.id != 0) {
            dl->AddImage(logoTex.id, ImVec2(S(12), S(12)),
                         ImVec2(S(12 + 193), S(12 + 69)));
        }

        const char* brand = "v" FULL_VERSION_STR " Viste";
        widgets::DrawTextPx(F.head, 14.5f, ImVec2(S(111), S(62)), kWhite, brand);

        static bool updateCheckStarted = false;
        if (!updateCheckStarted) {
            UpdateChecker::checkForUpdate(FULL_VERSION_STR);
            updateCheckStarted = true;
        }
        auto updateInfo = UpdateChecker::getUpdateInfo();
        if (updateInfo.hasUpdate) {
            auto upTex = TextureManager::get().find("update_button");
            ImVec2 upMin(S(181), S(62));
            ImVec2 upMax(S(181 + 17), S(62 + 17));
            bool hovered = ImGui::IsMouseHoveringRect(upMin, upMax);
            if (upTex.id != 0) {
                ImU32 tint = hovered ? kWhite : IM_COL32(220, 220, 220, 255);
                dl->AddImage(upTex.id, upMin, upMax, ImVec2(0, 0), ImVec2(1, 1), tint);
            }
            if (hovered && ImGui::IsMouseClicked(0) && !updateInfo.downloadUrl.empty()) {
                UpdateChecker::openInBrowser(updateInfo.downloadUrl.c_str());
            }
        }
    }

    {
        ImVec2 frameMin(S(230), S(20.5f));
        ImVec2 frameMax(S(230 + 53), S(20.5f + 52));
        dl->AddRectFilled(frameMin, frameMax, IM_COL32(74, 74, 74, 26), S(5));
        dl->AddRect(frameMin, frameMax, IM_COL32(23, 23, 23, 255), S(5), 0,
                    std::max(1.0f, S(1)));

        ImVec2 langMin(S(239), S(29.5f));
        ImVec2 langMax(S(239 + 36), S(29.5f + 19));
        bool isRu = (state.lang == Lang::RU);

        auto langTex = TextureManager::get().find("lang_switch");
        if (langTex.id != 0) {
            float v0 = isRu ? 0.0f : 0.5f;
            float v1 = isRu ? 0.5f : 1.0f;
            dl->AddImage(langTex.id, langMin, langMax, ImVec2(0.0f, v0), ImVec2(1.0f, v1));
        } else {
            dl->AddRectFilled(langMin, langMax, IM_COL32(40, 38, 34, 255), S(3));
            ImU32 ruCol = isRu ? IM_COL32(240, 133, 66, 255) : IM_COL32(110, 105, 92, 255);
            ImU32 enCol = isRu ? IM_COL32(110, 105, 92, 255) : IM_COL32(240, 133, 66, 255);
            widgets::DrawTextPx(F.head, 10.0f, ImVec2(langMin.x + S(3), langMin.y + S(3)), ruCol, "RU");
            widgets::DrawTextPx(F.head, 10.0f, ImVec2(langMin.x + S(19), langMin.y + S(3)), enCol, "EN");
        }

        const char* langLbl = "RU/EN";
        ImVec2 lblSz = widgets::MeasureText(F.head, 14.5f, langLbl);
        widgets::DrawTextPx(F.head, 14.5f,
                            ImVec2(S(230) + (S(53) - lblSz.x) * 0.5f, S(52.0f)),
                            kWhite, langLbl);

        if (ImGui::IsMouseHoveringRect(frameMin, frameMax) && ImGui::IsMouseClicked(0)) {
            float mx = ImGui::GetMousePos().x;
            state.lang = (mx < langMin.x + S(18)) ? Lang::RU : Lang::EN;
        }
    }

    int mode = state.currentMode();
    bool clipOn = (getParam(state, kClipEnabled) > 0.5f);
    {
        const float kModeBtnX[4] = { S(310), S(450), S(590), S(730) };
        const float kModeY = S(25);
        const float kBtnW = S(136);
        const float kBtnH = S(24);
        const char* modeNames[] = { s.modeInstrument, s.modeDrums, s.modeVocal };

        ImGui::PushID("ModeRow");
        for (int i = 0; i < 3; ++i) {
            ImGui::SetCursorPos(ImVec2(kModeBtnX[i], kModeY));
            ImGui::PushID(i);
            if (widgets::ModeSelectorButton(modeNames[i], mode == i, kBtnW, kBtnH)) {
                if (mode != i) {
                    mode = i;
                    setParam(state, kMode, static_cast<float>(i) / 2.0f);
                }
            }
            ImGui::PopID();
        }
        ImGui::SetCursorPos(ImVec2(kModeBtnX[3], kModeY));
        bool prevClip = clipOn;
        widgets::ToggleButton(s.clipLabel, &clipOn, kBtnW, kBtnH);
        if (clipOn != prevClip) {
            setParam(state, kClipEnabled, clipOn ? 1.0f : 0.0f);
        }
        ImGui::PopID();
    }

    bool prevAdv = state.advancedOpen;

    const float kKnobsY = S(87);
    const float kKnobBig = S(90);

    ImGui::SetCursorPos(ImVec2(S(35), kKnobsY));
    float saturation = getParam(state, kSaturation);
    if (widgets::Knob(s.drive, &saturation, 0.0f, 1.0f, 0.0f, kKnobBig, true)) {
        setParam(state, kSaturation, saturation);
    }

    ImGui::SetCursorPos(ImVec2(S(143), kKnobsY));
    float dryWet = getParam(state, kDryWet);
    if (widgets::Knob(s.mix, &dryWet, 0.0f, 1.0f, 1.0f, kKnobBig, true)) {
        setParam(state, kDryWet, dryWet);
    }

    float meterX, signalX;
    if (clipOn) {
        ImGui::SetCursorPos(ImVec2(S(251), kKnobsY));
        float clipAmount = getParam(state, kClipAmount);
        if (widgets::Knob(s.clipLabel, &clipAmount, 0.0f, 1.0f, 0.5f, kKnobBig, true)) {
            setParam(state, kClipAmount, clipAmount);
        }
        meterX  = S(359);
        signalX = S(403);
    } else {
        meterX  = S(251);
        signalX = S(295);
    }

    {
        float inLvl = 0.0f, outLvl = 0.0f;
        if (state.metering) {
            inLvl = state.metering->inputPeak.load(std::memory_order_relaxed);
            outLvl = state.metering->outputPeak.load(std::memory_order_relaxed);
            state.metering->decayPeaks();
        }
        ImGui::SetCursorPos(ImVec2(meterX, kKnobsY));
        widgets::InOutMeter(inLvl, outLvl, S(32), S(100));
    }

    {
        float signalW = std::max(panelRight - signalX, S(80));
        ImGui::SetCursorPos(ImVec2(signalX, kKnobsY));
        if (state.metering) {
            widgets::WaveformOverlay(s.signal,
                state.metering->inputWaveform.samples.data(),
                state.metering->outputWaveform.samples.data(),
                dsp::MeteringData::kWaveformSize,
                state.metering->inputWaveform.writePos.load(std::memory_order_relaxed),
                state.metering->outputWaveform.writePos.load(std::memory_order_relaxed),
                ImVec2(signalW, S(114)));
        } else {
            widgets::WaveformOverlay(s.signal,
                nullptr, nullptr, 0, 0, 0, ImVec2(signalW, S(114)));
        }
    }

    if (state.advancedOpen != prevAdv) {
        float widthScale = ws.x / kBaseW;
        state.requestedHeight = static_cast<int>(
            (state.advancedOpen ? kBaseHOpen : kBaseHClosed) * widthScale);
    }

    const float advPanelY = S(215);
    const float advPanelH = S(123);
    if (state.advancedOpen) {
        ImVec2 panelMin(S(24), advPanelY);
        ImVec2 panelMax(panelRight, advPanelY + advPanelH);
        float border = std::max(1.0f, S(2));

        dl->AddRectFilled(panelMin, panelMax, kPanelFill);
        dl->AddRect(panelMin, panelMax, kAccent, 0.0f, 0, border);

        {
            ImVec2 ts = widgets::MeasureText(F.head, 19.0f, s.advToggle);
            float tx = (ws.x - ts.x) * 0.5f;
            dl->AddRectFilled(ImVec2(tx - S(6), advPanelY),
                              ImVec2(tx + ts.x + S(6), advPanelY + S(8)), kPanelFill);
            widgets::DrawTextPx(F.head, 19.0f, ImVec2(tx, S(206)), kWhite, s.advToggle);
        }

        const float advKnob = S(48);
        const float knobY = advPanelY + S(22);
        const float kAdvKnobX[4] = { S(60), S(144), S(228), S(312) };

        const float descX = S(545);
        const float descY = advPanelY + S(20);
        const float descLh = S(19.5f);
        dl->AddRectFilled(ImVec2(S(536), advPanelY + S(22)),
                          ImVec2(S(536) + std::max(1.0f, S(1)), advPanelY + S(22 + 96)),
                          kLabelGray);
        auto descLine = [&](int line, const char* text) {
            widgets::DrawTextPx(F.light, 18.0f, ImVec2(descX, descY + descLh * line),
                                IM_COL32(182, 182, 182, 255), text);
        };

        if (mode == 0) {
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[0], knobY));
            float low = getParam(state, kInstLowSat);
            if (widgets::Knob(s.instLow, &low, 0.0f, 1.0f, 0.55f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kInstLowSat, low);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[1], knobY));
            float mid = getParam(state, kInstMidSat);
            if (widgets::Knob(s.instMid, &mid, 0.0f, 1.0f, 0.7f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kInstMidSat, mid);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[2], knobY));
            float high = getParam(state, kInstHighSat);
            if (widgets::Knob(s.instHigh, &high, 0.0f, 1.0f, 0.35f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kInstHighSat, high);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[3], knobY));
            float character = getParam(state, kInstCharacter);
            if (widgets::Knob(s.instChar, &character, 0.0f, 1.0f, 0.6f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kInstCharacter, character);
            }

            descLine(0, s.instDesc1);
            descLine(1, s.instDesc2);
            descLine(2, s.instDesc3);
            descLine(3, "y = lerp(tanh, tanh+T2, char)");
        } else if (mode == 1) {
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[0], knobY));
            float sens = getParam(state, kDrumTransientSens);
            if (widgets::Knob(s.drumSens, &sens, 0.0f, 1.0f, 0.6f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kDrumTransientSens, sens);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[1], knobY));
            float punch = getParam(state, kDrumPunch);
            if (widgets::Knob(s.drumPunch, &punch, 0.0f, 1.0f, 0.6f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kDrumPunch, punch);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[2], knobY));
            float sustain = getParam(state, kDrumSustainSat);
            if (widgets::Knob(s.drumSustain, &sustain, 0.0f, 1.0f, 0.5f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kDrumSustainSat, sustain);
            }

            descLine(0, s.drumDesc1);
            descLine(1, s.drumDesc2);
            descLine(2, s.drumDesc3);
            descLine(3, "y = lerp(sat, dry*boost, punch*gate\xc2\xb2)");
        } else {
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[0], knobY));
            float bias = getParam(state, kTapeBias);
            if (widgets::Knob(s.vocalBias, &bias, 0.0f, 1.0f, 0.3f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kTapeBias, bias);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[1], knobY));
            float wow = getParam(state, kTapeWow);
            if (widgets::Knob(s.vocalWow, &wow, 0.0f, 1.0f, 0.15f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kTapeWow, wow);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[2], knobY));
            float flutter = getParam(state, kTapeFlutter);
            if (widgets::Knob(s.vocalFlutter, &flutter, 0.0f, 1.0f, 0.1f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kTapeFlutter, flutter);
            }
            ImGui::SetCursorPos(ImVec2(kAdvKnobX[3], knobY));
            float cutoff = getParam(state, kTapeHeadCutoff);
            if (widgets::Knob(s.vocalCutoff, &cutoff, 0.0f, 1.0f, 0.75f, advKnob, true, "knob_small", 8.0f)) {
                setParam(state, kTapeHeadCutoff, cutoff);
            }

            descLine(0, s.vocalDesc1);
            descLine(1, s.vocalDesc2);
            descLine(2, s.vocalDesc3);
            descLine(3, s.vocalDesc4);
            descLine(4, "y = tanh(d*(x + fb*state))");
        }

        {
            const float osX = panelRight - S(67);
            const float kOsBtnW = S(34);
            const float kOsBtnH = S(18);
            const float kOsBtnY[3] = { advPanelY + S(37), advPanelY + S(59), advPanelY + S(81) };

            ImVec2 osTs = widgets::MeasureText(F.label, 16.0f, s.osLabel);
            widgets::DrawTextPx(F.label, 16.0f,
                                ImVec2(osX + (kOsBtnW - osTs.x) * 0.5f, advPanelY + S(14)),
                                kWhite, s.osLabel);

            float osNorm = getParam(state, kOversampling);
            int osMode = static_cast<int>(osNorm * 2.0f + 0.5f);

            static const char* osTexNames[] = { "os_1x", "os_2x", "os_4x" };

            ImGui::PushID("OsSelector");
            for (int oi = 0; oi < 3; ++oi) {
                bool osSel = (osMode == oi);
                auto osTex = TextureManager::get().find(osTexNames[oi]);

                ImGui::SetCursorPos(ImVec2(osX, kOsBtnY[oi]));
                ImGui::PushID(oi);

                ImVec2 btnPos = ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton("os", ImVec2(kOsBtnW, kOsBtnH));
                if (ImGui::IsItemClicked()) {
                    osMode = oi;
                    setParam(state, kOversampling, static_cast<float>(oi) / 2.0f);
                }

                if (osTex.id != 0 && osTex.height > 0) {
                    float v0 = osSel ? 0.5f : 0.0f;
                    float v1 = osSel ? 1.0f : 0.5f;
                    dl->AddImage(osTex.id, btnPos,
                                 ImVec2(btnPos.x + kOsBtnW, btnPos.y + kOsBtnH),
                                 ImVec2(0.0f, v0), ImVec2(1.0f, v1));
                } else {
                    ImU32 col = osSel ? IM_COL32(240, 133, 66, 255) : IM_COL32(46, 43, 38, 255);
                    dl->AddRectFilled(btnPos, ImVec2(btnPos.x + kOsBtnW, btnPos.y + kOsBtnH), col, S(3));
                }

                ImGui::PopID();
            }
            ImGui::PopID();
        }

        {
            float cx = ws.x * 0.5f;
            ImVec2 hMin(cx - S(40), advPanelY + advPanelH - S(15));
            ImVec2 hMax(cx + S(40), advPanelY + advPanelH);

            ImVec2 zoneMin(cx - S(42), advPanelY + advPanelH - S(17));
            bool hovered = ImGui::IsMouseHoveringRect(zoneMin, hMax);

            ImU32 fill = hovered ? IM_COL32(46, 46, 46, 255) : kTabFill;
            dl->AddRectFilled(hMin, hMax, fill, S(5), ImDrawFlags_RoundCornersTop);
            dl->AddRect(hMin, hMax, kAccent, S(5), ImDrawFlags_RoundCornersTop, border);

            float gripY = hMin.y + S(7);
            dl->AddRectFilled(ImVec2(cx - S(16), gripY), ImVec2(cx + S(16), gripY + S(2)),
                              kAccent, S(1));

            ImGui::SetCursorPos(zoneMin);
            ImGui::InvisibleButton("AdvClose", ImVec2(S(84), S(17)));
            if (ImGui::IsItemClicked()) {
                state.advancedOpen = false;
            }
        }
    }

    const float specX = S(24);
    const float specY = state.advancedOpen ? S(353) : S(219);
    float specW = panelRight - specX;
    float specH = std::max(ws.y - specY - S(24), S(40));

    if (!state.advancedOpen) {
        dl->AddRectFilled(ImVec2(specX, S(217)), ImVec2(panelRight, S(219)), kAccent);
    }

    ImGui::SetCursorPos(ImVec2(specX, specY));
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

    if (!state.advancedOpen) {
        ImVec2 ts = widgets::MeasureText(F.head, 19.0f, s.advToggle);
        const float bodyW = std::max(S(148), ts.x + S(24));
        const float bodyH = S(25);
        const float ear   = S(3);
        const float cx    = ws.x * 0.5f;
        const float bodyLeft  = cx - bodyW * 0.5f;
        const float bodyRight = cx + bodyW * 0.5f;
        const float bodyTop   = S(207);
        const float bodyBot   = bodyTop + bodyH;
        const float earY      = bodyTop + bodyH * (10.0f / 23.0f);

        bool hovered = ImGui::IsMouseHoveringRect(
            ImVec2(bodyLeft - ear, bodyTop), ImVec2(bodyRight + ear, bodyBot));

        ImU32 bgCol  = hovered ? IM_COL32(46, 46, 46, 255) : kTabFill;

        ImVec2 v0(bodyLeft + S(4), bodyTop);
        ImVec2 v1(bodyRight - S(4), bodyTop);
        ImVec2 v5(bodyRight + ear, earY);
        ImVec2 v2(bodyRight - S(4), bodyBot);
        ImVec2 v3(bodyLeft + S(4), bodyBot);
        ImVec2 v4(bodyLeft - ear, earY);

        ImVec2 verts[6] = { v0, v1, v5, v2, v3, v4 };
        dl->AddConvexPolyFilled(verts, 6, bgCol);
        dl->AddPolyline(verts, 6, kAccent, ImDrawFlags_Closed, std::max(1.0f, S(2)));

        widgets::DrawTextPx(F.head, 19.0f,
                            ImVec2(cx - ts.x * 0.5f, bodyTop + (bodyH - ts.y) * 0.5f),
                            kWhite, s.advToggle);

        ImGui::SetCursorPos(ImVec2(bodyLeft - ear, bodyTop));
        ImGui::InvisibleButton("AdvOpen", ImVec2(bodyW + ear * 2.0f, bodyH));
        if (ImGui::IsItemClicked()) {
            state.advancedOpen = true;
        }
    }

    ImGui::End();
}

} // namespace gui
