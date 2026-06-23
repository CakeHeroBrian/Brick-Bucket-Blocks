#include <game/world/blocks/block.hpp>

#include <rendering/texture_manager.hpp>

#include <print>

Block::Block(const Block::CreateInfo& createInfo) {
	m_type = createInfo.type;
	m_name = createInfo.blockName;

	for (const auto& sideInfo : createInfo.sideData) {
		uint16_t id = createInfo.textureManager.getTextureId(sideInfo.texture);
		if (id == TextureManager::INVALID_TEXTURE_ID) {
			std::print("Failed to find texture, {}", sideInfo.texture);
			continue;
		}

		if (sideInfo.side == Side::All) {
			for (uint8_t side = 0; side < SIDE_COUNT; side++) {
				m_sideTextures[side] = id;
			}
		}
		else if (sideInfo.side == Side::Sides) {
			m_sideTextures[static_cast<uint8_t>(Side::Back)] = id;
			m_sideTextures[static_cast<uint8_t>(Side::Front)] = id;
			m_sideTextures[static_cast<uint8_t>(Side::Left)] = id;
			m_sideTextures[static_cast<uint8_t>(Side::Right)] = id;
		}
		else {
			m_sideTextures[static_cast<uint8_t>(sideInfo.side)] = id;
		}
	}
}

Block::~Block() {
	cleanup();
}

Block::Block(Block&& other) noexcept :
	m_sideTextures{std::move(other.m_sideTextures)},
	m_type{ other.m_type }
{
	other.m_sideTextures = std::array<uint16_t, Block::SIDE_COUNT>{ 0, 0, 0, 0, 0, 0 };
}

Block& Block::operator=(Block&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	m_sideTextures = std::move(other.m_sideTextures);
	m_type = other.m_type;

	other.m_sideTextures = std::array<uint16_t, Block::SIDE_COUNT>{ 0, 0, 0, 0, 0, 0 };

	return *this;
}

uint16_t Block::operator[](uint8_t index) const noexcept {
	return m_sideTextures[index % SIDE_COUNT];
}

// private

void Block::cleanup() noexcept {
	m_sideTextures = std::array<uint16_t, Block::SIDE_COUNT>{ 0, 0, 0, 0, 0, 0 };
}