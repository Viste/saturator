#ifndef SATURATOR_DEMO_CUSTOMCKNOB_H
#define SATURATOR_DEMO_CUSTOMCKNOB_H

#include <iostream>
#include "vstgui4/vstgui/lib/controls/cknob.h"
#include "vstgui4/vstgui/lib/controls/icontrollistener.h"

using namespace VSTGUI;

struct CustomCKnob : public CAnimKnob {

    CustomCKnob(
            const CRect &size,
            IControlListener *listener,
            int32_t tag,
            CBitmap *background
    );

    CMouseEventResult onMouseMoved(CPoint &where, const CButtonState &buttons) override;

    CLASS_METHODS(CustomCKnob, CKnob)

};


#endif //SATURATOR_DEMO_CUSTOMCKNOB_H
