#ifndef BOX_RENDER_GLRENDERER_HPP
#define BOX_RENDER_GLRENDERER_HPP

#include <Brick/Component.hpp>
#include <Box/DocumentInterface.hpp>
#include <Stick/Error.hpp>
#include <Stick/UniquePtr.hpp>
#include <Crunch/RectanglePacker.hpp>
#include <Crunch/Colors.hpp>

namespace box
{
    namespace render
    {
        // The rendering Pipeline:
        // ======================================================
        // 1. Simple geometry and images are rendered on the fly.
        // 2. text and complex geometry are rastered on their own layers. (A layer is just a sub area in a texture atlas)
        // 3. animated items are rendered on their own layers.

        class STICK_API GLRenderBuffer
        {
        public:

            GLRenderBuffer();

            stick::Error initialize(stick::Size _width, stick::Size _height, stick::UInt32 _sampleCount);

            void deallocate();


            stick::UInt32 textureHandle;
            stick::UInt32 fboHandle;
            stick::UInt32 msaaFboHandle;
            stick::UInt32 msaaRenderBuffer;
        };

        class STICK_API TextureAtlasPage
        {
        public:

            TextureAtlasPage();

            stick::Error initialize(stick::Size _width, stick::Size _height, stick::UInt32 _sampleCount);

            void deallocate();

            crunch::RectanglePacker packer;
            GLRenderBuffer renderBuffer;
        };

        struct STICK_API TextureAtlasRegion
        {
            TextureAtlasPage * page;
            crunch::Rectf region;
        };

        struct RenderLayer;
        using RenderLayerPtr = stick::UniquePtr<RenderLayer>;

        struct STICK_API RenderLayer
        {
            using ChildArray = stick::DynamicArray<RenderLayerPtr>;
            using NodeArray = stick::DynamicArray<brick::Entity>;

            RenderLayer();

            void markDirty(bool _b);

            void markChildrenDirty(bool _b);

            void addNode(brick::Entity _node);

            bool removeNode(brick::Entity _node);

            void addChild(RenderLayer * _child);

            void removeChild(RenderLayer * _child);

            const NodeArray & nodes() const;

            const ChildArray & children() const;

            bool isDirty() const;

            bool childrenDirty() const;


        private:

            bool m_bDirty;
            bool m_bChildrenDirty;
            NodeArray m_nodes;
            RenderLayer * m_parent;
            ChildArray m_children;
            TextureAtlasRegion m_textureRegion;
        };


        namespace comps
        {
            using NeedsRedraw = brick::Component<ComponentName("NeedsRedraw"), bool>;
            using BgTextureAtlasRect = brick::Component<ComponentName("BgTextureAtlasRect"), crunch::Rectf>;
            using RenderLayer = brick::Component<ComponentName("RenderLayer"), RenderLayer *>;
        }

        class STICK_API GLRenderer : public DocumentInterface
        {
        public:

            GLRenderer();

            ~GLRenderer();

            //@TODO: Allow to pass in allocator, texture atlas page width,height,samplecount
            stick::Error initialize(brick::Entity _document);

            stick::Error render();


            //overwritten from DocumentInterface
            void markDocumentForRendering() final;
            void markNodeForRendering(brick::Entity _hint) final;
            void addedNode(brick::Entity _node) final;
            void removedNode(brick::Entity _node) final;
            void reversedChildren(brick::Entity _node) final;
            void nodeIsAnimatedChanged(brick::Entity _node) final;

            //function to determine if a newly discovered node should sit on its own RenderLayer
            bool shouldHaveItsOwnRenderLayer(brick::Entity _node);

        private:

            stick::Error drawToLayers();

            stick::Error compositeLayers();

            stick::Error recursivelyDrawLayers(RenderLayer & _layer);

            stick::Error recursivelyDrawNode(brick::Entity _e);

            //used by recursivelyDrawNode to draw individual rectangles
            void drawRectangle(const crunch::Rectf & _rect, const crunch::ColorRGBA & _color);

            brick::Entity m_document;
            stick::UInt32 m_textureProgram;
            stick::UInt32 m_colorProgram;
            stick::UInt32 m_vao;
            stick::UInt32 m_vbo;
            stick::UInt32 m_defaultSampler;

            //@TODO: add some FBO & crunch::RectanglePacker based texture atlas that renders all the items to
            //that texture. That would allow to redraw the whole document with minimal
            //draw calls. 1 draw call in the case we can squeeze everyting in one texture.

            TextureAtlasPage m_textureAtlas;
            RenderLayerPtr m_rootLayer;
        };
    }
}

#endif //BOX_RENDER_GLRENDERER_HPP
