#pragma once

#include <stdbool.h>
#include <strolb/window.h>

// Alphabet keys
#define SLB_KEY_A GLFW_KEY_A
#define SLB_KEY_B GLFW_KEY_B
#define SLB_KEY_C GLFW_KEY_C
#define SLB_KEY_D GLFW_KEY_D
#define SLB_KEY_E GLFW_KEY_E
#define SLB_KEY_F GLFW_KEY_F
#define SLB_KEY_G GLFW_KEY_G
#define SLB_KEY_H GLFW_KEY_H
#define SLB_KEY_I GLFW_KEY_I
#define SLB_KEY_J GLFW_KEY_J
#define SLB_KEY_K GLFW_KEY_K
#define SLB_KEY_L GLFW_KEY_L
#define SLB_KEY_M GLFW_KEY_M
#define SLB_KEY_N GLFW_KEY_N
#define SLB_KEY_O GLFW_KEY_O
#define SLB_KEY_P GLFW_KEY_P
#define SLB_KEY_Q GLFW_KEY_Q
#define SLB_KEY_R GLFW_KEY_R
#define SLB_KEY_S GLFW_KEY_S
#define SLB_KEY_T GLFW_KEY_T
#define SLB_KEY_U GLFW_KEY_U
#define SLB_KEY_V GLFW_KEY_V
#define SLB_KEY_W GLFW_KEY_W
#define SLB_KEY_X GLFW_KEY_X
#define SLB_KEY_Y GLFW_KEY_Y
#define SLB_KEY_Z GLFW_KEY_Z

// Number keys
#define SLB_KEY_0 GLFW_KEY_0
#define SLB_KEY_1 GLFW_KEY_1
#define SLB_KEY_2 GLFW_KEY_2
#define SLB_KEY_3 GLFW_KEY_3
#define SLB_KEY_4 GLFW_KEY_4
#define SLB_KEY_5 GLFW_KEY_5
#define SLB_KEY_6 GLFW_KEY_6
#define SLB_KEY_7 GLFW_KEY_7
#define SLB_KEY_8 GLFW_KEY_8
#define SLB_KEY_9 GLFW_KEY_9

// Function keys
#define SLB_KEY_F1  GLFW_KEY_F1
#define SLB_KEY_F2  GLFW_KEY_F2
#define SLB_KEY_F3  GLFW_KEY_F3
#define SLB_KEY_F4  GLFW_KEY_F4
#define SLB_KEY_F5  GLFW_KEY_F5
#define SLB_KEY_F6  GLFW_KEY_F6
#define SLB_KEY_F7  GLFW_KEY_F7
#define SLB_KEY_F8  GLFW_KEY_F8
#define SLB_KEY_F9  GLFW_KEY_F9
#define SLB_KEY_F10 GLFW_KEY_F10
#define SLB_KEY_F11 GLFW_KEY_F11
#define SLB_KEY_F12 GLFW_KEY_F12

// Arrow keys
#define SLB_KEY_UP    GLFW_KEY_UP
#define SLB_KEY_DOWN  GLFW_KEY_DOWN
#define SLB_KEY_LEFT  GLFW_KEY_LEFT
#define SLB_KEY_RIGHT GLFW_KEY_RIGHT

// Modifier keys
#define SLB_KEY_LEFT_SHIFT    GLFW_KEY_LEFT_SHIFT
#define SLB_KEY_RIGHT_SHIFT   GLFW_KEY_RIGHT_SHIFT
#define SLB_KEY_LEFT_CONTROL  GLFW_KEY_LEFT_CONTROL
#define SLB_KEY_RIGHT_CONTROL GLFW_KEY_RIGHT_CONTROL
#define SLB_KEY_LEFT_ALT      GLFW_KEY_LEFT_ALT
#define SLB_KEY_RIGHT_ALT     GLFW_KEY_RIGHT_ALT
#define SLB_KEY_LEFT_SUPER    GLFW_KEY_LEFT_SUPER
#define SLB_KEY_RIGHT_SUPER   GLFW_KEY_RIGHT_SUPER

// Navigation keys
#define SLB_KEY_HOME      GLFW_KEY_HOME
#define SLB_KEY_END       GLFW_KEY_END
#define SLB_KEY_PAGE_UP   GLFW_KEY_PAGE_UP
#define SLB_KEY_PAGE_DOWN GLFW_KEY_PAGE_DOWN
#define SLB_KEY_INSERT    GLFW_KEY_INSERT
#define SLB_KEY_DELETE    GLFW_KEY_DELETE
#define SLB_KEY_BACKSPACE GLFW_KEY_BACKSPACE
#define SLB_KEY_TAB       GLFW_KEY_TAB
#define SLB_KEY_ENTER     GLFW_KEY_ENTER
#define SLB_KEY_ESCAPE    GLFW_KEY_ESCAPE

