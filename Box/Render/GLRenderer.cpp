#include <Box/Render/GLRenderer.hpp>
#include <Box/Box.hpp>
#include <Crunch/Vector2.hpp>

#if STICK_PLATFORM == STICK_PLATFORM_OSX
#include <OpenGL/OpenGL.h> //for CGL functions
//check if opengl 3+ is available
#if CGL_VERSION_1_3
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif //CGL_VERSION_1_3
#elif STICK_PLATFORM == STICK_PLATFORM_LINUX
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#undef None //??? some glx thing
#undef GL_GLEXT_PROTOTYPES
#undef WindingRule //????? where is this a macro on linux?
#endif

#ifdef STICK_DEBUG
#define ASSERT_NO_GL_ERROR(_func) do { _func; \
GLenum err = glGetError(); \
if(err != GL_NO_ERROR) \
{ \
switch(err) \
{ \
case GL_NO_ERROR: \
printf("%s line %i GL_NO_ERROR: No error has been recorded.\n", __FILE__, __LINE__);\
break; \
case GL_INVALID_ENUM: \
printf("%s line %i GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.\n", __FILE__, __LINE__);\
break; \
case GL_INVALID_VALUE: \
printf("%s line %i GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.\n", __FILE__, __LINE__);\
break; \
case GL_INVALID_OPERATION: \
printf("%s line %i GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.\n", __FILE__, __LINE__);\
break; \
case GL_INVALID_FRAMEBUFFER_OPERATION: \
printf("%s line %i GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag.\n", __FILE__, __LINE__);\
break; \
case GL_OUT_OF_MEMORY: \
printf("%s line %i GL_OUT_OF_MEMORY: There is not enough memory left to executeLua the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.\n", __FILE__, __LINE__);\
break; \
} \
exit(EXIT_FAILURE); \
} \
} while(false)
#else
#define ASSERT_NO_GL_ERROR(_func) _func
#endif

namespace box
{
    namespace render
    {
        using namespace stick;
        using namespace brick;
        using namespace crunch;

        //The shader programs used by the renderer
        static String vertexShaderCode =
            "#version 150 \n"
            "uniform mat4 transformProjection; \n"
            "in vec2 vertex; \n"
            "in vec4 color; \n"
            "out vec4 icol;\n"
            "void main() \n"
            "{ \n"
            "gl_Position = transformProjection * vec4(vertex, 0.0, 1.0); \n"
            "icol = color;\n"
            "} \n";

        static String fragmentShaderCode =
            "#version 150 \n"
            "in vec4 icol; \n"
            "out vec4 pixelColor; \n"
            "void main() \n"
            "{ \n"
            "pixelColor = icol; \n"
            "} \n";

        namespace detail
        {
            static Error compileShader(const String & _shaderCode, GLenum _shaderType, GLuint & _outHandle)
            {
                Error ret;
                GLenum glHandle = glCreateShader(_shaderType);
                const char * cstr = _shaderCode.cString();
                GLint len = (GLint)_shaderCode.length();
                ASSERT_NO_GL_ERROR(glShaderSource(glHandle, 1, &cstr, &len));
                ASSERT_NO_GL_ERROR(glCompileShader(glHandle));

                //check if the shader compiled
                GLint state;
                ASSERT_NO_GL_ERROR(glGetShaderiv(glHandle, GL_COMPILE_STATUS, &state));
                if (state == GL_FALSE)
                {
                    GLint infologLength;
                    ASSERT_NO_GL_ERROR(glGetShaderiv(glHandle, GL_INFO_LOG_LENGTH, &infologLength));

                    char * str = (char *)malloc(infologLength);
                    ASSERT_NO_GL_ERROR(glGetShaderInfoLog(glHandle, infologLength, &infologLength, str));

                    ret = Error(ec::InvalidArgument, String::concat("Could not compile GLSL shader: ", str), STICK_FILE, STICK_LINE);
                    glDeleteShader(glHandle);
                    free(str);
                }
                else
                {
                    _outHandle = glHandle;
                }
                return ret;
            }

