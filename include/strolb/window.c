#include <strolb/window.h>

void slb_Window_FramebufferSizeCallback(GLFWwindow* window, int height,
                                        int width)
{
    slb_Window* win = (slb_Window*)(glfwGetWindowUserPointer(window));
    win->framebufferResized = true;
}

slb_Window slb_Window_Create(const char* title, int16_t width, int16_t height,
                         bool fullscreen, bool maximize)
{
    slb_Window window;

    // Glfw: Initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Glfw window creation
    // --------------------
    window.window = glfwCreateWindow(
        width, height, title,
        fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (window.window == NULL)
    {
        printf("SK ERROR: Failed to create GLFW window.\n");
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(window.window);
    glfwSetWindowUserPointer(window.window, &window);
    glfwSetFramebufferSizeCallback(window.window,
                                   slb_Window_FramebufferSizeCallback);

    if (maximize)
        glfwMaximizeWindow(window.window);

    window.width = width;
    window.height = height;

    return window;
}

bool slb_Window_ShouldClose(slb_Window* window)
{
    return glfwWindowShouldClose(window->window);
}

void slb_Window_Update(slb_Window* window)
{
    glfwSwapBuffers(window->window);
    glfwPollEvents();
}

float slb_Window_GetAspectRatio(slb_Window* window)
{
    if (window->width == 0 || window->height == 0)
    {
        // Handle the minimized window case
        return 1.0f;
    }

    return (float)window->width / (float)window->height;
}

void slb_Window_Rename(slb_Window* window, const char* newName)
{
    glfwSetWindowTitle(window->window, newName);
}

void slb_Window_Close(slb_Window* window)
{
    glfwTerminate();
}