// Keypad keys
#define SLB_KEY_KP_0        GLFW_KEY_KP_0
#define SLB_KEY_KP_1        GLFW_KEY_KP_1
#define SLB_KEY_KP_2        GLFW_KEY_KP_2
#define SLB_KEY_KP_3        GLFW_KEY_KP_3
#define SLB_KEY_KP_4        GLFW_KEY_KP_4
#define SLB_KEY_KP_5        GLFW_KEY_KP_5
#define SLB_KEY_KP_6        GLFW_KEY_KP_6
#define SLB_KEY_KP_7        GLFW_KEY_KP_7
#define SLB_KEY_KP_8        GLFW_KEY_KP_8
#define SLB_KEY_KP_9        GLFW_KEY_KP_9
#define SLB_KEY_KP_DECIMAL  GLFW_KEY_KP_DECIMAL
#define SLB_KEY_KP_DIVIDE   GLFW_KEY_KP_DIVIDE
#define SLB_KEY_KP_MULTIPLY GLFW_KEY_KP_MULTIPLY
#define SLB_KEY_KP_SUBTRACT GLFW_KEY_KP_SUBTRACT
#define SLB_KEY_KP_ADD      GLFW_KEY_KP_ADD
#define SLB_KEY_KP_ENTER    GLFW_KEY_KP_ENTER
#define SLB_KEY_KP_EQUAL    GLFW_KEY_KP_EQUAL

// Special character keys
#define SLB_KEY_SPACE         GLFW_KEY_SPACE
#define SLB_KEY_APOSTROPHE    GLFW_KEY_APOSTROPHE    // '
#define SLB_KEY_COMMA         GLFW_KEY_COMMA         // ,
#define SLB_KEY_MINUS         GLFW_KEY_MINUS         // -
#define SLB_KEY_PERIOD        GLFW_KEY_PERIOD        // .
#define SLB_KEY_SLASH         GLFW_KEY_SLASH         // /
#define SLB_KEY_SEMICOLON     GLFW_KEY_SEMICOLON     // ;
#define SLB_KEY_EQUAL         GLFW_KEY_EQUAL         // =
#define SLB_KEY_LEFT_BRACKET  GLFW_KEY_LEFT_BRACKET  // [
#define SLB_KEY_BACKSLASH     GLFW_KEY_BACKSLASH     // forward slash
#define SLB_KEY_RIGHT_BRACKET GLFW_KEY_RIGHT_BRACKET // ]
#define SLB_KEY_GRAVE_ACCENT  GLFW_KEY_GRAVE_ACCENT  // `

// Lock keys
#define SLB_KEY_CAPS_LOCK    GLFW_KEY_CAPS_LOCK
#define SLB_KEY_SCROLL_LOCK  GLFW_KEY_SCROLL_LOCK
#define SLB_KEY_NUM_LOCK     GLFW_KEY_NUM_LOCK
#define SLB_KEY_PRINT_SCREEN GLFW_KEY_PRINT_SCREEN
#define SLB_KEY_PAUSE        GLFW_KEY_PAUSE

#define SLB_MOUSE_BUTTON_LEFT   GLFW_MOUSE_BUTTON_LEFT
#define SLB_MOUSE_BUTTON_RIGHT  GLFW_MOUSE_BUTTON_RIGHT
#define SLB_MOUSE_BUTTON_MIDDLE GLFW_MOUSE_BUTTON_MIDDLE

// Additional mouse buttons (many mice have these extra buttons)
#define SLB_MOUSE_BUTTON_4 GLFW_MOUSE_BUTTON_4
#define SLB_MOUSE_BUTTON_5 GLFW_MOUSE_BUTTON_5
#define SLB_MOUSE_BUTTON_6 GLFW_MOUSE_BUTTON_6
#define SLB_MOUSE_BUTTON_7 GLFW_MOUSE_BUTTON_7
#define SLB_MOUSE_BUTTON_8 GLFW_MOUSE_BUTTON_8

// Mouse button count definition
#define SLB_MOUSE_BUTTON_COUNT (GLFW_MOUSE_BUTTON_LAST + 1)

typedef int slb_Key;

// Only returns true on the first frame that a key is pressed
bool slb_Input_GetKeyDown(slb_Window* window, slb_Key key);
// Only returns true on the first frame that a key is released
bool slb_Input_GetKeyUp(slb_Window* window, slb_Key key);
// Returns true if the key is pressed
bool slb_Input_GetKey(slb_Window* window, slb_Key key);

// Much like the key functions but with mouse input

bool slb_Input_GetMouseButtonDown(slb_Window* window, slb_Key mouseKey);
bool slb_Input_GetMouseButtonUp(slb_Window* window, slb_Key mouseKey);
bool slb_Input_GetMouseButton(slb_Window* window, slb_Key mouseKey);

// These functions get the screen-space position of the mouse

int slb_Input_GetMouseInputHorizontal(slb_Window* window);
int slb_Input_GetMouseInputVertical(slb_Window* window);
