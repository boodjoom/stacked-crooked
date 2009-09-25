#include "Element.h"
#include "ElementImpl.h"
#include "Utils/ErrorReporter.h"
#include "Poco/StringTokenizer.h"
#include <boost/bind.hpp>


using namespace Utils;


namespace XULWin
{
    ElementImpl * gNullNativeComponent(0);


    Element::Element(const std::string & inType, Element * inParent, ElementImpl * inNativeComponent) :
        mType(inType),
        mParent(inParent),
        mImpl(inNativeComponent)
    {
        if (mImpl)
        {
            mImpl->setOwningElement(this);
        }
    }


    Element::~Element()
    {
        // Children require parent access while destructing.
        // So we destruct them while parent still alive.
        mChildren.clear();
    }


    const std::string & Element::type() const
    {
        return mType;
    }

    
    Element * Element::getElementById(const std::string & inId)
    {
        struct Helper
        {
            static Element * findChildById(const Children & inChildren, const std::string & inId)
            {
                Element * result(0);
                for (size_t idx = 0; idx != inChildren.size(); ++idx)
                {
                    ElementPtr child = inChildren[idx];
                    if (child->getAttribute("id") == inId)
                    {
                        result = child.get();
                        break;
                    }
                    else
                    {
                        result = findChildById(child->children(), inId);
                        if (result)
                        {
                            break;
                        }
                    }
                }
                return result;
            }
        };
        return Helper::findChildById(children(), inId);
    }


    void Element::removeChild(const Element * inChild)
    {
        Children::iterator it = std::find_if(mChildren.begin(), mChildren.end(), boost::bind(&ElementPtr::get, _1) == inChild);
        if (it != mChildren.end())
        {
            mChildren.erase(it);
            mImpl->rebuildLayout();
        }
        else
        {
            ReportError("Remove child failed because it wasn't found.");
        }
    }


    void Element::setStyles(const AttributesMapping & inAttributes)
    {
        AttributesMapping::const_iterator it = inAttributes.find("style");
        if (it != inAttributes.end())
        {
            Poco::StringTokenizer tok(it->second, ";:",
                                      Poco::StringTokenizer::TOK_IGNORE_EMPTY
                                      | Poco::StringTokenizer::TOK_TRIM);

            Poco::StringTokenizer::Iterator it = tok.begin(), end = tok.end();
            std::string key, value;
            int counter = 0;
            for (; it != end; ++it)
            {
                if (counter%2 == 0)
                {
                    key = *it;
                }
                else
                {
                    value = *it;
                    setStyle(key, value);
                }
                counter++;
            }
        }
    }


    void Element::setAttributes(const AttributesMapping & inAttributes)
    {
        mAttributes = inAttributes;
        
        if (mImpl)
        {
            AttributesMapping::iterator it = mAttributes.begin(), end = mAttributes.end();
            for (; it != end; ++it)
            {
                // ignore error reports about failure to apply attributes
                // it's unlikely to be an issue here
                ErrorCatcher errorIgnorer;
                errorIgnorer.disableLogging(true);
                setAttribute(it->first, it->second);
            }
        }
    }


    void Element::initAttributeControllers()
    {
        if (mImpl)
        {
            mImpl->initAttributeControllers();
        }
    }


