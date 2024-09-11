#include "CustomCKnob.hpp"

CustomCKnob::CustomCKnob(const CRect &size, IControlListener *listener, int32_t tag, CBitmap *background) : CAnimKnob(size, listener, tag, background) {}

CMouseEventResult CustomCKnob::onMouseMoved(VSTGUI::CPoint &where, const VSTGUI::CButtonState &buttons)
{
    if (buttons.isDoubleClick())
    {
        if (auto listener = getListener())
        {
            listener->controlModifierClicked(this, buttons);
            return kMouseEventHandled;
        }
    }
    return CAnimKnob::onMouseMoved(where, buttons);
}