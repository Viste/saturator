#include "MainKnobController.hpp"
#include <vstgui/lib/controls/ccontrol.h>
#include <vstgui/lib/controls/cknob.h>
#include <vstgui/uidescription/uiattributes.h>
#include <vstgui/uidescription/icontroller.h>

MainKnobController::MainKnobController(IController *baseController) :
        DelegationController(baseController) {
}

MainKnobController::~MainKnobController() {

}

CView *MainKnobController::verifyView(
        VSTGUI::CView *view,
        const UIAttributes &attributes,
        const VSTGUI::IUIDescription *description
) {
    ((CAnimKnob *) view)->setListener(this);
    return controller->verifyView(view, attributes, description);
}

int32_t MainKnobController::controlModifierClicked(CControl *pControl, CButtonState button) {
    if (button.isDoubleClick()) {
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

void MainKnobController::valueChanged(VSTGUI::CControl *pControl) {
    controller->valueChanged(pControl);
}


