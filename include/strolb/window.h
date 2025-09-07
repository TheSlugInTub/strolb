#pragma once

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    const char*        title;
    int16_t            width;
    int16_t            height;
    struct GLFWwindow* window;
    bool               framebufferResized;
} slb_Window;

// Initialize a GLFW window and OpenGL context
slb_Window slb_Window_Create(const char* title, int16_t width, int16_t height,
                         bool fullscreen, bool maximize);

// Is the window closed or not? Useful for running a game loop
bool slb_Window_ShouldClose(slb_Window* window);

// Swaps buffers, poll events
void slb_Window_Update(slb_Window* window);

// (float)width / (float)height
float slb_Window_GetAspectRatio(slb_Window* window);

// Renames a window
void slb_Window_Rename(slb_Window* window, const char* newName);

// Closes the window
void slb_Window_Close(slb_Window* window);
