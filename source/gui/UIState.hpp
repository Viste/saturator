#pragma once

#include "../dsp/MeteringData.hpp"
#include "../PluginIds.hpp"
#include "Locale.hpp"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace gui {

struct UIState {
    Steinberg::Vst::EditController* controller = nullptr;
    dsp::MeteringData* metering = nullptr;

    bool advancedOpen = false;
    int requestedHeight = 0;
    Lang lang = Lang::RU;

    int currentMode() const {
        if (!controller) return 0;
        double norm = controller->getParamNormalized(kMode);
        return static_cast<int>(norm * 2.0 + 0.5);
    }
};

} // namespace gui
