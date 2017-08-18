#ifndef BOX_RENDER_RENDERINTERFACE_HPP
#define BOX_RENDER_RENDERINTERFACE_HPP

#include <Box/EventPublisher.hpp>
#include <Box/Render/Vertex.hpp>

namespace box
{
    namespace render
    {
        struct NativeTextureHandle;
        struct NativeRenderBufferHandle;

        STICK_API_ENUM_CLASS(DrawMode)
        {
            Triangles,
            TriangleStripe,
            TriangleFan
        };

        class STICK_API RenderInterface : public EventPublisherT<Event, detail::PublishingPolicyBasic>
        {
        public:

            RenderInterface() = default;
            virtual ~RenderInterface() = default;


            virtual NativeRenderBufferHandle createRenderBuffer(stick::Size _width, stick::Size _height);

            virtual void destroyRenderBuffer(NativeRenderBufferHandle _handle);


            virtual void drawGeometry(Vertex * _data, stick::Size _vertexCount, DrawMode _drawMode, const crunch::Mat4f * _transformation) = 0;

            virtual NativeTextureHandle createTexture(stick::Size _width, stick::Size _height);

            virtual void destroyTexture(NativeTextureHandle _handle);

            virtual stick::Error loadTexture(NativeTextureHandle & _handle, const stick::URI & _source) = 0;

            virtual void update() {}
        };
    }
}

#endif //BOX_RENDER_RENDERINTERFACE_HPP
