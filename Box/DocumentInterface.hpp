#ifndef BOX_DOCUMENTINTERFACE_HPP
#define BOX_DOCUMENTINTERFACE_HPP

#include <Stick/Platform.hpp>
#include <Brick/Entity.hpp>

namespace box
{
    class STICK_API DocumentInterface
    {
    public:

        virtual ~DocumentInterface() = default;

        virtual void markDocumentForRendering() = 0;
        virtual void markNodeForRendering(brick::Entity) = 0;
    };
}

#endif //BOX_DOCUMENTINTERFACE_HPP
