#ifndef BOX_CONSTANTS_HPP
#define BOX_CONSTANTS_HPP

#include <Stick/Platform.hpp>
#include <Stick/Utility.hpp>

namespace box
{
    STICK_API_ENUM_CLASS(Unit)
    {
        Pixels,
        Percent
    };

    STICK_API_ENUM_CLASS(Overflow)
    {
        Visible,
        Hidden,
        Scroll
    };

    STICK_API_ENUM_CLASS(Flow)
    {
        Row,
        RowReverse,
        Column,
        ColumnReverse
    };

    STICK_API_ENUM_CLASS(Direction)
    {
        LeftToRight,
        RightToLeft
    };

    STICK_API_ENUM_CLASS(Wrap)
    {
        None,
        Normal,
        Reverse
    };

    STICK_API_ENUM_CLASS(Justify)
    {
        Start,
        Center,
        End,
        SpaceBetween,
        SpaceAround
    };

    STICK_API_ENUM_CLASS(AlignItems)
    {
        Start,
        Center,
        End,
        Stretch
    };

    STICK_API_ENUM_CLASS(AlignLines)
    {
        Start,
        Center,
        End,
        SpaceBetween,
        SpaceAround,
        Stretch
    };

    STICK_API_ENUM_CLASS(Position)
    {
        Relative,
        Absolute
    };

    STICK_API_ENUM_CLASS(TagType)
    {
        Box,
        Image,
        Custom
    };

    STICK_API_ENUM_CLASS(DebugStringOptions)
    {
        ComputedLayout = 1 << 0,
        Style = 1 << 1,
        Children = 1 << 2
    };

    STICK_API_ENUM_CLASS(BackgroundImageWrap)
    {
        Repeat,
        Stretch,
        Cover,
        Once
    };

    // enable bit masking for that enum
    static constexpr bool enableBitmaskOperators(DebugStringOptions)
    {
        return true;
    }
}

#endif //BOX_CONSTANTS_HPP
