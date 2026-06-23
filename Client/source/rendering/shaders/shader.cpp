
#include <rendering/shaders/shader.hpp>

#include <glad/glad.h>
#include <fstream>

#include <print>

Shader::Shader(const Shader::CreateInfo& createInfo) {
	if (createInfo.filepath.empty() || createInfo.type == Shader::Type::Invalid) {
		return;
	}

	std::string fileContents = readFile(createInfo.filepath);
	if (fileContents.empty()) {
		return;
	}

	uint32_t shaderType = shaderTypeToGLType(createInfo.type);
	if (shaderType == GL_INVALID_ENUM) {
		return;
	}

	const char* shaderContents = fileContents.c_str();
	if (!shaderContents) {
		return;
	}

	m_id = glCreateShader(shaderType);
	glShaderSource(m_id, 1, &shaderContents, 0);
	glCompileShader(m_id);

	if (!validateShaderCompilation(createInfo.type, createInfo.filepath)) {
		glDeleteShader(m_id);
		m_id = Shader::INVALID_ID;
	}
}

Shader::~Shader() {
	if (m_id == Shader::INVALID_ID) {
		return;
	}

	glDeleteShader(m_id);
	m_id = Shader::INVALID_ID;
}

Shader::operator bool() const {
	return m_id != Shader::INVALID_ID;
}

// private

std::string Shader::readFile(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		return "";
	}

	uint64_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::string content(fileSize, 0);
	file.read(&content[0], fileSize);
	return content;
}

uint32_t Shader::shaderTypeToGLType(const Shader::Type type) {
	switch (type) {
	case Shader::Type::Vertex:
		return GL_VERTEX_SHADER;
		break;
	case Shader::Type::Fragment:
		return GL_FRAGMENT_SHADER;
		break;
	case Shader::Type::Compute:
		return GL_COMPUTE_SHADER;
		break;
	case Shader::Type::Geometry:
		return GL_GEOMETRY_SHADER;
		break;
	default:
		return GL_INVALID_ENUM;
		break;
	}
}

std::string Shader::getNameFromType(const Shader::Type type) {
	switch (type) {
	case Shader::Type::Vertex:
		return "Vertex";
		break;
	case Shader::Type::Fragment:
		return "Fragment";
		break;
	case Shader::Type::Compute:
		return "Compute";
		break;
	default:
		return "Invalid";
		break;
	}
}

bool Shader::validateShaderCompilation(const Shader::Type type, const std::string& filepath) {
	if (type == Shader::Type::Invalid) {
		return false;
	}

	int32_t success = 0;
	glGetShaderiv(m_id, GL_COMPILE_STATUS, &success);

	if (success) {
		return true;
	}

	int32_t infoLogLength = 0;
	glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (!infoLogLength) {
		return false;
	}

	std::string infoLog(infoLogLength, 0);
	glGetShaderInfoLog(m_id, infoLogLength, nullptr, infoLog.data());

	std::print("[{0}] failed to compile {1}:\n {2}\n", getNameFromType(type), filepath, infoLog);

	//std::cout << "[" << getNameFromType(type) << "] failed to compile " << filepath << ":\n" << infoLog << "\n";
	return false;
}