

#include <rendering/shaders/shader_program.hpp>

#include <glad/glad.h>

#include <unordered_set>

#include <glm/gtc/type_ptr.hpp>

#include <print>


struct ShaderVariable {
	std::string name = "";
	int32_t variableLocation = 0;
	uint32_t shaderProgramID = 0;

	bool operator==(const ShaderVariable& other) const {
		return other.shaderProgramID == shaderProgramID && other.name == name;
	}
};

struct hashShaderVariable {
	std::size_t operator()(const ShaderVariable& key) const {
		std::size_t seed = std::hash<std::string>()(key.name);
		seed ^= std::hash<uint32_t>()(key.shaderProgramID) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

static std::unordered_set<ShaderVariable, hashShaderVariable> allAvaliableShaderLocations{};

static int32_t getVariableLocation(const ShaderProgram& shaderProgram, const std::string& variableName);
static void retreiveActiveUniforms(const ShaderProgram& shaderProgram);

ShaderProgram::ShaderProgram(const std::vector<Shader::CreateInfo>& shaderCreateInfos) {
	//if (*this) {
	//	return;
	//}

	m_id = glCreateProgram();
	//glUseProgram(m_id);

	std::vector<Shader> shaders;
	shaders.reserve(shaderCreateInfos.size());

	for (const auto& createInfo : shaderCreateInfos) {
		shaders.emplace_back(createInfo);
		glAttachShader(m_id, shaders.back().getId());
	}

	glLinkProgram(m_id);
	if (!validateShaderProgramLinking()) {
		glDeleteProgram(m_id);
		m_id = INVALID_PROGRAM_ID;
	}

	retreiveActiveUniforms(*this);
}

ShaderProgram::~ShaderProgram() {
	if (!*this) {
		return;
	}

	glDeleteProgram(m_id);
	m_id = INVALID_PROGRAM_ID;
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : m_id{ other.m_id } {
	other.m_id = INVALID_PROGRAM_ID;
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	if (*this) {
		glDeleteProgram(m_id);
	}

	m_id = other.m_id;
	other.m_id = INVALID_PROGRAM_ID;
	return *this;
}

ShaderProgram::operator bool() const {
	return m_id != INVALID_PROGRAM_ID;
}

void ShaderProgram::bind() {
	if (!*this) {
		return;
	}
	glUseProgram(m_id);
}

void ShaderProgram::unbind() {
	glUseProgram(0);
}

void ShaderProgram::uploadVec2(const std::string& variableName, const glm::vec2& vec2) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform2f(variableLocation, vec2.x, vec2.y);
}

void ShaderProgram::uploadVec3(const std::string& variableName, const glm::vec3& vec3) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform3f(variableLocation, vec3.x, vec3.y, vec3.z);
}

void ShaderProgram::uploadVec4(const std::string& variableName, const glm::vec4& vec4) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform4f(variableLocation, vec4.x, vec4.y, vec4.z, vec4.w);
}

void ShaderProgram::uploadUVec2(const std::string& variableName, const glm::uvec2& uvec2) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform2ui(variableLocation, uvec2.x, uvec2.y);
}

void ShaderProgram::uploadUVec3(const std::string& variableName, const glm::uvec3& uvec3) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform3ui(variableLocation, uvec3.x, uvec3.y, uvec3.z);
}

void ShaderProgram::uploadUVec4(const std::string& variableName, const glm::uvec4& uvec4) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform4ui(variableLocation, uvec4.x, uvec4.y, uvec4.z, uvec4.w);
}

void ShaderProgram::uploadIVec2(const std::string& variableName, const glm::ivec2& ivec2) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform2i(variableLocation, ivec2.x, ivec2.y);
}

void ShaderProgram::uploadIVec3(const std::string& variableName, const glm::ivec3& ivec3) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform3i(variableLocation, ivec3.x, ivec3.y, ivec3.z);
}

void ShaderProgram::uploadIVec4(const std::string& variableName, const glm::ivec4& ivec4) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform4i(variableLocation, ivec4.x, ivec4.y, ivec4.z, ivec4.w);
}

// array
void ShaderProgram::uploadVec2Array(const std::string& variableName, const glm::vec2* vec2, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform2fv(variableLocation, length, &((*vec2)[0]));
}

void ShaderProgram::uploadVec3Array(const std::string& variableName, const glm::vec3* vec3, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform3fv(variableLocation, length, &((*vec3)[0]));
}

void ShaderProgram::uploadVec4Array(const std::string& variableName, const glm::vec4* vec4, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform4fv(variableLocation, length, glm::value_ptr(vec4[0]));
}

void ShaderProgram::uploadUVec2Array(const std::string& variableName, const glm::uvec2* uvec2, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform2uiv(variableLocation, length, glm::value_ptr(uvec2[0]));
}

void ShaderProgram::uploadUVec3Array(const std::string& variableName, const glm::uvec3* uvec3, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform3uiv(variableLocation, length, glm::value_ptr(uvec3[0]));
}

void ShaderProgram::uploadUVec4Array(const std::string& variableName, const glm::uvec4* uvec4, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform4uiv(variableLocation, length, glm::value_ptr(uvec4[0]));
}