    void Element::initStyleControllers()
    {
        if (mImpl)
        {
            mImpl->initStyleControllers();
        }
    }

    
    std::string Element::getAttribute(const std::string & inName) const
    {
        std::string result;
        if (!mImpl || !mImpl->getAttribute(inName, result))
        {
            AttributesMapping::const_iterator it = mAttributes.find(inName);
            if (it != mAttributes.end())
            {
                result = it->second;
            }
        }
        return result;
    }
    
    
    std::string Element::getDocumentAttribute(const std::string & inName) const
    {
        std::string result;
        AttributesMapping::const_iterator it = mAttributes.find(inName);
        if (it != mAttributes.end())
        {
            result = it->second;
        }
        return result;
    }
    
    
    void Element::setStyle(const std::string & inName, const std::string & inValue)
    {
        std::string type = this->type();
        if (!mImpl || !mImpl->setStyle(inName, inValue))
        {
            mStyles[inName] = inValue;
        }
    }
    
    
    void Element::setAttribute(const std::string & inName, const std::string & inValue)
    {
        if (!mImpl || !mImpl->setAttribute(inName, inValue))
        {
            mAttributes[inName] = inValue;
        }
    }
    
    
    ElementImpl * Element::impl() const
    {
        return mImpl.get();
    }
    
    
    void Element::addChild(ElementPtr inChild)
    {
        mChildren.push_back(inChild);
    }


    Window::Window(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Window::Type(),
                inParent,
                new NativeWindow(inAttributesMapping))
    {
    }


    void Window::showModal()
    {
        if (NativeWindow * nativeWindow = impl()->downcast<NativeWindow>())
        {
            nativeWindow->showModal();
        }
    }


    Button::Button(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Button::Type(),
                inParent,
                new PaddingDecorator(new NativeButton(inParent->impl(), inAttributesMapping)))
    {
    }


    Label::Label(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Label::Type(),
                inParent,
                new PaddingDecorator(new NativeLabel(inParent->impl(), inAttributesMapping)))
    {
    }


    std::string Label::value() const
    {
        return getAttribute("value");
    }


    Description::Description(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Description::Type(),
                inParent,
                new PaddingDecorator(new NativeDescription(inParent->impl(), inAttributesMapping)))
    {
    }


    Text::Text(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Text::Type(),
                inParent,
                new PaddingDecorator(new NativeLabel(inParent->impl(), inAttributesMapping)))
    {
    }


