#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include <vulkan/vulkan.hpp>


struct GLFWwindow;


class Window {
public:
	static const uint32_t INVALID_WINDOW_SIZE = UINT32_MAX;
	struct CreateInfo {
		uint32_t width = INVALID_WINDOW_SIZE;
		uint32_t height = INVALID_WINDOW_SIZE;
		std::string title = "Brick Bucket";

		std::vector<std::pair<int32_t, int32_t>> preCreationWindowHints{};
	};

	struct Size {
		uint32_t width = INVALID_WINDOW_SIZE;
		uint32_t height = INVALID_WINDOW_SIZE;
	};
public:
	Window() = default;
	Window(const Window::CreateInfo& createInfo);

	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	Window(Window&&) noexcept;
	Window& operator=(Window&&) noexcept;

	explicit operator bool() const noexcept;

	bool shouldClose();
	void swapBuffers();
	void setHint(int32_t hint, int32_t value);

	void createSurface(VkInstance instance, VkSurfaceKHR& surface);

	uint32_t getWidth() const noexcept { return m_width; }
	uint32_t getHeight() const noexcept { return m_height; }
	float getAspectRatio() const noexcept { return float(m_width) / float(m_height); }
	Size getWindowSize() const noexcept { return Size{.width = m_width, .height = m_height}; }
	const std::string& getTitle() const noexcept { return m_title; }

	bool windowResized() const { return m_resizedFlag; }
	void resetResizedFlag() { m_resizedFlag = false; }

	void setWidth(uint32_t newWidth) noexcept;
	void setHeight(uint32_t newHeight) noexcept;
	void setWindowSize(const Window::Size& newWindowSize) noexcept;
	void setTitle(const std::string& newTitle) noexcept;

	void setInputMode(int32_t inputMode, int32_t value);

	void makeContextCurrent() const noexcept;
private:
	static void frameBufferCallback(GLFWwindow* glfwWindowPtr, int32_t newWidth, int32_t newHeight);
private:
	uint32_t m_width = INVALID_WINDOW_SIZE;
	uint32_t m_height = INVALID_WINDOW_SIZE;
	std::string m_title = "Brick Bucket";

	GLFWwindow* m_nativeWindowPtr = nullptr;

	bool m_resizedFlag = false;
};