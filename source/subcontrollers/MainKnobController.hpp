#ifndef VSRATURATOR_MAINKNOBCONTROLLER_H
#define VSRATURATOR_MAINKNOBCONTROLLER_H

#include <vstgui/uidescription/delegationcontroller.h>

using namespace VSTGUI;

struct MainKnobController : public DelegationController, public CBaseObject {

    explicit MainKnobController(IController *baseController);

    ~MainKnobController() override;

    CView *verifyView(
            VSTGUI::CView *view,
            const VSTGUI::UIAttributes &attributes,
            const VSTGUI::IUIDescription *description
    ) override;

    int32_t controlModifierClicked(VSTGUI::CControl *pControl, VSTGUI::CButtonState button) override;

    void valueChanged(VSTGUI::CControl *pControl) override;

};


#endif //VSRATURATOR_MAINKNOBCONTROLLER_H
