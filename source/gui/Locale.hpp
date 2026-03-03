#pragma once

namespace gui {

enum class Lang { RU, EN };

struct Strings {
    // header
    const char* title;
    // modes
    const char* modeInstrument;
    const char* modeDrums;
    const char* modeVocal;
    // main controls
    const char* advToggle;
    const char* drive;
    const char* mix;
    const char* inOut;
    // displays
    const char* signal;
    const char* spectrum;
    // instrument
    const char* instLow;
    const char* instMid;
    const char* instHigh;
    const char* instChar;
    const char* instDesc1;
    const char* instDesc2;
    const char* instDesc3;
    // drums
    const char* drumSens;
    const char* drumPunch;
    const char* drumSustain;
    const char* drumDesc1;
    const char* drumDesc2;
    const char* drumDesc3;
    // vocal/tape
    const char* vocalBias;
    const char* vocalWow;
    const char* vocalFlutter;
    const char* vocalCutoff;
    const char* vocalDesc1;
    const char* vocalDesc2;
    const char* vocalDesc3;
    const char* vocalDesc4;
    // oversampling
    const char* osLabel;
    // waveform legend
    const char* wfIn;
    const char* wfOut;
    // clipper
    const char* clipLabel;
};

void setLocale(Lang lang);
Lang currentLang();
const Strings& locale();

} // namespace gui
