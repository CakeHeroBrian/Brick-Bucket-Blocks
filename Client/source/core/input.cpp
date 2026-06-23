#include "core/input.hpp"

#include <GLFW/glfw3.h>

namespace Input {
	std::array<bool, GLFW_KEY_LAST> keyPressedData = {};
	std::array<bool, GLFW_MOUSE_BUTTON_LAST> mouseButtonPressedData = {};

	std::array<int32_t, GLFW_MOUSE_BUTTON_LAST> mouseButtonPreviousState = {};
	std::array<int32_t, GLFW_MOUSE_BUTTON_LAST> mouseButtonCurrentState = {};

	float mouseX = 0.0f;
	float mouseY = 0.0f;
	float mouseScrollX = 0.0f;
	float mouseScrollY = 0.0f;
	float deltaMouseX = 0.0f;
	float deltaMouseY = 0.0f;

	bool mouseClicked = false;
	bool mouseReleased = false;

	static float lastMouseX = 0.0f;
	static float lastMouseY = 0.0f;
	static bool firstFrame = true;

	void keyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods)
	{
		if (key >= 0 && key < GLFW_KEY_LAST)
		{
			keyPressedData[key] = (action == GLFW_PRESS) || (action == GLFW_REPEAT);
		}
	}

	void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
		mouseX = (float)xpos;
		mouseY = (float)ypos;
		if (firstFrame) {
			lastMouseX = (float)xpos;
			lastMouseY = (float)ypos;
			firstFrame = false;
		}

		int32_t windowWidth = 0;
		int32_t windowHeight = 0;
		glfwGetWindowSize(window, &windowWidth, &windowHeight);

		deltaMouseX = ((float)xpos - lastMouseX);
		deltaMouseY = (lastMouseY - (float)ypos);

		lastMouseX = (float)xpos;
		lastMouseY = (float)ypos;
	}

	void mouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods) {
		if (button >= 0 && button < GLFW_MOUSE_BUTTON_LAST) {
			mouseButtonPressedData[button] = action == GLFW_PRESS;
			mouseButtonCurrentState[button] = action;
		}
		
		if (button == GLFW_MOUSE_BUTTON_1) {
			mouseClicked = action == GLFW_PRESS;
			mouseReleased = action == GLFW_RELEASE;
		}
	}

	void endFrame() {
		
		mouseScrollX = 0.0f;
		mouseScrollY = 0.0f;
		deltaMouseX = 0.0f;
		deltaMouseY = 0.0f;

		mouseClicked = false;
		mouseReleased = false;
	}

	void captureEvents() {
		for (int32_t i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++) {
			mouseButtonPreviousState[i] = mouseButtonCurrentState[i];
		}
	}

	void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
		mouseScrollX = (float)xoffset;
		mouseScrollY = (float)yoffset;
	}
}