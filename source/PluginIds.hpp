#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

//------------------------------------------------------------------------
static const Steinberg::FUID kSaturatorProcessorUID (0x2E4A854C, 0x7FCB523F, 0xCD849892, 0xA6E241C9);
static const Steinberg::FUID kSaturatorControllerUID (0x3F5BB5E5, 0xE7D64B78, 0xA7DDB976, 0xAF892432);



enum Params : Steinberg::Vst::ParamID {
    kBypass = 1,
    kSaturation = 2,
    kGain = 3,
    kDeath = 4,
    kSwitch = 5
};

#define SaturatorVST3Category "Fx"
