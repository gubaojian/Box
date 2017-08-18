#ifndef BOX_RENDER_VERTEX_HPP
#define BOX_RENDER_VERTEX_HPP

#include <Crunch/Vector2.hpp>
#include <Crunch/Colors.hpp>

namespace box
{
    namespace render
    {
        struct STICK_API Vertex
        {
            crunch::Vec2f position;
            crunch::ColorRGBA color;
            crunch::Vec2f textureCoordinates;
        };
    }
}

#endif //BOX_RENDER_VERTEX_HPP
