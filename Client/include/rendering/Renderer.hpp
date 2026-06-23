#pragma once

class Window;

#include <game/world/Chunk.hpp>

#include <array>

#include <rendering/shaders/shader_program.hpp>

struct Vertex {
	//glm::vec2 position{ 0.0f };
	//glm::vec3 color{ 1.0f };

	//		 x		y		z	 f		 t
	// 0b |000000|000000|000000|000|0000000000|0
	uint32_t data = 0;
};

class Renderer {
public:
	struct CreateInfo {
		Window* pWindow;
	};
public:
	Renderer() = default;
	Renderer(const Renderer::CreateInfo& createInfo);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	Renderer(Renderer&& other) noexcept;
	Renderer& operator=(Renderer&& other) noexcept;

	explicit operator bool() const noexcept;
	
	ShaderProgram& getShaderProgram() noexcept { return m_shaaderProgram; }
	ShaderProgram& getCrosshairShaderProgram() noexcept { return m_crosshairShaderProgram; }
private:
	void createShaders();

	void cleanup();
private:
	ShaderProgram m_shaaderProgram{};
	ShaderProgram m_crosshairShaderProgram{};

	Window* pWindow = nullptr;
};