#pragma once

#include <array>
#include <atomic>

#include <glm/glm.hpp>

class Chunk;

struct Region {
	glm::ivec3 position{ 0 };
	std::array<std::atomic<Chunk*>, 4 * 4 * 6> chunks{};

	std::array<std::atomic<uint16_t>, (64 * 4)* (64 * 4)> heightmap{};

	glm::vec3 bbMin{ 0.0f };
	glm::vec3 bbMax{ 0.0f };
};

inline int32_t floorDiv(int32_t a, int32_t b) {
	int32_t res = a / b;
	int32_t rem = a % b;
	if (rem != 0 && ((a ^ b) < 0)) {
		res--;
	}
	return res;
}

inline int32_t floorMod(int32_t a, int32_t b) {
	return ((a % b) + b) % b;
}

static inline glm::i32vec3 calculateRegionPosition(int32_t chunkX, int32_t chunkZ) noexcept {
	return {
		floorDiv(chunkX, 4),
		0,
		floorDiv(chunkZ, 4)
	};
}

static constexpr inline size_t calulateRegionHeightMapIndex(int32_t regionX, int32_t regionZ) noexcept {
	return regionZ + (regionX * 64 * 4);
}

static constexpr inline size_t calulateChunkIndexInRegion(int32_t chunkX, int32_t chunkY, int32_t chunkZ) noexcept {
	return chunkX + (chunkZ * 4) + (chunkY * 4 * 4);
}

static constexpr inline uint16_t getRegionPositionHeight(int32_t regionX, int32_t regionZ, Region* region) noexcept {
	if (!region) {
		return 0;
	}

	return (region->heightmap[calulateRegionHeightMapIndex(regionX, regionZ)]).load(std::memory_order_acquire);
}

static void setRegionPositionHeight(int32_t regionX, int32_t regionZ, Region* region, uint16_t newHeight) {
	if (!region) {
		return;
	}

	(region->heightmap[calulateRegionHeightMapIndex(regionX, regionZ)]).store(newHeight, std::memory_order_release);
}