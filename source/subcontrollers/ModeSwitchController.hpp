#ifndef SATURATOR_DEMO_MODESWITCHCONTROLLER_H
#define SATURATOR_DEMO_MODESWITCHCONTROLLER_H

#include <vstgui/uidescription/delegationcontroller.h>
#include <vstgui/lib/controls/cswitch.h>
#include <vstgui/lib/controls/cbuttons.h>

using namespace VSTGUI;

struct ModeSwitchController : public DelegationController, public CBaseObject {

    explicit ModeSwitchController(IController *baseController);

    ~ModeSwitchController() override;

    CView *verifyView(VSTGUI::CView *view, const VSTGUI::UIAttributes &attributes, const VSTGUI::IUIDescription *description) override;

    int32_t controlModifierClicked(VSTGUI::CControl *pControl, VSTGUI::CButtonState button) override;

    void valueChanged(VSTGUI::CControl *pControl) override;

    CHorizontalSwitch * horizontalSwitch = nullptr;

};

#endif // SATURATOR_DEMO_MODESWITCHCONTROLLER_H
