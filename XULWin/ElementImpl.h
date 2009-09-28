#ifndef NATIVECOMPONENT_H_INCLUDED
#define NATIVECOMPONENT_H_INCLUDED


#include "Element.h"
#include "AttributeController.h"
#include "Conversions.h"
#include "Layout.h"
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <windows.h>
#include <string>
#include <map>


namespace XULWin
{
    class CommandId
    {
    public:
        CommandId() : mId(sId++) {}

        CommandId(int inId) : mId(inId) {}

        int intValue() const { return mId; }

    private:
        int mId;
        static int sId;
    };


    class Element;
    class ElementImpl;
    class Decorator;
    class BoxLayouter;
    class NativeComponent;
    typedef boost::shared_ptr<ElementImpl> ElementImplPtr;

    /**
     * ElementImpl is base class for all native UI elements.
     */
    class ElementImpl : public virtual WidthController,
                        public virtual HeightController,
                        boost::noncopyable
    {
    public:
        ElementImpl(ElementImpl * inParent);

        virtual ~ElementImpl() = 0;

        // WidthController methods
        virtual int getWidth() const;

        virtual void setWidth(int inWidth);

        // HeightController methods
        virtual int getHeight() const;

        virtual void setHeight(int inHeight);

        // Downcast that also resolves decorators.
        // Use this instead of manual cast, because
        // you may get a decorator instead of the 
        // actual element.
        template<class Type>
        Type * downcast()
        {
            if (Type * obj = dynamic_cast<Type*>(this))
            {
                return obj;
            }
            else if (Decorator * obj = dynamic_cast<Decorator*>(this))
            {
                return obj->decoratedElement()->downcast<Type>();
            }
            return 0;
        }


        template<class ConstType>
        const ConstType * constDowncast() const
        {
            if (const ConstType * obj = dynamic_cast<const ConstType*>(this))
            {
                return obj;
            }
            else if (const Decorator * obj = dynamic_cast<const Decorator*>(this))
            {
                return obj->decoratedElement()->constDowncast<ConstType>();
            }
            return 0;
        }

        int commandId() const { return mCommandId.intValue(); }

        int minimumWidth() const;

        int minimumHeight() const;

        virtual int calculateMinimumWidth() const = 0;

        virtual int calculateMinimumHeight() const = 0;

        // Tendency to expand, used for separators, scrollbars, etc..
        bool expansive() const;

        virtual void move(int x, int y, int w, int h) = 0;

        virtual Rect clientRect() const = 0;

        virtual void setOwningElement(Element * inElement);

        virtual Element * owningElement() const;

        ElementImpl * parent() const;

        virtual void rebuildLayout() = 0;

        void rebuildChildLayouts();

        virtual LRESULT handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam) = 0;

        virtual bool getAttribute(const std::string & inName, std::string & outValue);

        virtual bool getStyle(const std::string & inName, std::string & outValue);

        virtual bool setStyle(const std::string & inName, const std::string & inValue);

        virtual bool setAttribute(const std::string & inName, const std::string & inValue);

        virtual bool initAttributeControllers();

        virtual bool initOldStyleControllers();

        void setAttributeController(const std::string & inAttr, AttributeController * inController);

    protected:
        friend class BoxLayouter;
        ElementImpl * mParent;
        Element * mElement;
        CommandId mCommandId;
        bool mExpansive;
        typedef std::map<std::string, AttributeController *> AttributeControllers;
        AttributeControllers mAttributeControllers;


        typedef boost::function<std::string()> Getter;
        typedef boost::function<void(const std::string &)> Setter;
        struct Controller
        {
            Controller(Getter & inGetter, Setter & inSetter) :
                getter(inGetter),
                setter(inSetter)
            {
            }
            Getter getter;
            Setter setter;
        };

        typedef Getter StyleGetter;
        typedef Setter StyleSetter;
        typedef Controller OldStyleController;
        void setOldStyleController(const std::string & inAttr, const OldStyleController & inController);
        typedef std::map<std::string, OldStyleController> OldStyleControllers;
        OldStyleControllers mOldStyleControllers;

