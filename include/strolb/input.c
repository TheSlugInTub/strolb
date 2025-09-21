#include <strolb/input.h>

typedef unsigned char Bool;

Bool keyState[400] = {};
Bool prevKeyState[400] = {};

bool slb_Input_GetKeyDown(slb_Window* window, slb_Key key)
{
    prevKeyState[key] = keyState[key];
    keyState[key] =
        glfwGetKey(window->window, key) == GLFW_PRESS;
    return keyState[key] && !prevKeyState[key];
}

bool slb_Input_GetKeyUp(slb_Window* window, slb_Key key)
{
    bool result =
        glfwGetKey(window->window, key) != GLFW_PRESS &&
        prevKeyState[key];
    prevKeyState[key] = keyState[key];
    keyState[key] =
        glfwGetKey(window->window, key) == GLFW_PRESS;
    return result;
}

bool slb_Input_GetKey(slb_Window* window, slb_Key key)
{
    return glfwGetKey(window->window, key) == GLFW_PRESS;
}

bool slb_Input_GetMouseButtonDown(slb_Window* window, slb_Key mouseKey)
{
    prevKeyState[mouseKey] = keyState[mouseKey];
    keyState[mouseKey] = glfwGetMouseButton(window->window,
                                            mouseKey) == GLFW_PRESS;
    return keyState[mouseKey] && !prevKeyState[mouseKey];
}

bool slb_Input_GetMouseButtonUp(slb_Window* window, slb_Key mouseKey)
{
    bool result =
        glfwGetMouseButton(window->window, mouseKey) != GLFW_PRESS &&
        prevKeyState[mouseKey];
    prevKeyState[mouseKey] = keyState[mouseKey];
    keyState[mouseKey] =
        glfwGetMouseButton(window->window, mouseKey) == GLFW_PRESS;
    return result;
}

bool slb_Input_GetMouseButton(slb_Window* window, slb_Key mouseKey)
{
    return glfwGetMouseButton(window->window, mouseKey) ==
           GLFW_PRESS;
}

int slb_Input_GetMouseInputHorizontal(slb_Window* window)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window->window, &mouseX, &mouseY);
    return mouseX;
}

int slb_Input_GetMouseInputVertical(slb_Window* window)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window->window, &mouseX, &mouseY);
    return mouseY;
}
