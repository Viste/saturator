#ifndef SATURATOR_DEMO_CUSTOMVST3EDITOR_H
#define SATURATOR_DEMO_CUSTOMVST3EDITOR_H

#include "vstgui4/vstgui/plugin-bindings/vst3editor.h"
#include "vstgui4/vstgui/lib/vstguibase.h"

struct CustomVST3Editor:public VSTGUI::VST3Editor {

    CustomVST3Editor(Steinberg::Vst::EditController* controller, VSTGUI::UTF8StringPtr templateName, VSTGUI::UTF8StringPtr xmlFile);

    Steinberg::tresult setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor) override;

};


#endif //SATURATOR_DEMO_CUSTOMVST3EDITOR_H
