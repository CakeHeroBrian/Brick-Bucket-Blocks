#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <queue>

#include <glad/glad.h>

class TextureManager {
public:
	struct CreateInfo {
		std::string textureFolder = "assets/textures/";

		uint16_t textureWidth = 16;
		uint16_t textureHeight = 16;
		uint16_t textureLayers = 256;
	};

	static constexpr uint32_t INVALID_TEXTURE_ID = UINT32_MAX;
public:
	TextureManager() = default;
	TextureManager(const TextureManager::CreateInfo& createInfo);
	~TextureManager();

	TextureManager(const TextureManager&) = delete;
	TextureManager operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept;
	TextureManager& operator=(TextureManager&& other) noexcept;

	explicit operator bool() const noexcept;

	void addTexture(std::string_view textureName) noexcept;
	void removeTexture(std::string_view textureName) noexcept;
	uint16_t getTextureId(std::string_view textureName) const noexcept;

	void generateMipMaps() const noexcept;

	void bind(uint32_t bindSlot) const noexcept;
private:
	void cleanup();
private:
	std::unordered_map<std::string, uint16_t> m_textureNameToLayerId{};
	std::string m_textureFilepathBase = "assets/textures/";

	uint16_t m_currentImageIndex = 0;
	std::queue<uint16_t> m_availableIndices{};

	uint32_t m_textureWidth = 0;
	uint32_t m_textureHeight = 0;
	uint32_t m_textureLayers = 0;
	uint32_t m_textureId = INVALID_TEXTURE_ID;
};