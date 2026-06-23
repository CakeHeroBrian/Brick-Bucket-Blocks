#include <rendering/Renderer.hpp>
//#include <rendering/vulkan/descriptor_set_layout.hpp>

#include <core/window.hpp>

#include <GLFW/glfw3.h>

#include <print>

#include <rendering/shaders/shader.hpp>

Renderer::Renderer(const Renderer::CreateInfo& createInfo) : pWindow{ createInfo.pWindow } {
	//Device::CreateInfo deviceCreateInfo{};
	//deviceCreateInfo.pWindow = pWindow;

	//m_device = Device(deviceCreateInfo);

	//SwapChain::CreateInfo swapChainCreateInfo{};
	//swapChainCreateInfo.pDevice = &m_device;
	//swapChainCreateInfo.pWindow = pWindow;
	//swapChainCreateInfo.pOldSwapChain = nullptr;

	//m_swapChain = SwapChain(swapChainCreateInfo);

	//createDescriptorPoolAndLayout();
	//createPipeline();
	//createCommandBuffers();
	//createDescriptorSet();

	createShaders();
}

Renderer::~Renderer() {
	cleanup();
}

Renderer::Renderer(Renderer&& other) noexcept :
	//m_device{std::move(other.m_device)},
	//m_swapChain{std::move(other.m_swapChain)},
	//m_descriptorPool{std::move(other.m_descriptorPool)},
	//m_descriptorSetLayout{std::move(other.m_descriptorSetLayout)},
	//m_pipeline{std::move(other.m_pipeline)},
	//m_commandBuffers{std::move(other.m_commandBuffers)},
	//m_descriptorSet{std::move(other.m_descriptorSet)},
	m_shaaderProgram{std::move(other.m_shaaderProgram)},
	m_crosshairShaderProgram{ std::move(other.m_crosshairShaderProgram) },

	pWindow{other.pWindow}
{
	other.pWindow = nullptr;
	//other.m_commandBuffers = {};
}

Renderer& Renderer::operator=(Renderer&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	if (*this) {
		cleanup();
	}

	//m_device = std::move(other.m_device);
	//m_swapChain = std::move(other.m_swapChain);
	//m_descriptorPool = std::move(other.m_descriptorPool);
	//m_descriptorSetLayout = std::move(other.m_descriptorSetLayout);
	//m_pipeline = std::move(other.m_pipeline);
	//m_commandBuffers = std::move(other.m_commandBuffers);
	//m_descriptorSet = std::move(m_descriptorSet);
	m_shaaderProgram = std::move(other.m_shaaderProgram);
	m_crosshairShaderProgram = std::move(other.m_crosshairShaderProgram);
	pWindow = other.pWindow;

	other.pWindow = nullptr;
///	other.m_commandBuffers = {};

	return *this;
}

Renderer::operator bool() const noexcept {
	return
		//m_device && m_swapChain &&
		//m_descriptorPool && m_descriptorSetLayout &&
		//!m_commandBuffers.empty() &&
		//m_descriptorSet 
		m_shaaderProgram &&
		pWindow != nullptr;
}

// private

void Renderer::cleanup() {
	pWindow = nullptr;
}

void Renderer::createShaders() {

	std::vector<Shader::CreateInfo> shaderProgramCreateInfo{ {"assets/shaders/simple.vert", Shader::Type::Vertex }, { "assets/shaders/simple.frag", Shader::Type::Fragment} };
	m_shaaderProgram = ShaderProgram(shaderProgramCreateInfo);

	std::vector<Shader::CreateInfo> crossHairShaderCreateInfo{ {"assets/shaders/crosshair.vert", Shader::Type::Vertex}, {"assets/shaders/crosshair.frag", Shader::Type::Fragment} };
	m_crosshairShaderProgram = ShaderProgram(crossHairShaderCreateInfo);
}