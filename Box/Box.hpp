#ifndef BOX_BOX_HPP
#define BOX_BOX_HPP

#include <Brick/Entity.hpp>
#include <Brick/Component.hpp>
#include <Stick/DynamicArray.hpp>
#include <Stick/Error.hpp>
#include <Stick/Variant.hpp>
#include <Stick/URI.hpp>
#include <Box/BasicTypes.hpp>
#include <Box/Constants.hpp>
#include <Box/DocumentEvents.hpp>
#include <Box/EventForwarder.hpp>

#include <cmath>

#include <functional>

namespace box
{
    using EntityArray = stick::DynamicArray<brick::Entity>;
    constexpr Float undefined = std::numeric_limits<Float>::quiet_NaN();

    class DocumentInterface;

    namespace detail
    {
        STICK_API void removeFromParent(brick::Entity _e);
        STICK_API void removeImpl(brick::Entity _e, bool _bRemoveFromParent);

        struct STICK_API ComputedLayout
        {
            Rect box;
            Float marginLeft;
            Float marginTop;
            Float marginRight;
            Float marginBottom;
            bool bFixedWidth;
            bool bFixedHeight;
            stick::Size generation;
        };
    }

    struct STICK_API Value
    {
        Value()
        {
        }

        Value(Float _value, Unit _unit = Unit::Pixels) :
            value(_value),
            unit(_unit)
        {
        }

        Float value;
        Unit unit;
    };

    // class STICK_API Document
    // {
    // public:

    //     Document(brick::Hub & _hub = defaultHub());


    //     brick::Entity root();

    // private:
    // };

    enum class DirtyFlag
    {
        NotDirty,
        Dirty,
        ChildrenDirty,
        PositionDirty
    };

    struct STICK_API BackgroundImage
    {
        stick::URI uri;
        BackgroundImageWrap wrap;
    };

    using EventHandler = EventForwarderT<Event, detail::ForwardingPolicyBasic, detail::PublishingPolicyBasic, brick::Entity>;
    using NodeTag = stick::Variant<TagType, stick::String>;

    namespace comps
    {
        using DocumentInterface = brick::Component<ComponentName("DocumentInterface"), DocumentInterface*>;
        using Document = brick::Component<ComponentName("Document"), brick::Entity>;
        using Parent = brick::Component<ComponentName("Parent"), brick::Entity>;
        using Name = brick::Component<ComponentName("Name"), stick::String>;
        using Class = brick::Component<ComponentName("Class"), stick::String>;
        using Children = brick::Component<ComponentName("Children"), EntityArray>;
        using HubPointer = brick::Component<ComponentName("HubPointer"), brick::Hub *>;
        using Dirty = brick::Component<ComponentName("Dirty"), DirtyFlag>;
        using ComputedLayout = brick::Component<ComponentName("ComputedLayout"), detail::ComputedLayout>;
        using Tag = brick::Component<ComponentName("Tag"), NodeTag>;

        //styling components that affect the layouting algorithm
        using Width = brick::Component<ComponentName("Width"), Value>;
        using Height = brick::Component<ComponentName("Height"), Value>;
        using MaxWidth = brick::Component<ComponentName("MaxWidth"), Value>;
        using MaxHeight = brick::Component<ComponentName("MaxHeight"), Value>;
        using MinWidth = brick::Component<ComponentName("MinWidth"), Value>;
        using MinHeight = brick::Component<ComponentName("MinHeight"), Value>;
        using Overflow = brick::Component<ComponentName("Overflow"), box::Overflow>;
        using Flow = brick::Component<ComponentName("Flow"), box::Flow>;
        using Direction = brick::Component<ComponentName("Direction"), box::Direction>;
        using Wrap = brick::Component<ComponentName("Wrap"), box::Wrap>;
        using Justify = brick::Component<ComponentName("Justify"), box::Justify>;
        using AlignItems = brick::Component<ComponentName("AlignItems"), box::AlignItems>;
        using AlignLines = brick::Component<ComponentName("AlignLines"), box::AlignLines>;
        using Grow = brick::Component<ComponentName("Grow"), Float>;
        using Shrink = brick::Component<ComponentName("Shrink"), Float>;
        using Position = brick::Component<ComponentName("Position"), box::Position>;
        using Left = brick::Component<ComponentName("Left"), Value>;
        using Top = brick::Component<ComponentName("Top"), Value>;
        using Right = brick::Component<ComponentName("Right"), Value>;
        using Bottom = brick::Component<ComponentName("Bottom"), Value>;
        using PaddingLeft = brick::Component<ComponentName("PaddingLeft"), Value>;
        using PaddingTop = brick::Component<ComponentName("PaddingTop"), Value>;
        using PaddingRight = brick::Component<ComponentName("PaddingRight"), Value>;
        using PaddingBottom = brick::Component<ComponentName("PaddingBottom"), Value>;
        using MarginLeft = brick::Component<ComponentName("MarginLeft"), Value>;
        using MarginTop = brick::Component<ComponentName("MarginTop"), Value>;
        using MarginRight = brick::Component<ComponentName("MarginRight"), Value>;
        using MarginBottom = brick::Component<ComponentName("MarginBottom"), Value>;

        //styling components that are independant of layouting
        using Background = brick::Component<ComponentName("Background"), stick::Variant<crunch::ColorRGBA, BackgroundImage>>;

        //event handling components
        using EventHandler = brick::Component<ComponentName("EventHandler"), stick::UniquePtr<EventHandler>>;
        using MouseOn = brick::Component<ComponentName("EventHandler"), bool>;
    }

    // STICK_API void markForRendering(brick::Entity _e);
    // STICK_API brick::Entity rootNode(brick::Entity _e);

