#include <core/window.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/input.hpp"

#include <print>

Window::Window(const Window::CreateInfo& createInfo) : m_title{ createInfo.title } {
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	m_width = (std::min)(createInfo.width, uint32_t(mode->width));
	m_height = (std::min)(createInfo.height, uint32_t(mode->height));

	for (const auto& [hint, value] : createInfo.preCreationWindowHints) {
		glfwWindowHint(hint, value);
	}

	m_nativeWindowPtr = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
	if (!m_nativeWindowPtr) {
		std::println("Failed to create window.");
		return;
	}

	glfwSetWindowUserPointer(m_nativeWindowPtr, this);

	glfwSetFramebufferSizeCallback(m_nativeWindowPtr, &Window::frameBufferCallback);

	glfwSetKeyCallback(m_nativeWindowPtr, Input::keyCallback);
	glfwSetMouseButtonCallback(m_nativeWindowPtr, Input::mouseButtonCallback);
	glfwSetCursorPosCallback(m_nativeWindowPtr, Input::mouseCallback);
	glfwSetScrollCallback(m_nativeWindowPtr, Input::mouseScrollCallback);
}

Window::~Window() {
	if (m_nativeWindowPtr) {
		glfwDestroyWindow(m_nativeWindowPtr);
	}
	
	m_width = INVALID_WINDOW_SIZE;
	m_height = INVALID_WINDOW_SIZE;
	m_nativeWindowPtr = nullptr;
}

Window::Window(Window&& other) noexcept :
	m_width{other.m_width},
	m_height{other.m_height},
	m_title{std::move(other.m_title)},
	m_nativeWindowPtr{ other.m_nativeWindowPtr }
{
	if(m_nativeWindowPtr) {
		glfwSetWindowUserPointer(m_nativeWindowPtr, this);
	}

	other.m_width = INVALID_WINDOW_SIZE;
	other.m_height = INVALID_WINDOW_SIZE;
	other.m_nativeWindowPtr = nullptr;
	other.m_title = "";
}

Window& Window::operator=(Window&& other) noexcept {
	if (!other) {
		std::println("Other window is invalid");
		return *this;
	}

	if (this == &other) {
		return *this;
	}

	if (m_nativeWindowPtr) {
		glfwDestroyWindow(m_nativeWindowPtr);
		m_nativeWindowPtr = nullptr;
	}

	m_width = other.m_width;
	m_height = other.m_height;
	m_title = std::move(other.m_title);
	m_nativeWindowPtr = other.m_nativeWindowPtr;

	other.m_width = INVALID_WINDOW_SIZE;
	other.m_height = INVALID_WINDOW_SIZE;
	other.m_nativeWindowPtr = nullptr;
	other.m_title = "";

	if (m_nativeWindowPtr) {
		glfwSetWindowUserPointer(m_nativeWindowPtr, this);
	}

	return *this;
}

Window::operator bool() const noexcept {
	return m_nativeWindowPtr != nullptr && m_width != INVALID_WINDOW_SIZE && m_height != INVALID_WINDOW_SIZE;
}

bool Window::shouldClose() {
	if (!*this) {
		std::println("Window::ShouldClose() doesn't work on uninitialized windows.");
		return true;
	}

	return glfwWindowShouldClose(m_nativeWindowPtr);
}

void Window::swapBuffers() {
	if (!*this) {
		std::println("Window::SwapBuffers() doesn't work on uninitialized windows.");
		return;
	}

	glfwSwapBuffers(m_nativeWindowPtr);
}

void Window::setHint(int32_t hint, int32_t value) {
	if (!*this) {
		std::println("Window::SetHint() doesn't work on uninitialized windows.");
		return;
	}

	glfwWindowHint(hint, value);
}

void Window::createSurface(VkInstance instance, VkSurfaceKHR& surface) {
	if (glfwCreateWindowSurface(instance, m_nativeWindowPtr, nullptr, &surface) != VK_SUCCESS) {
		std::println("Failed to create window surface.");
	}
}

void Window::setWidth(uint32_t newWidth) noexcept {
	if (!*this) {
		std::println("Window::SetWidth() doesn't work on uninitialized windows.");
		return;
	}
	
	glfwSetWindowSize(m_nativeWindowPtr, newWidth, m_height);
	m_width = newWidth;
}

void Window::setHeight(uint32_t newHeight) noexcept {
	if (!*this) {
		std::println("Window::SetHeight() doesn't work on uninitialized windows.");
		return;
	}

	glfwSetWindowSize(m_nativeWindowPtr, m_width, newHeight);
	m_height = newHeight;
}

void Window::setWindowSize(const Window::Size& newWindowSize) noexcept {
	if (!*this) {
		std::println("Window::SetWindowSize() doesn't work on uninitialized windows.");
		return;
	}

	glfwSetWindowSize(m_nativeWindowPtr, newWindowSize.width, newWindowSize.height);
	m_width = newWindowSize.width;
	m_height = newWindowSize.height;
}

void Window::setTitle(const std::string& newTitle) noexcept {
	if (!*this) {
		std::println("Window::SetTitle() doesn't work on uninitialized windows.");
		return;
	}

	glfwSetWindowTitle(m_nativeWindowPtr, newTitle.c_str());
	m_title = std::move(newTitle);
}

void Window::setInputMode(int32_t inputMode, int32_t value) {
	if (!m_nativeWindowPtr) {
		return;
	}

	glfwSetInputMode(m_nativeWindowPtr, inputMode, value);
}

void Window::makeContextCurrent() const noexcept {
	if (!*this) {
		return;
	}

	glfwMakeContextCurrent(m_nativeWindowPtr);
}

// private
void Window::frameBufferCallback(GLFWwindow* glfwWindowPtr, int32_t newWidth, int32_t newHeight) {
	if (!glfwWindowPtr) {
		return;
	}

	Window* windowPtr = (Window*)glfwGetWindowUserPointer(glfwWindowPtr);
	if (!windowPtr) {
		return;
	}

	windowPtr->m_width = uint32_t(newWidth);
	windowPtr->m_height = uint32_t(newHeight);
	windowPtr->m_resizedFlag = true;
}