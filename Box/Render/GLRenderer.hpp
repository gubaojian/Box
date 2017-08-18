#ifndef BOX_RENDER_GLRENDERER_HPP
#define BOX_RENDER_GLRENDERER_HPP

#include <Brick/Component.hpp>
#include <Box/DocumentInterface.hpp>
#include <Stick/Error.hpp>

namespace box
{
    namespace render
    {
        namespace comps
        {
            using NeedsRedraw = brick::Component<ComponentName("NeedsRedraw"), bool>;
        }

        class STICK_API GLRenderer : public DocumentInterface
        {
        public:
            
            GLRenderer();
            
            ~GLRenderer();

            stick::Error initialize(brick::Entity _document);

            stick::Error render();


            //overwritten from DocumentInterface
            void markDocumentForRendering() final;

            void markNodeForRendering(brick::Entity _hint) final;

        private:

            stick::Error recursivelyDrawNode(brick::Entity _e);

            brick::Entity m_document;
            stick::UInt32 m_textureProgram;
            stick::UInt32 m_colorProgram;
            stick::UInt32 m_vao;
            stick::UInt32 m_vbo;

            //@TODO: add some FBO & crunch::RectanglePacker based texture atlas that renders all the items to
            //that texture. That would allow to redraw the whole document with minimal
            //draw calls. 1 draw call in the case we can squeeze everyting in one texture.
        };
    }
}

#endif //BOX_RENDER_GLRENDERER_HPP