            static Error createProgram(const String & _vertexShader, const String & _fragmentShader, std::initializer_list<const char *> _attr, GLuint & _outHandle)
            {
                GLuint vertexShader, fragmentShader;
                Error err = compileShader(_vertexShader, GL_VERTEX_SHADER, vertexShader);
                if (!err)
                {
                    err = compileShader(_fragmentShader, GL_FRAGMENT_SHADER, fragmentShader);
                }
                if (err) return err;

                GLuint program = glCreateProgram();

                ASSERT_NO_GL_ERROR(glAttachShader(program, vertexShader));
                ASSERT_NO_GL_ERROR(glAttachShader(program, fragmentShader));

                //bind the attribute locations from the layout
                /*const BufferLayout::BufferElementArray & elements = _settings.vertexLayout.elements();
                auto sit = elements.begin();

                for (; sit != elements.end(); ++sit)
                {
                    ASSERT_NO_GL_ERROR(glBindAttribLocation(program, (*sit).m_location, (*sit).m_name.cString()));
                }*/

                Size i = 0;
                for (const char * attr : _attr)
                {
                    printf("ENABLE ATTR %s %i\n", attr, i);
                    ASSERT_NO_GL_ERROR(glBindAttribLocation(program, i, attr));
                    ++i;
                }

                ASSERT_NO_GL_ERROR(glBindAttribLocation(program, 0, "vertex"));

                ASSERT_NO_GL_ERROR(glLinkProgram(program));

                //check if we had success
                GLint state;
                ASSERT_NO_GL_ERROR(glGetProgramiv(program, GL_LINK_STATUS, &state));

                if (state == GL_FALSE)
                {
                    GLint infologLength;
                    ASSERT_NO_GL_ERROR(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength));

                    char * str = (char *)malloc(infologLength);
                    ASSERT_NO_GL_ERROR(glGetProgramInfoLog(program, infologLength, &infologLength, str));

                    err = Error(ec::InvalidArgument, String::concat("Error linking GLSL program: ", str), STICK_FILE, STICK_LINE);
                    free(str);
                }

                ASSERT_NO_GL_ERROR(glDeleteShader(vertexShader));
                ASSERT_NO_GL_ERROR(glDeleteShader(fragmentShader));

                if (err)
                {
                    glDeleteProgram(program);
                }
                else
                {
                    _outHandle = program;
                }

                return err;
            }

            static Error validateFrameBuffer()
            {
                GLenum err;
                err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                switch (err)
                {
                    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                        return Error(ec::InvalidOperation, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: Invalid OpenGL FBO attachment!", STICK_FILE, STICK_LINE);
                    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                        return Error(ec::InvalidOperation, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: OpenGL FBO has no attachments!", STICK_FILE, STICK_LINE);
                    case GL_FRAMEBUFFER_UNSUPPORTED:
                        return Error(ec::InvalidOperation, "GL_FRAMEBUFFER_UNSUPPORTED: The OpenGL format combination is not supported by FBOs on your platform!", STICK_FILE, STICK_LINE);
                    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                        return Error(ec::InvalidOperation, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER!", STICK_FILE, STICK_LINE);
                    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                        return Error(ec::InvalidOperation, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER!", STICK_FILE, STICK_LINE);
                }

                return Error();
            }
        }

        GLRenderBuffer::GLRenderBuffer() :
            textureHandle(0),
            fboHandle(0),
            msaaFboHandle(0),
            msaaRenderBuffer(0)
        {

        }

        Error GLRenderBuffer::initialize(Size _width, Size _height, UInt32 _sampleCount)
        {
            ASSERT_NO_GL_ERROR(glGenFramebuffers(1, &fboHandle));

            ASSERT_NO_GL_ERROR(glGenTextures(1, &textureHandle));

            ASSERT_NO_GL_ERROR(glActiveTexture(GL_TEXTURE0));
            ASSERT_NO_GL_ERROR(glBindTexture(GL_TEXTURE_2D, textureHandle));

            ASSERT_NO_GL_ERROR(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLuint)_width, (GLuint)_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));

            ASSERT_NO_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, fboHandle));
            ASSERT_NO_GL_ERROR(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0));

            Error ret = detail::validateFrameBuffer();

            if (!ret && _sampleCount)
            {
                ASSERT_NO_GL_ERROR(glGenFramebuffers(1, &msaaFboHandle));

                GLint maxSamples;
                ASSERT_NO_GL_ERROR(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

                ASSERT_NO_GL_ERROR(glGenFramebuffers(1, &msaaFboHandle));
                ASSERT_NO_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, msaaFboHandle));

