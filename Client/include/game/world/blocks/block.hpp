#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>

class TextureManager;

class Block {
public:
	enum class Side : uint8_t {
		Back = 0,
		Front = 1,
		Left = 2,
		Right = 3,
		Top = 4,
		Bottom = 5,
		Sides = 6,
		All = 7
	};

	enum class Type : uint8_t {
		Solid = 0,
		Trasparent = 1,
		Air = 2,
		Water = 3,
	};

	struct SideData {
		std::string_view texture = "";
		Side side = Side::All;
	};

	struct CreateInfo {
		std::vector<SideData> sideData{};
		std::string_view blockName = "Missing";
		Type type = Type::Solid;

		TextureManager& textureManager;
	};
public:
	Block() = default;
	Block(const Block::CreateInfo& createInfo);
	~Block();

	Block(const Block&) = delete;
	Block& operator=(const Block&) = delete;

	Block(Block&& other) noexcept;
	Block& operator=(Block&& other) noexcept;

	uint16_t operator[](uint8_t index) const noexcept;

	Type getType() const noexcept { return m_type; }

	std::string_view getName() const noexcept { return m_name; }

	constexpr inline bool isTransparent() const noexcept { return m_type == Block::Type::Air || m_type == Block::Type::Trasparent || isWater(); }
	constexpr inline bool isAir() const noexcept { return m_type == Block::Type::Air; }
	constexpr inline bool isWater() const noexcept { return m_type == Block::Type::Water; }
	constexpr inline bool hasPhysics() const noexcept {return m_type == Block::Type::Solid || m_type == Block::Type::Trasparent;}
private:
	static constexpr const uint8_t SIDE_COUNT = 6;
private:
	void cleanup() noexcept;
private:
	std::string m_name = "Missing";
	std::array<uint16_t, SIDE_COUNT> m_sideTextures{};
	
	Type m_type = Type::Solid;
};