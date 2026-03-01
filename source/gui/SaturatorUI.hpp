#pragma once

#include "UIState.hpp"

namespace gui {

// главный ui плагина на dear imgui
struct SaturatorUI {
    static void render(UIState& state);
};

} // namespace gui
