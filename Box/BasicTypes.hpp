#ifndef BOX_BASICTYPES_HPP
#define BOX_BASICTYPES_HPP

#include <Stick/String.hpp>
#include <Crunch/Vector2.hpp>
#include <Crunch/Line.hpp>
#include <Crunch/Matrix3.hpp>
#include <Crunch/Matrix4.hpp>
#include <Crunch/Rectangle.hpp>
#include <Crunch/Colors.hpp>

namespace box
{
    using Float = stick::Float32;
    using Vec2f = crunch::Vector2<Float>;
    using Mat3f = crunch::Matrix3<Float>;
    using Mat4f = crunch::Matrix4<Float>;
    using Rect = crunch::Rectangle<Float>;
    using Line = crunch::Line<Vec2f>;
    using ColorRGB = crunch::ColorRGB;
    using ColorRGBA = crunch::ColorRGBA;
    using ColorHSB = crunch::ColorHSB;
    using ColorHSBA = crunch::ColorHSBA;
}

#endif //PAPER_BASICTYPES_HPP
