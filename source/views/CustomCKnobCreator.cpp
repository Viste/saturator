#include <vstgui/uidescription/viewcreator/multibitmapcontrolcreator.h>
#include "CustomCKnobCreator.hpp"

CustomCKnobCreator::CustomCKnobCreator() { UIViewFactory::registerViewCreator(*this); }

// return an unique name here
IdStringPtr CustomCKnobCreator::getViewName() const { return "CustomCKnob"; }

// return the name here from where your custom view inherites.
// Your view automatically supports the attributes from it.
IdStringPtr CustomCKnobCreator::getBaseViewName() const
{
    return kCControl;
}

//------------------------------------------------------------------------
UTF8StringPtr CustomCKnobCreator::getDisplayName() const
{
    return "Custom Anim Knob";
}


// create your view here.
// Note you don't need to apply attributes here as the apply method will be called with this new view
CView *CustomCKnobCreator::create(const UIAttributes &attributes, const IUIDescription *description) const
{
    return new CustomCKnob(CRect(0, 0, 0, 0), nullptr, -1, nullptr);
}

// apply custom attributes to your view
bool CustomCKnobCreator::apply(CView *view, const UIAttributes &attributes, const IUIDescription *description
) const {
    auto *animKnob = dynamic_cast<CAnimKnob *> (view);
    if (!animKnob)
        return false;

    bool b;
    if (attributes.getBooleanAttribute(kAttrInverseBitmap, b))
    {
        animKnob->setInverseBitmap(b);
    }
#if VSTGUI_ENABLE_DEPRECATED_METHODS
    IMultiBitmapControlCreator::apply(view, attributes, description);
#endif
    return KnobBaseCreator::apply(view, attributes, description);
}

// add your custom attributes to the list
bool CustomCKnobCreator::getAttributeNames(StringList &attributeNames) const
{
    attributeNames.emplace_back(kAttrInverseBitmap);
#if VSTGUI_ENABLE_DEPRECATED_METHODS
    IMultiBitmapControlCreator::getAttributeNames(attributeNames);
#endif
    return KnobBaseCreator::getAttributeNames(attributeNames);
}

// return the type of your custom attributes
IViewCreator::AttrType CustomCKnobCreator::getAttributeType(const std::string &attributeName) const
{
    if (attributeName == kAttrInverseBitmap)
        return kBooleanType;
    auto res = KnobBaseCreator::getAttributeType(attributeName);
    if (res != kUnknownType)
        return res;
#if VSTGUI_ENABLE_DEPRECATED_METHODS
    return IMultiBitmapControlCreator::getAttributeType(attributeName);
#else
    return res;
#endif
}

// return the string value of the custom attributes of the view
bool CustomCKnobCreator::getAttributeValue(CView *view, const string &attributeName, string &stringValue, const IUIDescription *desc) const
{
    auto *animKnob = dynamic_cast<CAnimKnob *> (view);
    if (!animKnob)
        return false;

    if (attributeName == kAttrInverseBitmap)
    {
        stringValue = animKnob->getInverseBitmap() ? strTrue : strFalse;
        return true;
    }
    if (KnobBaseCreator::getAttributeValue(view, attributeName, stringValue, desc))
        return true;
#if VSTGUI_ENABLE_DEPRECATED_METHODS
    return IMultiBitmapControlCreator::getAttributeValue(view, attributeName, stringValue, desc);
#else
    return false;
#endif
}