    STICK_API void markDirty(brick::Entity _e);
    STICK_API bool isUndefined(Float _value);
    STICK_API brick::Hub & defaultHub();
    STICK_API brick::Entity createNode(brick::Entity _document, const stick::String & _name = "");
    STICK_API brick::Entity createDocument(DocumentInterface * _interface = nullptr, brick::Hub & _hub = defaultHub());
    STICK_API void addChild(brick::Entity _e, brick::Entity _child);
    STICK_API bool removeChild(brick::Entity _e, brick::Entity _child);
    STICK_API void removeChildren(brick::Entity _e);
    STICK_API void reverseChildren(brick::Entity _e);
    STICK_API void remove(brick::Entity _e);
    STICK_API stick::Error layout(brick::Entity _e, Float _width = undefined, Float _height = undefined);

    STICK_API void setTag(brick::Entity _e, const stick::String & _tag);
    STICK_API void setSize(brick::Entity _e, Value _width, Value _height);
    STICK_API void setWidth(brick::Entity _e, Value _w);
    STICK_API void setHeight(brick::Entity _e, Value _h);

    STICK_API void setMinSize(brick::Entity _e, Value _width, Value _height);
    STICK_API void setMinWidth(brick::Entity _e, Value _w);
    STICK_API void setMinHeight(brick::Entity _e, Value _h);

    STICK_API void setMaxSize(brick::Entity _e, Value _width, Value _height);
    STICK_API void setMaxWidth(brick::Entity _e, Value _w);
    STICK_API void setMaxHeight(brick::Entity _e, Value _h);

    STICK_API void setPadding(brick::Entity _e, Value _value);
    STICK_API void setPadding(brick::Entity _e, Value _top, Value _right, Value _bottom, Value _left);
    STICK_API void setPadding(brick::Entity _e, Value _top, Value _right, Value _bottom);
    STICK_API void setPadding(brick::Entity _e, Value _vertical, Value _horizontal);
    STICK_API void setPaddingLeft(brick::Entity _e, Value _value);
    STICK_API void setPaddingTop(brick::Entity _e, Value _value);
    STICK_API void setPaddingRight(brick::Entity _e, Value _value);
    STICK_API void setPaddingBottom(brick::Entity _e, Value _value);

    STICK_API void setMargin(brick::Entity _e, Value _value);
    STICK_API void setMargin(brick::Entity _e, Value _top, Value _right, Value _bottom, Value _left);
    STICK_API void setMargin(brick::Entity _e, Value _top, Value _right, Value _bottom);
    STICK_API void setMargin(brick::Entity _e, Value _vertical, Value _horizontal);
    STICK_API void setMarginLeft(brick::Entity _e, Value _value);
    STICK_API void setMarginTop(brick::Entity _e, Value _value);
    STICK_API void setMarginRight(brick::Entity _e, Value _value);
    STICK_API void setMarginBottom(brick::Entity _e, Value _value);

    STICK_API CallbackID addEventCallback(brick::Entity _e, const EventHandler::Callback & _cb);
    STICK_API void removeEventCalback(brick::Entity _e, CallbackID _id);

    STICK_API brick::Entity nodeAtPosition(brick::Entity _e, Float _x, Float _y);

    // STICK_API void appendToDebugString(
    //                             const brick::Entity & _e,
    //                             stick::Size _indentLevel,
    //                             DebugStringOptions _options = DebugStringOptions::ComputedLayout | DebugStringOptions::Style | DebugStringOptions::Children);


    template<class T>
    void publish(brick::Entity _e, const T & _event, bool _bPropagate)
    {
        STICK_ASSERT(_e.hasComponent<comps::EventHandler>());
        printf("DO DA PUBLISH OOOOOOOOO\n");
        _e.get<comps::EventHandler>()->publish(_event, _bPropagate);
    }

    template<class T, class ...Args>
    void setComponent(brick::Entity _e, Args..._args)
    {
        _e.set<T>(std::forward<Args>(_args)...);
        publish(_e, ComponentChangedEvent(stick::TypeInfoT<T>::typeID(), T::name()), false);
    }

    template<class C>
    stick::Maybe<typename C::ValueType &> findComponent(brick::Entity _e)
    {
        brick::Entity i(_e);
        while (i.isValid())
        {
            auto maybe = i.maybe<C>();
            if (maybe)
                return maybe;
            i = i.get<comps::Parent>();
        }
        return stick::Maybe<typename C::ValueType &>();
    }

    template<class F>
    brick::Entity findChild(brick::Entity _e, F _func)
    {
        auto mc = _e.maybe<comps::Children>();
        if (mc)
        {
            for (brick::Entity c : *mc)
            {
                if (_func(c))
                    return c;
            }
        }

        return brick::Entity();
    }

    template<class F>
    brick::Entity findChildRecursively(brick::Entity _e, F _func)
    {
        //@TODO: instead of a recursive function call make this iterative to
        // potentially avoid running into max stack depth errors?
        auto mc = _e.maybe<comps::Children>();
        if (mc)
        {
            for (brick::Entity c : *mc)
            {
                if (_func(c))
                    return c;
                auto ret = findChildRecursively(c, _func);
                if (ret) return ret;
            }
        }

        return brick::Entity();
    }

    template<class F>
    void applyRecursively(brick::Entity _e, F _func)
    {
        //@TODO: instead of a recursive function call make this iterative to
        // potentially avoid running into max stack depth errors?
        auto mc = _e.maybe<comps::Children>();
        if (mc)
        {
            for (brick::Entity c : *mc)
            {
                _func(c);
                applyRecursively(c, _func);
            }
        }
    }

    STICK_API brick::Entity findByName(brick::Entity _e, const stick::String & _name);
    STICK_API void findByClass(brick::Entity _e, const stick::String & _className, EntityArray & _outResults);
}

#endif //BOX_BOX_HPP