        typedef std::map<HWND, ElementImpl*> Components;
        static Components sComponentsByHandle;
    };


    class NativeComponent : public ElementImpl,
                            public virtual DisabledController,
                            public virtual LabelController
    {
    public:
        typedef ElementImpl Super;

        NativeComponent(ElementImpl * inParent, const AttributesMapping & inAttributes);

        virtual ~NativeComponent();

        // DisabledController methods
        virtual bool getDisabled() const;

        virtual void setDisabled(bool inDisabled);

        // LabelController methods
        virtual std::string getLabel() const;

        virtual void setLabel(const std::string & inLabel);

        static void SetModuleHandle(HMODULE inModule);

        virtual HWND handle() const;

        virtual bool initAttributeControllers();

        virtual bool initOldStyleControllers();

        /**
         * Override this method if you want your control to handle its own command events.
         * (Normally the parent control handles them through the WM_COMMAND message.)
         */
        virtual void handleCommand(WPARAM wParam, LPARAM lParam) {}

        virtual LRESULT handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam);

        static LRESULT CALLBACK MessageHandler(HWND hWnd, UINT inMessage, WPARAM wParam, LPARAM lParam);

    protected:
        HWND mHandle;
        HMODULE mModuleHandle;

        typedef std::map<int, NativeComponent*> ComponentsById;
        static ComponentsById sComponentsById;

        WNDPROC mOrigProc;

    private:
        static HMODULE sModuleHandle;
    };


    class NativeWindow : public NativeComponent,
                         public virtual TitleController
    {
    public:
        typedef NativeComponent Super;

        static void Register(HMODULE inModuleHandle);

        NativeWindow(const AttributesMapping & inAttributesMapping);

        virtual std::string getTitle() const;

        virtual void setTitle(const std::string & inTitle);

        void showModal();

        virtual void move(int x, int y, int w, int h);

        virtual void rebuildLayout();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        virtual Rect clientRect() const;

        virtual Rect windowRect() const;

        virtual bool initAttributeControllers();

        virtual bool initOldStyleControllers();

        virtual LRESULT handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam);

        static LRESULT CALLBACK MessageHandler(HWND hWnd, UINT inMessage, WPARAM wParam, LPARAM lParam);
    };


    class NativeControl : public NativeComponent
    {
    public:
        typedef NativeComponent Super;

        NativeControl(ElementImpl * inParent, const AttributesMapping & inAttributesMapping, LPCTSTR inClassName, DWORD inExStyle, DWORD inStyle);

        virtual ~NativeControl();
        
        bool initOldStyleControllers();

        virtual void rebuildLayout();

        virtual Rect clientRect() const;

        virtual void move(int x, int y, int w, int h);

        // Gets a NativeComponent object from this object. This
        // is only needed in constructors of NativeComponents, because
        // they need to know which is their native parent handle object.
        // If this is a NativeComponent, return this.
        // If this is a VirtualControl, return first parent that is a NativeComponent.
        // If this is a Decorator, resolve until a NativeComponent is found.
        static NativeComponent * GetNativeParent(ElementImpl * inElementImpl);
    };


    class VirtualControl : public ElementImpl
    {
    public:
        typedef ElementImpl Super;

        VirtualControl(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual ~VirtualControl();

        virtual bool initAttributeControllers();

        virtual bool initOldStyleControllers();

        virtual int calculateMinimumWidth() const { return 0; }

        virtual int calculateMinimumHeight() const { return 0; }

        virtual void rebuildLayout();

        virtual Rect clientRect() const;

        virtual void move(int x, int y, int w, int h);

        virtual LRESULT handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam);

    protected:
        Rect mRect;
    };


    class NativeButton : public NativeControl
    {
    public:
        typedef NativeControl Super;

        NativeButton(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeLabel : public NativeControl,
                        public virtual StringValueController
    {
    public:
        typedef NativeControl Super;

        NativeLabel(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        // StringValueController methods
        virtual std::string getValue() const;

        virtual void setValue(const std::string & inStringValue);

        virtual bool initAttributeControllers();

        virtual bool initOldStyleControllers();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeDescription : public NativeControl,
                              public virtual StringValueController
    {
    public:
        typedef NativeControl Super;

        NativeDescription(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        // StringValueController methods
        virtual std::string getValue() const;

        virtual void setValue(const std::string & inStringValue);

        virtual bool initAttributeControllers();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeTextBox : public NativeControl,
                          public virtual StringValueController,
                          public virtual ReadOnlyController
    {
    public:
        typedef NativeControl Super;

        NativeTextBox(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        // StringValueController methods
        virtual std::string getValue() const;

        virtual void setValue(const std::string & inStringValue);

        // ReadOnlyController methods
        virtual bool isReadOnly() const;

        virtual void setReadOnly(bool inReadOnly);

        virtual bool initAttributeControllers();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        virtual void handleCommand(WPARAM wParam, LPARAM lParam);

    private:
        bool mReadonly;
        static DWORD GetFlags(const AttributesMapping & inAttributesMapping);
        static bool IsReadOnly(const AttributesMapping & inAttributesMapping);
    };


    class NativeCheckBox : public NativeControl,
                           public virtual CheckedController
    {
    public:
        typedef NativeControl Super;

        NativeCheckBox(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual bool isChecked() const;

        virtual void setChecked(bool inChecked);

        virtual bool initAttributeControllers();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class BoxLayouter : public virtual OrientController,
                        public virtual AlignController
    {
    public:
        BoxLayouter(Orientation inOrient, Alignment inAlign);

        // OrientController methods
        virtual Orientation getOrient() const;

        virtual void setOrient(Orientation inOrient);

        // AlignController methods
        virtual Alignment getAlign() const;

        virtual void setAlign(Alignment inAlign);

        virtual bool initAttributeControllers();

        virtual void setAttributeController(const std::string & inAttr, AttributeController * inController) = 0;

        //virtual void setOldAttributeController(const std::string & inAttr, const ElementImpl::OldAttributeController & inController) = 0;

        virtual void rebuildLayout();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        virtual size_t numChildren() const = 0;

        virtual const ElementImpl * getChild(size_t idx) const = 0;

        virtual ElementImpl * getChild(size_t idx) = 0;

        virtual Rect clientRect() const = 0;

        virtual void rebuildChildLayouts() = 0;

    private:
        Orientation mOrient;
        Alignment mAlign;
    };


    class VirtualBox : public VirtualControl,
                       public BoxLayouter
    {
    public:
        typedef VirtualControl Super;

        VirtualBox(ElementImpl * inParent, const AttributesMapping & inAttributesMapping, Orientation inOrient = HORIZONTAL);

        virtual bool initAttributeControllers();

        virtual void rebuildLayout()
        {
            BoxLayouter::rebuildLayout();
        }

        virtual int calculateMinimumWidth() const
        {
            return BoxLayouter::calculateMinimumWidth();
        }

        virtual int calculateMinimumHeight() const
        {
            return BoxLayouter::calculateMinimumHeight();
        }
        
        virtual size_t numChildren() const
        { return mElement->children().size(); }

        virtual const ElementImpl * getChild(size_t idx) const
        { return mElement->children()[idx]->impl(); }

        virtual ElementImpl * getChild(size_t idx)
        { return mElement->children()[idx]->impl(); }

        virtual Rect clientRect() const
        { return Super::clientRect(); }

        virtual void rebuildChildLayouts()
        { return Super::rebuildChildLayouts(); }
        

        virtual void setAttributeController(const std::string & inAttr, AttributeController * inController);

        //virtual void setOldAttributeController(const std::string & inAttr, const OldAttributeController & inController);
    };


    class NativeBox : public NativeControl,
                      public BoxLayouter
    {
    public:
        typedef NativeControl Super;

        NativeBox(ElementImpl * inParent, const AttributesMapping & inAttributesMapping, Orientation inOrient);

        virtual bool initAttributeControllers();

        virtual void rebuildLayout();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        virtual Rect clientRect() const;

        virtual size_t numChildren() const
        { return mElement->children().size(); }

        virtual const ElementImpl * getChild(size_t idx) const
        { return mElement->children()[idx]->impl(); }

        virtual ElementImpl * getChild(size_t idx)
        { return mElement->children()[idx]->impl(); }

        virtual void rebuildChildLayouts()
        { return Super::rebuildChildLayouts(); }
        
        virtual void setAttributeController(const std::string & inAttr, AttributeController * inController);

        //virtual void setOldAttributeController(const std::string & inAttr, const OldAttributeController & inController);

    };


    class NativeMenuList : public NativeControl
    {
    public:
        typedef NativeControl Super;

        NativeMenuList(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);
            
        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        virtual void move(int x, int y, int w, int h);

        void addMenuItem(const std::string & inText);

        void removeMenuItem(const std::string & inText);
    };


    class NativeSeparator : public NativeControl
    {
    public:
        typedef NativeControl Super;

        NativeSeparator(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeSpacer : public VirtualControl
    {
    public:
        typedef VirtualControl Super;

        NativeSpacer(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeMenuButton : public NativeControl
    {
    public:
        typedef NativeControl Super;

        NativeMenuButton(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeGrid : public VirtualControl
    {
    public:
        typedef VirtualControl Super;

        NativeGrid(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        virtual void rebuildLayout();
    };


    class NativeRows : public VirtualControl
    {
    public:
        typedef VirtualControl Super;

        NativeRows(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);
    };


    class NativeRow : public VirtualControl
    {
    public:
        typedef VirtualControl Super;

        NativeRow(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeColumns : public VirtualControl
    {
    public:
        typedef VirtualControl Super;

        NativeColumns(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);
    };


    class NativeColumn : public VirtualControl
    {
    public:
        typedef VirtualControl Super;

        NativeColumn(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeRadioGroup : public VirtualBox
    {
    public:
        typedef VirtualBox Super;

        NativeRadioGroup(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);
    };


    class NativeRadio : public NativeControl
    {
    public:
        typedef NativeControl Super;

        NativeRadio(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;
    };


    class NativeProgressMeter : public NativeControl,
                                public virtual IntValueController
    {
    public:
        typedef NativeControl Super;

        NativeProgressMeter(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        // IntValueController methods
        virtual int getValue() const;

        virtual void setValue(int inValue);

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        bool initAttributeControllers();
    };


    class NativeDeck : public VirtualControl,
                       public virtual SelectedIndexController
    {
    public:
        typedef VirtualControl Super;

        NativeDeck(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        // SelectedIndexController methods
        virtual int getSelectedIndex() const;

        virtual void setSelectedIndex(int inSelectedIndex);

        virtual void rebuildLayout();

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        bool initAttributeControllers();

    private:
        int mSelectedIndex;
    };


    class NativeScrollbar : public NativeControl,
                            public virtual ScrollbarCurrentPositionController,
                            public virtual ScrollbarMaxPositionController,
                            public virtual ScrollbarIncrementController,
                            public virtual ScrollbarPageIncrementController
    {
    public:
        typedef NativeControl Super;

        NativeScrollbar(ElementImpl * inParent, const AttributesMapping & inAttributesMapping);

        virtual int getCurrentPosition() const;

        virtual void setCurrentPosition(int inCurrentPosition);

        virtual int getMaxPosition() const;

        virtual void setMaxPosition(int inMaxPosition);

        virtual int getIncrement() const;

        virtual void setIncrement(int inIncrement);

        virtual int getPageIncrement() const;

        virtual void setPageIncrement(int inPageIncrement);

        class EventHandler
        {
        public:
            virtual bool curposChanged(NativeScrollbar * inSender, int inOldPos, int inNewPos) = 0;
        };

        EventHandler * eventHandler() { return mEventHandler; }

        void setEventHandler(EventHandler * inEventHandler)
        { mEventHandler = inEventHandler; }

        virtual int calculateMinimumWidth() const;

        virtual int calculateMinimumHeight() const;

        bool initAttributeControllers();

        virtual LRESULT handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam);

    private:
        static DWORD GetFlags(const AttributesMapping & inAttributesMapping);

        EventHandler * mEventHandler;
        int mIncrement;
    };


} // namespace XULWin


#endif // NATIVECOMPONENT_H_INCLUDED
