#include "CustomVST3Editor.hpp"

CustomVST3Editor::CustomVST3Editor(Steinberg::Vst::EditController *controller, VSTGUI::UTF8StringPtr templateName,
                                   VSTGUI::UTF8StringPtr xmlFile) : VST3Editor(controller, templateName, xmlFile) {}

Steinberg::tresult
CustomVST3Editor::setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor) {
    return Steinberg::kResultOk;
}
