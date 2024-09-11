#include "ModeSwitchController.hpp"
#include <vstgui/lib/controls/ccontrol.h>
#include <vstgui/lib/controls/cswitch.h>
#include <vstgui/uidescription/icontroller.h>
#include <vstgui/uidescription/uiattributes.h>
#include <iostream>

ModeSwitchController::ModeSwitchController(
        IController *baseController
) : DelegationController(baseController) {

}

ModeSwitchController::~ModeSwitchController() {
    if (horizontalSwitch) horizontalSwitch->forget();
}

CView *ModeSwitchController::verifyView(VSTGUI::CView *view, const UIAttributes &attributes, const VSTGUI::IUIDescription *description)
{

    const std::string *name = attributes.getAttributeValue("custom-view-name");
    if (!name)
    {
        return controller->verifyView(view, attributes, description);
    }
    if (*name == "ModeSwitch")
    {
        horizontalSwitch = (CHorizontalSwitch *) view;
        horizontalSwitch->setListener(this);
        horizontalSwitch->remember();
        return horizontalSwitch;
    }

    ((CHorizontalSwitch *) view)->setListener(this);
    return controller->verifyView(view, attributes, description);
}

int32_t ModeSwitchController::controlModifierClicked(
        CControl *pControl,
        CButtonState button
) {
    if (button.isDoubleClick())
    {
        pControl->beginEdit();
        pControl->setValue(pControl->getDefaultValue());
        pControl->endEdit();
        pControl->valueChanged();
        if (pControl->isDirty()) {
            pControl->invalid();
        }
        return VSTGUI::kMouseEventHandled;
    }
    return controller->controlModifierClicked(pControl, button);
}

void ModeSwitchController::valueChanged(VSTGUI::CControl *pControl) {
    controller->valueChanged(pControl);
}

