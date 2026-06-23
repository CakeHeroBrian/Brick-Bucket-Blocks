#pragma once

#include <string>
#include <cstdint>


class Shader {
public:
	enum struct Type : uint8_t {
		Vertex = 0,
		Fragment = 1,
		Compute = 2,
		Geometry = 3,
		Invalid = 4
	};

	struct CreateInfo {
		std::string filepath = "";
		Type type = Type::Invalid;
	};
public:
	Shader(const Shader::CreateInfo& createInfo);
	~Shader();

	[[nodiscard]] explicit operator bool() const;

	uint32_t getId() const { return m_id; }
private:
	std::string readFile(const std::string& filepath);
	uint32_t shaderTypeToGLType(const Shader::Type type);
	std::string getNameFromType(const Shader::Type type);

	bool validateShaderCompilation(const Shader::Type type, const std::string& filepath);
private:
	uint32_t m_id = 0;
private:
	static const uint32_t INVALID_ID = UINT32_MAX;
};