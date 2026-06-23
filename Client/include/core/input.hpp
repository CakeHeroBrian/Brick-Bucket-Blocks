#pragma once

#include <array>
#include <cstdint>

#include <GLFW/glfw3.h>

struct GLFWwindow;

namespace Input
{
    extern std::array<bool, GLFW_KEY_LAST> keyPressedData;
    extern std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonPressedData;
    extern std::array<int32_t, GLFW_MOUSE_BUTTON_LAST> mouseButtonPreviousState;
    extern std::array<int32_t, GLFW_MOUSE_BUTTON_LAST> mouseButtonCurrentState;

    extern float mouseX;
    extern float mouseY;
    extern float mouseScrollX;
    extern float mouseScrollY;
    extern float deltaMouseX;
    extern float deltaMouseY;
    extern uint32_t currentMouseState;
    extern bool mouseClicked;
    extern bool mouseReleased;

    void keyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods);
    void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    void mouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods);
    void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void endFrame();
    void captureEvents();

    inline bool isKeyDown(int32_t key)
    {
        return keyPressedData[key];
    }

    inline bool isMouseButtonDown(int32_t mouseButton)
    {
        return mouseButtonPressedData[mouseButton];
    }

    inline bool isMouseButtonPressed(int32_t mouseButton) {
        return mouseButtonCurrentState[mouseButton] == GLFW_PRESS && mouseButtonPreviousState[mouseButton] == GLFW_RELEASE;
    }
}