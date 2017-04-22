#ifndef BOX_BOX_HPP
#define BOX_BOX_HPP

#include <Box/BasicTypes.hpp>
#include <Box/Constants.hpp>
#include <Brick/Entity.hpp>
#include <Brick/Component.hpp>
#include <Stick/DynamicArray.hpp>
#include <Stick/Error.hpp>

#include <cmath>

//#include <functional>

namespace box
{
    using EntityArray = stick::DynamicArray<brick::Entity>;
    constexpr Float undefined = std::numeric_limits<Float>::quiet_NaN();

    namespace detail
    {
        STICK_API void removeFromParent(brick::Entity _e);
        STICK_API void removeImpl(brick::Entity _e, bool _bRemoveFromParent);

        struct STICK_API ComputedLayout
        {
            Rect box;
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

    namespace comps
    {
        using Parent = brick::Component<ComponentName("Parent"), brick::Entity>;
        using Name = brick::Component<ComponentName("Name"), stick::String>;
        using Children = brick::Component<ComponentName("Children"), EntityArray>;
        using HubPointer = brick::Component<ComponentName("HubPointer"), brick::Hub *>;
        using HasNewLayout = brick::Component<ComponentName("HasNewLayout"), bool>;
        using Dirty = brick::Component<ComponentName("Dirty"), bool>;
        using ComputedLayout = brick::Component<ComponentName("ComputedLayout"), detail::ComputedLayout>;

        //styling components
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
        using Align = brick::Component<ComponentName("Align"), box::Align>;
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
    }

    STICK_API bool isUndefined(Float _value);
    STICK_API brick::Hub & defaultHub();
    STICK_API brick::Entity createNode(const stick::String & _name = "", brick::Hub & _hub = defaultHub());
    STICK_API void addChild(brick::Entity _e, brick::Entity _child);
    STICK_API bool removeChild(brick::Entity _e, brick::Entity _child);
    STICK_API void removeChildren(brick::Entity _e);
    STICK_API void reverseChildren(brick::Entity _e);
    STICK_API void remove(brick::Entity _e);
    STICK_API stick::Error layout(brick::Entity _e, Float _width = undefined, Float _height = undefined);

    // STICK_API void appendToDebugString(
    //                             const brick::Entity & _e,
    //                             stick::Size _indentLevel,
    //                             DebugStringOptions _options = DebugStringOptions::ComputedLayout | DebugStringOptions::Style | DebugStringOptions::Children);

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
}

#endif //BOX_BOX_HPP
