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
            "in vec2 col; \n"
            "out vec4 icol;\n"
            "void main() \n"
            "{ \n"
            "gl_Position = transformProjection * vec4(vertex, 0.0, 1.0); \n"
            "icol = col;\n"
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
        }

        GLRenderer::GLRenderer()
        {

        }

        GLRenderer::~GLRenderer()
        {

        }

        Error GLRenderer::initialize(Entity _document)
        {
            m_document = _document;
            Error ret = detail::createProgram(vertexShaderCode, fragmentShaderCode, {"vertex", "color"}, m_colorProgram);
            if (ret) return ret;
            ASSERT_NO_GL_ERROR(glGenVertexArrays(1, &m_vao));
            ASSERT_NO_GL_ERROR(glGenBuffers(1, &m_vbo));
            ASSERT_NO_GL_ERROR(glBindVertexArray(m_vao));
            ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
            ASSERT_NO_GL_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), ((char *)0)));
            ASSERT_NO_GL_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), ((char *)0 + sizeof(float) * 2)));
            ASSERT_NO_GL_ERROR(glEnableVertexAttribArray(0));
        }

        struct Vertex
        {
            Vec2f position;
            ColorRGBA color;
        };

        Error GLRenderer::recursivelyDrawNode(Entity _e)
        {
            STICK_ASSERT(_e.hasComponent<box::comps::ComputedLayout>());
            //draw background
            if (auto mbg = _e.maybe<box::comps::Background>())
            {
                //solid background
                if ((*mbg).is<crunch::ColorRGBA>())
                {
                    auto & col = (*mbg).get<crunch::ColorRGBA>();
                    auto & box = _e.get<box::comps::ComputedLayout>().box;
                    DynamicArray<Vertex> vertices =
                    {
                        {box.min(), col},
                        {box.min() + Vec2f(0, box.height()) , col},
                        {box.min() + Vec2f(box.width(), 0) , col},
                        {box.max(), col}
                    };
                    ASSERT_NO_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, &vertices[0].position.x, GL_DYNAMIC_DRAW));
                    ASSERT_NO_GL_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
                }
                //background image
                else if ((*mbg).is<BackgroundImage>())
                {
                    //@TODO
                }
            }

            // DynamicArray<Vertex> vertices = {}

            //draw border
        }

        Error GLRenderer::render()
        {
            ASSERT_NO_GL_ERROR(glUseProgram(m_colorProgram));
            ASSERT_NO_GL_ERROR(glBindVertexArray(m_vao));
            ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, m_vbo));
            // ASSERT_NO_GL_ERROR(glViewport(0, 0, m_viewport.x, m_viewport.y));

            recursivelyDrawNode(m_document);
            // @TODO: Traverse the document tree twice, once to redraw all dirty nodes to a texture
            // and a second time to actually draw the document to screen.
        }

        void GLRenderer::markDocumentForRendering()
        {

        }

        void GLRenderer::markNodeForRendering(Entity _node)
        {
            _node.set<comps::NeedsRedraw>(true);
        }
    }
}
