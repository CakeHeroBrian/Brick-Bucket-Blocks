#pragma once

//#include "client_pch.hpp"

#include <rendering/shaders/shader.hpp>

#include <vector>

#include <glm/glm.hpp>

class ShaderProgram {
public:
	ShaderProgram() = default;
	ShaderProgram(const std::vector<Shader::CreateInfo>& shaderCreateInfos);
	~ShaderProgram();

	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;

	ShaderProgram(ShaderProgram&& other) noexcept;
	ShaderProgram& operator=(ShaderProgram&& other) noexcept;

	[[nodiscard]] explicit operator bool() const;

	void bind();
	void unbind();

	const uint32_t getId() const { return m_id; }

	void uploadVec2(const std::string& variableName, const glm::vec2& vec2);
	void uploadVec3(const std::string& variableName, const glm::vec3& vec3);
	void uploadVec4(const std::string& variableName, const glm::vec4& vec4);

	void uploadUVec2(const std::string& variableName, const glm::uvec2& uvec2);
	void uploadUVec3(const std::string& variableName, const glm::uvec3& uvec3);
	void uploadUVec4(const std::string& variableName, const glm::uvec4& uvec4);

	void uploadIVec2(const std::string& variableName, const glm::ivec2& ivec2);
	void uploadIVec3(const std::string& variableName, const glm::ivec3& ivec3);
	void uploadIVec4(const std::string& variableName, const glm::ivec4& ivec4);

	// array
	void uploadVec2Array(const std::string& variableName, const glm::vec2* vec2, const uint32_t length);
	void uploadVec3Array(const std::string& variableName, const glm::vec3* vec3, const uint32_t length);
	void uploadVec4Array(const std::string& variableName, const glm::vec4* vec4, const uint32_t length);

	void uploadUVec2Array(const std::string& variableName, const glm::uvec2* uvec2, const uint32_t length);
	void uploadUVec3Array(const std::string& variableName, const glm::uvec3* uvec3, const uint32_t length);
	void uploadUVec4Array(const std::string& variableName, const glm::uvec4* uvec4, const uint32_t length);

	void uploadIVec2Array(const std::string& variableName, const glm::ivec2* ivec2, const uint32_t length);
	void uploadIVec3Array(const std::string& variableName, const glm::ivec3* ivec3, const uint32_t length);
	void uploadIVec4Array(const std::string& variableName, const glm::ivec4* ivec4, const uint32_t length);


	void uploadFloat(const std::string& variableName, const float value);
	void uploadFloatArray(const std::string& variableName, const uint32_t length, const float* array);

	void uploadInt(const std::string& variableName, const int32_t value);
	void uploadIntArray(const std::string& variableName, const uint32_t length, const int32_t* array);

	void uploadUInt(const std::string& variableName, const uint32_t value);
	void uploadUIntArray(const std::string& variableName, const uint32_t length, const uint32_t* array);

	void uploadU64(const std::string& variableName, const uint64_t value);
	void uploadU64Array(const std::string& variableName, const uint32_t length, const uint64_t* array);

	void uploadBool(const std::string& variableName, const bool value);
	void uploadBoolArray(const std::string& variableName, const uint32_t length, const bool* array);

	void uploadMat2(const std::string& variableName, const glm::mat2& mat2);
	void uploadMat3(const std::string& variableName, const glm::mat3& mat3);
	void uploadMat4(const std::string& variableName, const glm::mat4& mat4);
private:
	bool validateShaderProgramLinking();

	void clearShaderVariables();
private:
	uint32_t m_id = INVALID_PROGRAM_ID;
private:
	static const uint32_t INVALID_PROGRAM_ID = UINT32_MAX;
};