void ShaderProgram::uploadIVec2Array(const std::string& variableName, const glm::ivec2* ivec2, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform2iv(variableLocation, length, glm::value_ptr(ivec2[0]));
}

void ShaderProgram::uploadIVec3Array(const std::string& variableName, const glm::ivec3* ivec3, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform3iv(variableLocation, length, glm::value_ptr(ivec3[0]));
}

void ShaderProgram::uploadIVec4Array(const std::string& variableName, const glm::ivec4* ivec4, const uint32_t length) {
	int32_t variableLocation = getVariableLocation(*this, variableName);
	glUniform4iv(variableLocation, length, glm::value_ptr(ivec4[0]));
}

void ShaderProgram::uploadFloat(const std::string& variableName, const float value) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1f(vairalbleLocation, value);
}

void ShaderProgram::uploadFloatArray(const std::string& variableName, const uint32_t length, const float* array) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1fv(vairalbleLocation, length, array);
}

void ShaderProgram::uploadInt(const std::string& variableName, const int32_t value) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1i(vairalbleLocation, value);
}

void ShaderProgram::uploadIntArray(const std::string& variableName, const uint32_t length, const int32_t* array) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1iv(vairalbleLocation, length, array);
}

void ShaderProgram::uploadUInt(const std::string& variableName, const uint32_t value) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1ui(vairalbleLocation, value);
}
void ShaderProgram::uploadUIntArray(const std::string& variableName, const uint32_t length, const uint32_t* array) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1uiv(vairalbleLocation, length, array);
}

void ShaderProgram::uploadU64(const std::string& variableName, const uint64_t value) {
	uint64_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniformHandleui64ARB(vairalbleLocation, value);
}

void ShaderProgram::uploadU64Array(const std::string& variableName, const uint32_t length, const uint64_t* array) {

}

void ShaderProgram::uploadBool(const std::string& variableName, const bool value) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniform1ui(vairalbleLocation, value ? 1 : 0);
}

void ShaderProgram::uploadMat2(const std::string& variableName, const glm::mat2& mat2) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniformMatrix2fv(vairalbleLocation, 1, GL_FALSE, &mat2[0][0]);
}

void ShaderProgram::uploadMat3(const std::string& variableName, const glm::mat3& mat3) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniformMatrix3fv(vairalbleLocation, 1, GL_FALSE, &mat3[0][0]);
}

void ShaderProgram::uploadMat4(const std::string& variableName, const glm::mat4& mat4) {
	int32_t vairalbleLocation = getVariableLocation(*this, variableName);
	glUniformMatrix4fv(vairalbleLocation, 1, GL_FALSE, &mat4[0][0]);
}

// private shader

void ShaderProgram::clearShaderVariables() {
	allAvaliableShaderLocations.clear();
}

bool ShaderProgram::validateShaderProgramLinking() {
	if (!*this) {
		return false;
	}

	int32_t success = 0;
	glGetProgramiv(m_id, GL_LINK_STATUS, &success);
	if (success) {
		return true;
	}

	int32_t infoLogLength = 0;
	glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (!infoLogLength) {
		std::println("[Program] Failed to link: no info log available");
		return false;
	}

	std::string infoLog(infoLogLength, 0);
	glGetProgramInfoLog(m_id, infoLogLength, nullptr, infoLog.data());
	std::print("[Program] Failed to link: {0}", infoLog);

	return false;
}

// private

static void retreiveActiveUniforms(const ShaderProgram& shaderProgram) {
	int32_t numberOfUniforms = 0;
	glGetProgramiv(shaderProgram.getId(), GL_ACTIVE_UNIFORMS, &numberOfUniforms);

	int32_t maxCharacterLength = 0;
	glGetProgramiv(shaderProgram.getId(), GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxCharacterLength);

	if (!numberOfUniforms && !maxCharacterLength) {
		//std::println("No uniforms exist.");
		return;
	}

	std::vector<char> characterBuffer{};
	characterBuffer.resize(maxCharacterLength);

	for (int32_t i = 0; i < numberOfUniforms; i++) {
		int32_t length = 0;
		int32_t size = 0;
		uint32_t dataType = 0;
		glGetActiveUniform(shaderProgram.getId(), i, maxCharacterLength, &length, &size, &dataType, characterBuffer.data());

		if (!length) {
			continue;
		}

		int32_t variableLocation = glGetUniformLocation(shaderProgram.getId(), characterBuffer.data());
		ShaderVariable shaderVariable{
			std::string(characterBuffer.data(), length),
			variableLocation,
			shaderProgram.getId(),
		};
		allAvaliableShaderLocations.emplace(shaderVariable);
	}

}

static int32_t getVariableLocation(const ShaderProgram& shaderProgram, const std::string& variableName) {
	ShaderVariable match = {
		variableName,
		0,
		shaderProgram.getId()
	};

	auto iterator = allAvaliableShaderLocations.find(match);
	if (iterator != allAvaliableShaderLocations.end()) {
		return iterator->variableLocation;
	}

	return -1;
}