#ifndef FIRST_BASECONTROLLER_H
#define FIRST_BASECONTROLLER_H

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "subcontrollers/ModeSwitchController.hpp"
#include <vstgui/plugin-bindings/vst3editor.h>

using namespace Steinberg;
using namespace Steinberg::Vst;

struct BaseController : public EditController,
                        public VST3EditorDelegate {

    BaseController();

    ~BaseController() SMTG_OVERRIDE = default;

    static FUnknown *createInstance(void * /*context*/) {
        return (IEditController *) new BaseController;
    }

    tresult PLUGIN_API initialize(FUnknown *context) SMTG_OVERRIDE;


    tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    tresult PLUGIN_API setComponentState(IBStream *state) SMTG_OVERRIDE;

    IPlugView *PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;

    tresult PLUGIN_API setParamNormalized(
            ParamID tag,
            ParamValue value
    ) SMTG_OVERRIDE;

    tresult PLUGIN_API getParamStringByValue(
            ParamID tag,
            ParamValue valueNormalized,
            String128 string
    ) SMTG_OVERRIDE;

    tresult PLUGIN_API getParamValueByString(
            ParamID tag,
            TChar *string,
            ParamValue &valueNormalized
    ) SMTG_OVERRIDE;

    IController *createSubController(
            const char *name,
            const IUIDescription *description,
            VST3Editor *editor
    ) override;

    tresult PLUGIN_API notify(IMessage *message) SMTG_OVERRIDE;

    OBJ_METHODS(BaseController, EditController)

    REFCOUNT_METHODS(EditController)

private:

};

#endif // FIRST_BASECONTROLLER_H