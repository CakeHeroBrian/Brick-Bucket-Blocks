#include <rendering/texture_manager.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb.hpp>


#include <print>

TextureManager::TextureManager(const TextureManager::CreateInfo& createInfo) {
	m_textureFilepathBase = createInfo.textureFolder;

	m_textureWidth = createInfo.textureWidth;
	m_textureHeight = createInfo.textureHeight;
	m_textureLayers = createInfo.textureLayers;

	glCreateTextures(
		GL_TEXTURE_2D_ARRAY,
		1,
		&m_textureId
	);

	uint32_t mipLevels = static_cast<uint32_t>(
		std::floor(std::log2(std::max(m_textureWidth, m_textureHeight)))
		) + 1;

	glTextureStorage3D(
		m_textureId,
		mipLevels,
		GL_RGBA8,
		m_textureWidth,
		m_textureHeight,
		m_textureLayers
	);

	glTextureParameteri(m_textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureId, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTextureParameteri(m_textureId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

TextureManager::~TextureManager() {
	cleanup();
}

TextureManager::TextureManager(TextureManager&& other) noexcept :
	m_textureNameToLayerId{std::move(other.m_textureNameToLayerId)},
	m_textureFilepathBase{std::move(other.m_textureFilepathBase)},
	m_currentImageIndex{other.m_currentImageIndex },
	m_availableIndices{ std::move(other.m_availableIndices) },\
	m_textureWidth{ other.m_textureWidth },
	m_textureHeight{ other.m_textureHeight },
	m_textureLayers{ other.m_textureLayers },
	m_textureId{other.m_textureId }
{
	other.m_textureNameToLayerId = {};
	other.m_textureFilepathBase = "";

	other.m_textureId = INVALID_TEXTURE_ID;

	other.m_availableIndices = {};
}

TextureManager& TextureManager::operator=(TextureManager&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	if (*this) {
		cleanup();
	}

	m_textureNameToLayerId = std::move(other.m_textureNameToLayerId);
	m_textureFilepathBase = std::move(other.m_textureFilepathBase);
	m_currentImageIndex = other.m_currentImageIndex;
	m_availableIndices = std::move(other.m_availableIndices);
	m_textureWidth = other.m_textureWidth;
	m_textureHeight = other.m_textureHeight;
	m_textureLayers = other.m_textureLayers;
	m_textureId = other.m_textureId;

	other.m_textureNameToLayerId = {};
	other.m_textureFilepathBase = "";
	other.m_textureId = INVALID_TEXTURE_ID;
	other.m_availableIndices = {};

	return *this;
}

TextureManager::operator bool() const noexcept {
	return m_textureId != INVALID_TEXTURE_ID;
}

void TextureManager::addTexture(std::string_view textureName) noexcept {
	if (!*this) {
		std::println("Cannot add texture to an unintialized texture manager");
		return;
	}

	if (m_currentImageIndex >= m_textureLayers) {
		std::println("Texture2DArray is full");
	}

	uint16_t id = UINT16_MAX;
	if (!m_availableIndices.empty()) {
		id = m_availableIndices.front();
		m_availableIndices.pop();
	}
	else {
		id = m_currentImageIndex;
		++m_currentImageIndex;
	}

	std::string texture{ textureName };
	auto textureIt = m_textureNameToLayerId.find(texture);
	if (textureIt == m_textureNameToLayerId.end()) {
		int32_t textureWidth = 0;
		int32_t textureHeight = 0;
		int32_t textureChannels = 0;

		std::string filepath = m_textureFilepathBase + texture + ".png";
		uint8_t* pixels = stbi_load(
			filepath.c_str(),
			&textureWidth,
			&textureHeight,
			&textureChannels,
			STBI_rgb_alpha
		);

		if (!pixels) {
			std::println("Failed to load: {}", filepath);
			stbi_image_free(pixels);
			return;
		}

		if (textureWidth != m_textureWidth || textureHeight != m_textureHeight || textureChannels != 4) {
			std::println("{} is not a valid textures, enter a texture that is {}x{} with 4 channels", filepath, m_textureWidth, m_textureHeight);
			stbi_image_free(pixels);
			return;
		}

		glTextureSubImage3D(
			m_textureId,
			0,
			0,
			0,
			id,
			m_textureWidth,
			m_textureHeight,
			1,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			pixels
		);
		
		m_textureNameToLayerId[texture] = id;
		stbi_image_free(pixels);
		return;
	}

	std::println("Texture: {} already exists", textureName);
}

void TextureManager::removeTexture(std::string_view textureName) noexcept {
	if (!*this) {
		std::println("Cannot remove textures from an uninitialized texture manager.");
		return;
	}

	std::string texture{ textureName };
	auto textureIt = m_textureNameToLayerId.find(texture);
	if (textureIt != m_textureNameToLayerId.end()) {
		m_availableIndices.push(textureIt->second);
		m_textureNameToLayerId.erase(texture);
		return;
	}

	std::println("Texture: {} does not exist.", textureName);
}

uint16_t TextureManager::getTextureId(std::string_view textureName) const noexcept {
	if (!*this) {
		std::println("Cannot get texture id from uninitialized texture manager");
		return UINT16_MAX;
	}

	auto textureIt = m_textureNameToLayerId.find(std::string(textureName));
	if (textureIt == m_textureNameToLayerId.end()) {
		std::println("Counld not find {}", textureName);
		return UINT16_MAX;
	}

	return textureIt->second;
}

void TextureManager::bind(uint32_t bindSlot) const noexcept {
	glBindTextureUnit(bindSlot, m_textureId);
}

void TextureManager::generateMipMaps() const noexcept {
	if (!*this) {
		return;
	}

	glGenerateTextureMipmap(m_textureId);
}

// private

void TextureManager::cleanup() noexcept {
	if (m_textureId != INVALID_TEXTURE_ID) {
		glDeleteTextures(1, &m_textureId);
		m_textureId = INVALID_TEXTURE_ID;
	}

	if (!m_textureNameToLayerId.empty()) {
		m_textureNameToLayerId.clear();
	}
}