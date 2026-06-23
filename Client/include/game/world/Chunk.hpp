#pragma once

#include <game/world/blocks/block.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <unordered_map>
#include <array>
#include <atomic>

#include <gtl/phmap.hpp>

#include <game/world/world.hpp>

struct VertexData {
	uint32_t faceData = 0;
	uint32_t skylightData = 0;

	uint32_t convertColorToPacked(glm::vec3 color) {
		glm::u32vec3 uintColor = glm::u32vec3(color);

		return (uintColor.x << 24) | (uintColor.y << 16) | (uintColor.z << 8);
	}
};

struct VertexInfo {
	std::vector<VertexData> solidVertices{};
	std::vector<VertexData> transparentVertices{};
};

struct BlockPtrHash {
	size_t operator()(const Block* blockPtr) const noexcept {
		auto ptr = reinterpret_cast<uintptr_t>(blockPtr);

		return ptr * 11400714819323198485ULL >> (64 - 20);
	}
};



enum class LightState {
	Unlit,
	LocalLit,
	FullyPropagated
};

class Chunk {
public:
	struct CreateInfo {
		std::array<Chunk*, 6> neightborChunks{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		const Block* airBlock = nullptr;

		Region* region = nullptr;
		glm::i32vec2 regionLocalBase{ 0 };

		glm::i32vec3 chunkPosition{ 0, 0, 0 };
	};

	static constexpr const uint32_t CHUNK_WIDTH = 64;
	static constexpr const uint32_t CHUNK_HEIGHT = 64;
	static constexpr const uint32_t CHUNK_DEPTH = 64;

	static constexpr const uint32_t CHUNK_AREA = CHUNK_WIDTH * CHUNK_HEIGHT;
	static constexpr const uint32_t CHUNK_VOLUME = CHUNK_AREA * CHUNK_DEPTH;

	static constexpr const glm::vec3 CHUNK_SIZE = glm::vec3(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH);
	static constexpr const glm::i8vec3 CHUNK_END = glm::i8vec3(CHUNK_WIDTH - 1, CHUNK_HEIGHT - 1, CHUNK_DEPTH - 1);

	static constexpr const std::array<glm::i8vec3, 6> DIRECTIONS = {
		glm::i8vec3(0, 0, -1),
		glm::i8vec3(0, 0, 1),
		glm::i8vec3(-1, 0, 0),
		glm::i8vec3(1, 0, 0),
		glm::i8vec3(0, 1, 0),
		glm::i8vec3(0, -1, 0)
	};

	struct DrawArraysIndirectCommand {
		uint32_t count = 0;
		uint32_t instanceCount = 0;
		uint32_t first = 0;
		uint32_t baseInstance = 0;
	};

	static constexpr uint32_t INVALID_BUFFER_ID = UINT32_MAX;

public:
	Chunk() = default;
	Chunk(const Chunk::CreateInfo& createInfo);
	~Chunk();

	Chunk(const Chunk&) = delete;
	Chunk& operator=(const Chunk&) = delete;

	Chunk(Chunk&&) = delete;
	Chunk& operator=(Chunk&&) = delete;

	void setBlock(uint8_t x, uint8_t y, uint8_t z, const Block& block) noexcept;
	void setBlockRaw(uint8_t x, uint8_t y, uint8_t z, const Block& block) noexcept;

	VertexInfo generateChunkMesh();

	void uploadChunkMesh(const std::vector<VertexData>& solidVertices, const std::vector<VertexData>& transparentVertices) noexcept;

	uint32_t getDrawCount() const noexcept { return m_drawCount; }
	uint32_t getTransparentDrawCount() const noexcept { return m_transparentDrawCount; }
	uint32_t getDrawBufferId() const noexcept { return m_meshDataId; }
	uint32_t getTransparentDrawBufferId() const noexcept { return m_transparentMeshData; }
	uint32_t getDrawCommandId() const noexcept { return m_drawCommandId; }
	uint32_t getTransparentDrawCommandId() const noexcept { return m_transparentDrawCommandId; }

	glm::mat4 getChunkTransfrom() const noexcept { return  m_transformMatrix; }

	void setNeighborChunk(uint32_t face, Chunk* chunk) noexcept {
		Chunk* toStore = chunk;
		if (toStore && !toStore->tryAddReference()) {
			toStore = nullptr;
		}

		Chunk* old = m_neightborChunks[face];
		m_neightborChunks[face] = toStore;
		if (old) {
			old->subReference();
		}
		

		bool hadNeighbor = old != nullptr;
		bool hasNeighbor = toStore != nullptr;

		//m_neightborChunks[face].store(chunk, std::memory_order_release);

		if (!hadNeighbor && hasNeighbor) {
			m_requiredNeighbors++;//.fetch_add(1u, std::memory_order_relaxed);
		}
		else if (hadNeighbor && !hasNeighbor) {
			m_requiredNeighbors--;// .fetch_sub(1, std::memory_order_relaxed);
		}
	}

