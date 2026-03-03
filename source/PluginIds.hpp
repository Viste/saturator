#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

static const Steinberg::FUID kSaturatorProcessorUID(0x2E4A854C, 0x7FCB523F, 0xCD849892, 0xA6E241C9);
static const Steinberg::FUID kSaturatorControllerUID(0x3F5BB5E5, 0xE7D64B78, 0xA7DDB976, 0xAF892432);

enum Params : Steinberg::Vst::ParamID {
    kBypass        = 0,
    kMode          = 1,     // 0=инструмент, 1=ударные, 2=вокал
    kSaturation    = 2,
    kInputGain     = 3,
    kOutputGain    = 4,
    kDryWet        = 5,
    kOversampling  = 6,

    // инструмент (100-199)
    kInstLowSat      = 100,
    kInstMidSat      = 101,
    kInstHighSat     = 102,
    kInstLowMidFreq  = 103,
    kInstMidHighFreq = 104,
    kInstCharacter   = 105,

    // ударные (200-299)
    kDrumTransientSens = 200,
    kDrumAttackMs      = 201,
    kDrumSustainSat    = 202,
    kDrumPunch         = 203,

    // вокал/лента (300-399)
    kTapeBias       = 300,
    kTapeWow        = 301,
    kTapeFlutter    = 302,
    kTapeHissLevel  = 303,
    kTapeHeadCutoff = 304,
    kTapeSpeed      = 305,

    // клиппер (400-499)
    kClipEnabled    = 400,
    kClipAmount     = 401,
};

#define SaturatorVST3Category "Fx"