                ASSERT_NO_GL_ERROR(glGenRenderbuffers(1, &msaaRenderBuffer));
                ASSERT_NO_GL_ERROR(glBindRenderbuffer(GL_RENDERBUFFER, msaaRenderBuffer));
                ASSERT_NO_GL_ERROR(glRenderbufferStorageMultisample(GL_RENDERBUFFER, std::min((UInt32)maxSamples, _sampleCount), GL_RGBA8, (GLuint)_width, (GLuint)_height));
                ASSERT_NO_GL_ERROR(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaRenderBuffer));

                ret = detail::validateFrameBuffer();
            }

            ASSERT_NO_GL_ERROR(glBindTexture(GL_TEXTURE_2D, 0));
            ASSERT_NO_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));

            return ret;
        }

        void GLRenderBuffer::deallocate()
        {
            if (fboHandle)
                ASSERT_NO_GL_ERROR(glDeleteFramebuffers(1, &fboHandle));

            if (msaaFboHandle)
            {
                ASSERT_NO_GL_ERROR(glDeleteRenderbuffers(1,  &msaaRenderBuffer));
                ASSERT_NO_GL_ERROR(glDeleteFramebuffers(1, &msaaFboHandle));
            }

            if (textureHandle)
                ASSERT_NO_GL_ERROR(glDeleteTextures(1, &textureHandle));
        }

        TextureAtlasPage::TextureAtlasPage()
        {

        }

        Error TextureAtlasPage::initialize(Size _width, Size _height, UInt32 _sampleCount)
        {
            Error ret = renderBuffer.initialize(_width, _height, _sampleCount);
            if (!ret)
            {
                packer.setMaxSize(_width, _height);
                packer.reset(_width, _height);
            }
            return ret;
        }

        void TextureAtlasPage::deallocate()
        {
            renderBuffer.deallocate();
        }

        RenderLayer::RenderLayer() :
            m_bDirty(true),
            m_bChildrenDirty(true),
            m_parent(nullptr)
        {

        }

        void RenderLayer::markChildrenDirty(bool _b)
        {
            m_bChildrenDirty = _b;
            if (m_parent && _b)
                m_parent->markChildrenDirty(true);
        }

        void RenderLayer::markDirty(bool _b)
        {
            m_bDirty = _b;
            if (m_parent && _b)
                m_parent->markChildrenDirty(true);
        }

        void RenderLayer::addNode(Entity _node)
        {
            m_nodes.append(_node);
            markDirty(true);
        }

        bool RenderLayer::removeNode(brick::Entity _node)
        {
            auto it = std::find(m_nodes.begin(), m_nodes.end(), _node);
            if (it != m_nodes.end())
            {
                m_nodes.remove(it);
                markDirty(true);
            }
        }

        void RenderLayer::addChild(RenderLayer * _child)
        {
            if (_child->m_parent)
            {
                for (auto it = _child->m_parent->m_children.begin();
                        it != _child->m_parent->m_children.end();
                        ++it)
                {
                    if ((*it).get() == _child)
                    {
                        _child->m_parent = this;
                        m_children.append(std::move(*it));
                        _child->m_parent->m_children.remove(it);
                        break;
                    }
                }
            }
            else
            {
                _child->m_parent = this;
                m_children.append(RenderLayerPtr(_child, stick::defaultAllocator()));
            }
        }

        void RenderLayer::removeChild(RenderLayer * _child)
        {
            for (auto it = m_children.begin(); it != m_children.end(); ++it)
            {
                if ((*it).get() == _child)
                {
                    m_children.remove(it);
                    break;
                }
            }
        }

        bool RenderLayer::isDirty() const
        {
            return m_bDirty;
        }

        bool RenderLayer::childrenDirty() const
        {
            return m_bChildrenDirty;
        }

        const RenderLayer::NodeArray & RenderLayer::nodes() const
        {
            return m_nodes;
        }

        const RenderLayer::ChildArray & RenderLayer::children() const
        {
            return m_children;
        }

        GLRenderer::GLRenderer() :
            m_textureProgram(0),
            m_colorProgram(0),
            m_vao(0),
            m_vbo(0),
            m_defaultSampler(0)
        {

        }

        GLRenderer::~GLRenderer()
        {

        }

        Error GLRenderer::initialize(Entity _document)
        {
            m_document = _document;
            m_document.set<box::comps::DocumentInterface>(this);

            m_rootLayer = stick::makeUnique<RenderLayer>();
            m_document.set<comps::RenderLayer>(m_rootLayer.get());
            m_rootLayer->addNode(m_document);

            Error ret = detail::createProgram(vertexShaderCode, fragmentShaderCode, {"vertex", "color"}, m_colorProgram);
            if (ret) return ret;

            GLint ts;
            ASSERT_NO_GL_ERROR(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &ts));
            ts /= 8 * 2;
            printf("TEXTURE ATLAS SIZE %i\n", ts);
            ret = m_textureAtlas.initialize(ts, ts, 8);
            if (ret) return ret;

            ASSERT_NO_GL_ERROR(glGenVertexArrays(1, &m_vao));
            ASSERT_NO_GL_ERROR(glGenBuffers(1, &m_vbo));
            ASSERT_NO_GL_ERROR(glBindVertexArray(m_vao));
            ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
            ASSERT_NO_GL_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), ((char *)0)));
            ASSERT_NO_GL_ERROR(glEnableVertexAttribArray(0));
            ASSERT_NO_GL_ERROR(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), ((char *)0 + sizeof(float) * 2)));
            ASSERT_NO_GL_ERROR(glEnableVertexAttribArray(1));

            ASSERT_NO_GL_ERROR(glGenSamplers(1, &m_defaultSampler));
            ASSERT_NO_GL_ERROR(glSamplerParameteri(m_defaultSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
            ASSERT_NO_GL_ERROR(glSamplerParameteri(m_defaultSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            ASSERT_NO_GL_ERROR(glSamplerParameteri(m_defaultSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
            ASSERT_NO_GL_ERROR(glSamplerParameteri(m_defaultSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            ASSERT_NO_GL_ERROR(glSamplerParameteri(m_defaultSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

            return Error();
        }

        struct Vertex
        {
            Vec2f position;
            ColorRGBA color;
        };

        void GLRenderer::drawRectangle(const Rectf & _rect, const ColorRGBA & _color)
        {
            DynamicArray<Vertex> vertices =
            {
                {_rect.min(), _color},
                {_rect.min() + Vec2f(0, _rect.height()), _color},
                {_rect.min() + Vec2f(_rect.width(), 0), _color},
                {_rect.max(), _color}
            };
            ASSERT_NO_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, &vertices[0].position.x, GL_DYNAMIC_DRAW));
            ASSERT_NO_GL_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
        }

        Error GLRenderer::recursivelyDrawLayers(RenderLayer & _layer)
        {
            if (_layer.isDirty())
            {
                for (auto node : _layer.nodes())
                {
                    STICK_ASSERT(node.hasComponent<box::comps::ComputedLayout>());
                    // draw background
                    if (auto mbg = node.maybe<box::comps::Background>())
                    {
                        //solid background
                        if ((*mbg).is<crunch::ColorRGBA>())
                        {
                            auto & col = (*mbg).get<ColorRGBA>();
                            auto & box = node.get<box::comps::ComputedLayout>().box;
                            drawRectangle(box, col);
                        }
                        //background image
                        else if ((*mbg).is<BackgroundImage>())
                        {
                            //@TODO
                        }
                    }
                }
                _layer.markDirty(false);
            }
            if (_layer.childrenDirty())
            {
                for (auto & child : _layer.children())
                    recursivelyDrawLayers(*child);
                _layer.markChildrenDirty(false);
            }

            return Error();
        }

        Error GLRenderer::recursivelyDrawNode(Entity _e)
        {
            if (_e.hasComponent<comps::NeedsRedraw>())
            {
                STICK_ASSERT(_e.hasComponent<box::comps::ComputedLayout>());
                // draw background
                if (auto mbg = _e.maybe<box::comps::Background>())
                {
                    //check if we need to clear a previous rectangle in the
                    if (auto mbgrect = _e.maybe<comps::BgTextureAtlasRect>())
                    {
                        drawRectangle(*mbgrect, ColorRGBA(0, 0, 0, 0));
                    }
                    //solid background
                    if ((*mbg).is<crunch::ColorRGBA>())
                    {
                        auto & col = (*mbg).get<ColorRGBA>();
                        auto & box = _e.get<box::comps::ComputedLayout>().box;
                        drawRectangle(box, col);
                    }
                    //background image
                    else if ((*mbg).is<BackgroundImage>())
                    {
                        //@TODO
                    }
                }
            }

            if (auto mchildren = _e.maybe<box::comps::Children>())
            {
                for (Entity c : *mchildren)
                    recursivelyDrawNode(c);
            }

            // DynamicArray<Vertex> vertices = {}

            //draw border

            return Error();
        }

        Error GLRenderer::render()
        {
            printf("RENDER BRO %i %i\n", m_vao, m_vbo);
            ASSERT_NO_GL_ERROR(glUseProgram(m_colorProgram));
            ASSERT_NO_GL_ERROR(glBindVertexArray(m_vao));
            ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));

            auto m_transformProjection = crunch::Mat4f::ortho(0, 800, 600, 0, -1, 1);
            ASSERT_NO_GL_ERROR(glUniformMatrix4fv(glGetUniformLocation(m_colorProgram, "transformProjection"), 1, false, m_transformProjection.ptr()));
            // ASSERT_NO_GL_ERROR(glViewport(0, 0, m_viewport.x, m_viewport.y));

            // ASSERT_NO_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            // glDrawBuffer(GL_FRONT_AND_BACK);

            // recursivelyDrawNode(m_document);
            recursivelyDrawLayers(*m_rootLayer);

            // ASSERT_NO_GL_ERROR(glBindFramebuffer(GL_FRAMEBUFFER, m_textureAtlas.renderBuffer.msaaFboHandle));
            // ASSERT_NO_GL_ERROR(glDrawBuffer(GL_COLOR_ATTACHMENT0));

            // @TODO: Traverse the document tree twice, once to redraw all dirty nodes to a texture
            // and a second time to actually draw the document to screen.

            return Error();
        }

        bool GLRenderer::shouldHaveItsOwnRenderLayer(brick::Entity _node)
        {
            if (auto manimated = _node.maybe<box::comps::IsAnimated>())
                return *manimated;
            return false;
        }

        namespace detail
        {
            static RenderLayer * findRenderLayer(Entity _node)
            {
                while (_node)
                {
                    if (auto mrl = _node.maybe<comps::RenderLayer>())
                    {
                        return *mrl;
                    }
                    _node = _node.get<box::comps::Parent>();
                }
                return nullptr;
            }
        }

        void GLRenderer::markDocumentForRendering()
        {

        }

        void GLRenderer::markNodeForRendering(Entity _node)
        {
            _node.set<comps::NeedsRedraw>(true);
            RenderLayer * rl = detail::findRenderLayer(_node);
            STICK_ASSERT(rl);
            rl->markDirty(true);
        }

        void GLRenderer::addedNode(Entity _node)
        {
            printf("ADDED NONDE %s\n", _node.get<box::comps::Name>().cString());

            RenderLayer * rl = detail::findRenderLayer(_node.get<box::comps::Parent>());
            STICK_ASSERT(rl);

            if (auto mrl = _node.maybe<comps::RenderLayer>())
            {
                rl->addChild(*mrl);
            }
            else
            {
                rl->addNode(_node);

                //TODO: I think here we need to iterate over all the children
                //and see if they have RenderLayers. We need to add them to
                //rl, if they do!
                if (auto mchildren = _node.maybe<box::comps::Children>())
                {
                    for (auto & child : *mchildren)
                    {
                        if (auto mrl = child.maybe<comps::RenderLayer>())
                            rl->addChild(*mrl);
                    }
                }
            }
        }

        void GLRenderer::removedNode(Entity _node)
        {
            printf("REMOVED NONDE %s\n", _node.get<box::comps::Name>().cString());
            RenderLayer * rl = detail::findRenderLayer(_node);
            if (rl)
                rl->removeNode(_node);
        }

        void GLRenderer::nodeIsAnimatedChanged(Entity _node)
        {

        }

        void GLRenderer::reversedChildren(Entity _node)
        {

        }
    }
}
