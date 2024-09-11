#ifndef SATURATOR_DEMO_CUSTOMCKNOB_VIEWCREATOR_H
#define SATURATOR_DEMO_CUSTOMCKNOB_VIEWCREATOR_H

#include <vstgui/uidescription/uiviewfactory.h>
#include <vstgui4/vstgui/uidescription/uiviewcreator.h>
#include <vstgui/uidescription/uiattributes.h>
#include <vstgui4/vstgui/uidescription/detail/uiviewcreatorattributes.h>
#include <vstgui4/vstgui/uidescription/viewcreator/knobcreator.h>
#include <vstgui/vstgui.h>
#include "CustomCKnob.hpp"

using namespace VSTGUI;
using namespace VSTGUI::UIViewCreator;

class CustomCKnobCreator : public KnobBaseCreator {
public:
    // register this class with the view factory
    CustomCKnobCreator();

    // return an unique name here
    IdStringPtr getViewName() const;

    // return the name here from where your custom view inherites.
    // Your view automatically supports the attributes from it.
    IdStringPtr getBaseViewName() const;

    UTF8StringPtr getDisplayName() const;


    // create your view here.
    // Note you don't need to apply attributes here as the apply method will be called with this new view
    CView *create(const UIAttributes &attributes, const IUIDescription *description) const;

    // apply custom attributes to your view
    bool apply(CView *view, const UIAttributes &attributes, const IUIDescription *description) const;

    // add your custom attributes to the list
    bool getAttributeNames(StringList &attributeNames) const;

    // return the type of your custom attributes
    IViewCreator::AttrType getAttributeType(const std::string &attributeName) const;

    // return the string value of the custom attributes of the view
    bool getAttributeValue(
            CView *view,
            const string &attributeName,
            string &stringValue,
            const IUIDescription *desc
    ) const;
};

// create a static instance so that it registers itself with the view factory
CustomCKnobCreator __gCustomCKnobCreator;


#endif //SATURATOR_DEMO_CUSTOMCKNOB_VIEWCREATOR_H