    TextBox::TextBox(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(TextBox::Type(),
                inParent,
                new PaddingDecorator(new NativeTextBox(inParent->impl(), inAttributesMapping)))
    {
    }


    CheckBox::CheckBox(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(CheckBox::Type(),
                inParent,
                new PaddingDecorator(new NativeCheckBox(inParent->impl(), inAttributesMapping)))
    {
    }


    Box::Box(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Box::Type(),
                inParent,
                new NativeBox(inParent->impl(), inAttributesMapping))
    {
    }


    HBox::HBox(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(HBox::Type(),
                inParent,
                new NativeHBox(inParent->impl(), inAttributesMapping))
    {
    }


    VBox::VBox(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(VBox::Type(),
                inParent,
                new NativeVBox(inParent->impl(), inAttributesMapping))
    {
    }


    MenuList::MenuList(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(MenuList::Type(),
                inParent,
                new PaddingDecorator(new NativeMenuList(inParent->impl(), inAttributesMapping)))
    {
    }
        
    
    void MenuList::addMenuItem(const MenuItem * inItem)
    {
        if (NativeMenuList * nativeMenuList = impl()->downcast<NativeMenuList>())
        {
            nativeMenuList->addMenuItem(inItem->label());
        }
    }
        
    
    void MenuList::removeMenuItem(const MenuItem * inItem)
    {
        if (NativeMenuList * nativeMenuList = impl()->downcast<NativeMenuList>())
        {
            nativeMenuList->removeMenuItem(inItem->label());
        }
    }

    
    MenuPopup::MenuPopup(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(MenuPopup::Type(), inParent, gNullNativeComponent),
        mDestructing(false)
    {
    }

    
    MenuPopup::~MenuPopup()
    {
        mDestructing = true;
    }


    void MenuPopup::addMenuItem(const MenuItem * inItem)
    {
        if (mParent->type() == "menulist")
        {
            if (MenuList * menuList = mParent->downcast<MenuList>())
            {
                menuList->addMenuItem(inItem);
            }
        }
        else if (mParent->type() == "menubutton")
        {
            // not yet implemented
        }
        else
        {
            ReportError("MenuPopup is located in non-compatible container.");
        }
    }


    void MenuPopup::removeMenuItem(const MenuItem * inItem)
    {
        // We don't remove menu items when destructing.
        // It is not needed.
        if (mDestructing)
        {
            return;
        }

        if (mParent->type() == "menulist")
        {
            if (MenuList * menuList = mParent->downcast<MenuList>())
            {
                menuList->removeMenuItem(inItem);
            }
        }
        else if (mParent->type() == "menubutton")
        {
            // not yet implemented
        }
        else
        {
            ReportError("MenuPopup is located in non-compatible container.");
        }
    }

    
    MenuItem::MenuItem(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(MenuItem::Type(), inParent, gNullNativeComponent)
    {
    }
        
    
    MenuItem::~MenuItem()
    {
        if (MenuPopup * popup = mParent->downcast<MenuPopup>())
        {
            popup->removeMenuItem(this);
        }
        else
        {
            ReportError("MenuItem is located in non-compatible container.");
        }
    }


    void MenuItem::init()
    {
        if (MenuPopup * popup = mParent->downcast<MenuPopup>())
        {
            popup->addMenuItem(this);
        }
        else
        {
            ReportError("MenuItem is located in non-compatible container.");
        }
    }

    
    std::string MenuItem::label() const
    {
        return getAttribute("label");
    }

    
    std::string MenuItem::value() const
    {
        return getAttribute("value");
    }


    Separator::Separator(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Separator::Type(),
                inParent,
                new PaddingDecorator(new NativeSeparator(inParent->impl(), inAttributesMapping)))
    {
    }


    Separator::~Separator()
    {
    }


    Spacer::Spacer(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Spacer::Type(),
                inParent,
                new NativeSpacer(inParent->impl(), inAttributesMapping))
    {
    }


    Spacer::~Spacer()
    {
    }


    MenuButton::MenuButton(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(MenuButton::Type(),
                inParent,
                new PaddingDecorator(new NativeMenuButton(inParent->impl(), inAttributesMapping)))
    {
    }


    MenuButton::~MenuButton()
    {
    }


    Grid::Grid(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Grid::Type(),
                inParent,
                new NativeGrid(inParent->impl(), inAttributesMapping))
    {
    }


    Grid::~Grid()
    {
    }


    Rows::Rows(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Rows::Type(),
                inParent,
                new NativeRows(inParent->impl(), inAttributesMapping))
    {
    }


    Rows::~Rows()
    {
    }


    Columns::Columns(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Columns::Type(),
                inParent,
                new NativeColumns(inParent->impl(), inAttributesMapping))
    {
    }


    Columns::~Columns()
    {
    }


    Row::Row(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Row::Type(),
                inParent,
                new NativeRow(inParent->impl(), inAttributesMapping))
    {
    }


    Row::~Row()
    {
    }


    Column::Column(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Column::Type(),
                inParent,
                new NativeColumn(inParent->impl(), inAttributesMapping))
    {
    }


    Column::~Column()
    {
    }


    RadioGroup::RadioGroup(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(RadioGroup::Type(),
                inParent,
                new NativeRadioGroup(inParent->impl(), inAttributesMapping))
    {
    }


    RadioGroup::~RadioGroup()
    {
    }


    Radio::Radio(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Radio::Type(),
                inParent,
                new PaddingDecorator(new NativeRadio(inParent->impl(), inAttributesMapping)))
    {
    }


    Radio::~Radio()
    {
    }


    ProgressMeter::ProgressMeter(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(ProgressMeter::Type(),
                inParent,
                new PaddingDecorator(new NativeProgressMeter(inParent->impl(), inAttributesMapping)))
    {
    }


    ProgressMeter::~ProgressMeter()
    {
    }


    Deck::Deck(Element * inParent, const AttributesMapping & inAttributesMapping) :
        Element(Deck::Type(),
                inParent,
                new NativeDeck(inParent->impl(), inAttributesMapping))
    {
    }


    Deck::~Deck()
    {
    }


} // XULWin