	glm::vec3 getBBMin() const noexcept { return m_chunkBoundingBoxMin; }
	glm::vec3 getBBMax() const noexcept { return m_chunkBoundingBoxMax; }
	glm::i32vec3 getPosition() const noexcept { return m_chunkPosition; }
	Region* getRegion() noexcept { return m_region; }

	Chunk* getNeighbor(uint8_t index) noexcept { return m_neightborChunks[index]; }

	bool clearNeighborIfMatching(uint32_t face, Chunk* expected) noexcept {
		if (m_neightborChunks[face] == expected) {
			m_neightborChunks[face] = nullptr;
			m_requiredNeighbors--;// .fetch_sub(1, std::memory_order_relaxed);
			if (expected) {
				expected->subReference();
			}
		
			return true;
		}

		//m_neightborChunks[face] = nullptr;
		return false;
	}

	void setIsGenerated(bool value) noexcept { m_isGenerated.store(value, std::memory_order_release); }
	void setIsMeshGenerated(bool value) noexcept { m_isMeshGenerated.store(value, std::memory_order_release); }
	void incrementNeighborGenerated() noexcept { m_neighborCount.fetch_add(1, std::memory_order_release); }
	void incrementNeighborLit() noexcept { m_litNeighborsCount.fetch_add(1, std::memory_order_release); }
	void setLightState(LightState value) noexcept { m_lightState.store(value, std::memory_order_release); }
	void setIsRemeshSource(bool value) noexcept { m_isRemeshSource.store(value, std::memory_order_release); }

	bool isGenerated() const noexcept { return m_isGenerated.load(std::memory_order_acquire); }
	bool isMeshGenerated() const noexcept { return m_isMeshGenerated.load(std::memory_order_acquire); }
	bool areNeighborsGenerated() const noexcept { return m_neighborCount.load(std::memory_order_acquire) >= m_requiredNeighbors; }
	bool areNeighborsLit() const noexcept { return m_litNeighborsCount.load(std::memory_order_acquire) >= m_requiredNeighbors; }
	bool isDying() const noexcept { return m_refCount.load(std::memory_order_acquire) < 0; }
	LightState getLightState() const noexcept { return m_lightState.load(std::memory_order_acquire); }
	bool isRemeshSource() const noexcept {return m_isRemeshSource.load(std::memory_order_acquire);
	}

	void clearAllNeighbors() noexcept {
		for (uint32_t face = 0; face < 6; face++) {
			Chunk* old = m_neightborChunks[face];
			m_neightborChunks[face] = nullptr;
			if (old) {
				old->subReference();
			}
		}
	}

	bool tryAddReference() {
		int32_t current = m_refCount.load(std::memory_order_relaxed);
		while (true) {
			if (current < 0) {
				return false;
			}

			if (m_refCount.compare_exchange_weak(
				current, current + 1,
				std::memory_order_acquire,
				std::memory_order_relaxed)) {
				return true;
			}
		}
	}
	void subReference() noexcept { 
		int32_t current = m_refCount.load(std::memory_order_relaxed);
		while (true) {
			if (current == 0 || current == -1) {
				return;
			}

			int32_t next = (current > 0) ? current - 1 : current + 1;
			if (m_refCount.compare_exchange_weak(
				current, next,
				std::memory_order_release,
				std::memory_order_relaxed)) {
				return;
			}
		}
	}

	bool markIsDying() noexcept {
		int32_t current = m_refCount.load(std::memory_order_relaxed);
		while (true) {
			if (current < 0) {
				return false;
			}

			if (m_refCount.compare_exchange_weak(
				current, -(current + 1),
				std::memory_order_acq_rel,
				std::memory_order_relaxed)) {
				return current == 0;
			}
		}
	}
	bool isInUse() const noexcept { 
		int32_t value = m_refCount.load(std::memory_order_acquire);
		return value > 0 || value < -1;
	}

	const Block* getBlock(uint8_t x, uint8_t y, uint8_t z) noexcept { return m_localBlocks[m_blocks[z + (y * CHUNK_DEPTH) + (x * CHUNK_AREA)]]; }

	void propagateLocalLight() noexcept;
	void floodFillFromNeighbors() noexcept;

	void removeLight(int32_t x, int32_t y, int32_t z) noexcept;
	void clearLight() noexcept;

