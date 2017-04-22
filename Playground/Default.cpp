#include <Box/Box.hpp>
#include <GLFW/glfw3.h>
#include <GLAnnotate/GLAnnotate.hpp>
#include <Crunch/Colors.hpp>

using namespace stick;
using namespace box;
using namespace brick;

namespace mycomps
{
    using BackgroundColor = brick::Component<ComponentName("BackgroundColor"), crunch::ColorRGBA>;
}

void recursivelyDrawDocument(Entity _e, gla::GLAnnotate & _renderer)
{
    auto mc = _e.maybe<mycomps::BackgroundColor>();
    if (mc)
    {
        _renderer.setColor((*mc).r, (*mc).g, (*mc).b, (*mc).a);
        auto & cl = _e.get<comps::ComputedLayout>();
        _renderer.rect(cl.box.min().x, cl.box.min().y, cl.box.width(), cl.box.height());
    }
    auto mchildren = _e.maybe<comps::Children>();
    if (mchildren)
    {
        for (Entity & _e : (*mchildren))
        {
            recursivelyDrawDocument(_e, _renderer);
        }
    }
}

int main(int _argc, const char * _args[])
{
    // initialize glfw
    if (!glfwInit())
        return EXIT_FAILURE;

    // and set some hints to get the correct opengl versions/profiles
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //create the window
    GLFWwindow * window = glfwCreateWindow(800, 600, "Hello Paper Example", NULL, NULL);
    if (window)
    {
        glfwMakeContextCurrent(window);

        gla::GLAnnotate gla;

        bool res = gla.init();
        if (!res)
        {
            printf("%s\n", gla.error());
            return EXIT_FAILURE;
        }

        auto root = createNode("DAAA Root");
        root.set<comps::Width>(800.0f);
        root.set<comps::Height>(600.0f);
        root.set <mycomps::BackgroundColor>(0.5f, 0.3f, 0.1f, 1.0f);
        auto a = createNode("DAAA A");
        a.set<comps::Height>(200.0f);
        a.set <mycomps::BackgroundColor>(1.0f, 1.0f, 1.0f, 1.0f);
        addChild(root, a);
        auto b = createNode("DAAA FUUUUUCK B");
        b.set<comps::Width>(400.0f);
        b.set<comps::MaxWidth>(200.0f);
        b.set<comps::Height>(100.0f);
        b.set <mycomps::BackgroundColor>(0.3f, 0.3f, 0.3f, 1.0f);
        addChild(a, b);

        auto c = createNode("DAAA C");
        c.set<comps::Width>(200.0f);
        c.set<comps::Justify>(Justify::End);
        c.set <mycomps::BackgroundColor>(0.9f, 0.3f, 0.3f, 1.0f);
        addChild(root, c);

        auto e = createNode("E");
        e.set<comps::Width>(100.0f);
        e.set<comps::Height>(20.0f);
        e.set <mycomps::BackgroundColor>(0.9f, 0.3f, 0.9f, 1.0f);
        addChild(c, e);

        auto f = createNode("F");
        f.set<comps::Width>(50.0f);
        f.set<comps::Height>(50.0f);
        f.set <mycomps::BackgroundColor>(0.9f, 0.9f, 0.3f, 1.0f);
        addChild(c, f);

        auto g = createNode("G");
        g.set<comps::Width>(80.0f);
        g.set<comps::Height>(20.0f);
        g.set <mycomps::BackgroundColor>(0.9f, 0.1f, 0.9f, 1.0f);
        addChild(c, g);

        auto h = createNode("H");
        h.set<comps::Width>(50.0f);
        h.set<comps::Height>(50.0f);
        h.set <mycomps::BackgroundColor>(0.9f, 0.1f, 0.4f, 1.0f);
        addChild(c, h);

        auto i = createNode("I");
        i.set<comps::Width>(30.0f);
        i.set<comps::Height>(60.0f);
        i.set <mycomps::BackgroundColor>(0.1f, 0.1f, 1.0f, 1.0f);
        addChild(c, i);

        // the main loop
        float angle = 0;
        while (!glfwWindowShouldClose(window))
        {
            layout(root, 800, 600);
            printf("POST LAYOUT\n");

            // clear the background to black
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT);

            gla.ortho(0, 800, 600, 0, -1, 1);
            recursivelyDrawDocument(root, gla);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    else
    {
        glfwTerminate();
        printf("Could not open GLFW window :(\n");
        return EXIT_FAILURE;
    }

    // clean up glfw
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}