	inline constexpr uint32_t calculateBlockIndex(uint8_t x, uint8_t y, uint8_t z) const noexcept { return z + (y * CHUNK_WIDTH) + (x * CHUNK_AREA); }
	inline constexpr uint32_t calculateBlockIndex(glm::i8vec3 position) const noexcept { return (uint8_t)position.z + ((uint8_t)position.y * CHUNK_WIDTH) + ((uint8_t)position.x * CHUNK_AREA); }

	inline bool isBlockTransparent(uint8_t x, uint8_t y, uint8_t z) const noexcept {
		return (m_transparenceyMask[y + (x * CHUNK_WIDTH)] >> z) & 1;
	}

	
	inline bool isBlockInBounds(uint8_t x, uint8_t y, uint8_t z) const noexcept {
		return x < CHUNK_WIDTH && y < CHUNK_HEIGHT && z < CHUNK_DEPTH;
	}

	// bit hack
	inline bool isBlockInBounds(glm::u8vec3 position) const noexcept {
		return position.x < CHUNK_WIDTH && position.y < CHUNK_HEIGHT && position.z < CHUNK_DEPTH;
	}

	void incrementGraveyardAge() noexcept { m_graveyardAge++; }
	uint8_t getGraveyardAge() const noexcept { return m_graveyardAge; }
private:

	inline glm::ivec3 worldToLocal(const glm::ivec3& worldPos, const glm::ivec3& chunkPos) const noexcept{
		glm::ivec3 base = chunkPos * glm::ivec3(CHUNK_WIDTH, CHUNK_HEIGHT, CHUNK_DEPTH);
		return worldPos - base;
	}
private:
	void onBlockChange(int32_t localX, int32_t localY, int32_t localZ, const Block* oldBlock, const Block* newBlock);
	uint16_t rescalColumn(int32_t regionX, int32_t regionZ);

	uint8_t getSunlight(int32_t worldX, int32_t worldY, int32_t worldZ);

	uint8_t getLightAt(uint32_t index) const noexcept {
		uint32_t word = index / 16;
		uint32_t shift = (index % 16) * 4;
		return (m_sunlightStorage[word] >> shift) & 0xF;
	}

	void setLightAt(uint32_t index, uint8_t level) noexcept {
		uint32_t word = index / 16;
		uint32_t shift = (index % 16) * 4;
		uint64_t mask = uint64_t(0xF) << shift;
		m_sunlightStorage[word] = (m_sunlightStorage[word] & ~mask) | (uint64_t(level & 0xF) << shift);
	}
private:
	uint32_t m_blockIdCounter = 0;
	std::queue<uint8_t> m_availableIds{};
	std::array<uint32_t, 256> m_localBlockRefCount{};
	gtl::flat_hash_map<const Block*, uint8_t> m_blockToIdLookup{};

	std::array<uint8_t, CHUNK_VOLUME> m_blocks{};
	std::array<const Block*, 256> m_localBlocks{};
	std::array<std::array<uint16_t, 6>, 256> m_localBlockIdToTextures{};
	std::array<uint64_t, CHUNK_AREA> m_transparenceyMask{};

	std::array<uint64_t, CHUNK_VOLUME / 16> m_sunlightStorage{};

	std::array<Chunk*, 6> m_neightborChunks{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	uint32_t m_drawCount = 0;
	uint32_t m_transparentDrawCount = 0;

	glm::i32vec3 m_chunkPosition{ 0, 0, 0 };

	glm::mat4 m_transformMatrix{ 1.0f };

	glm::vec3 m_chunkBoundingBoxMin{ 0.0f };
	glm::vec3 m_chunkBoundingBoxMax{ 0.0f };

	std::atomic<bool> m_isGenerated = false;
	std::atomic<bool> m_isMeshGenerated = false;
	std::atomic<uint8_t> m_neighborCount = 0;
	std::atomic<uint8_t> m_litNeighborsCount = 0;
	std::atomic<bool> m_isLightPropagated = false;
	std::atomic<LightState> m_lightState{ LightState::Unlit };
	std::atomic<bool> m_isRemeshSource = false;
	uint8_t m_requiredNeighbors = 0;

	uint32_t m_meshDataId = INVALID_BUFFER_ID;
	uint32_t m_transparentMeshData = INVALID_BUFFER_ID;

	uint32_t m_drawCommandId = INVALID_BUFFER_ID;
	uint32_t m_transparentDrawCommandId = INVALID_BUFFER_ID;

	Region* m_region{ nullptr };
	glm::i32vec2 m_regionLocalBase{0};

	std::vector<uint32_t> m_lightSeeds{};

	std::atomic<int32_t> m_refCount{ 0 };
	uint8_t m_graveyardAge;
};