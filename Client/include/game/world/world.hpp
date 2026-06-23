#pragma once

#include <game/world/blocks/block.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <vector>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <fast_noise_lite/fast_noise_lite.hpp>
#include <gtl/phmap.hpp>
#include <gtl/vector.hpp>

#include <moody_camel/concurrentqueue.h>
#include <moody_camel/blockingconcurrentqueue.h>

//#include <game/world/Chunk.hpp>

struct IVec3Hash {
	std::size_t operator()(const glm::i32vec3& k) const {
		// MurmurHash3 32-bit integer mix
		uint32_t a = k.x;
		uint32_t b = k.y;
		uint32_t c = k.z;

		a ^= b ^ (c >> 18); a *= 0x85ebca6b;
		b ^= c ^ (a >> 22); b *= 0xc2b2ae35;
		c ^= a ^ (b >> 16); c *= 0x094be39d;
		a ^= b ^ (c >> 18); a *= 0x85ebca6b;

		return a;
	}
};

class Chunk;
class TextureManager;

struct VertexData;
//struct VertexInfo;

struct Region;

struct ChunkUploadData {
	std::vector<VertexData> solidVertices{};
	std::vector<VertexData> transparentVertices{};

	Chunk* chunk = nullptr;
};

class World {
public:
	struct BlockInfo {
		std::vector<Block::SideData> sideData{};
		std::string_view blockName = "Missing";
		Block::Type type = Block::Type::Solid;
	};

	struct CreateInfo {
		std::vector<BlockInfo> blockCreateInfos{};

		TextureManager& textureManager;
	};

	struct Biome {
		std::string name = "Plains";

		int32_t baseHeight = 0;
		int32_t amplitude = 1;
		float frequency = 0.001f;

		const Block* topBlock = nullptr;
		const Block* subsurfaceBlock = nullptr;
		const Block* stoneBlock = nullptr;
		int32_t subsurfaceDepth = 3;

		float minTemperature = 0.0f;
		float maxTemperature = 1.0f;

		float minHumidity = 0.0f;
		float maxHumidity = 1.0f;
	};

	struct BiomeSample {
		const Biome* biome = nullptr;

		float rawValue = 0.0f;
		float terrainHeight = 0.0f;

		float blendedBase = 0.0f;
		float blendedAmpplifier = 0.0f;

		float continentalBase = 0.0f;
		float blendedContinentalness = 0.0f;

		float errosionBase = 0.0f;
		float blendedErrosion = 0.0f;
		
		float ridgeBase = 0.0f;
		float blendedRidge = 0.0f;

		float weirdnessBase = 0.0f;
		float blendedWeirdness = 0.0f;

		float blendedOceanWeight = 0.0f;
		float blendedMountainWeight = 0.0f;

		float blendedRiverNoise = 0.0f;

		float temperatureBase = 0.0f;
		float blendedTemperature = 0.0f;

		float humidityBase = 0.0f;
		float blendedHumidity = 0.0f;

		float rawOceanWeight = 0.0f;
		float rawMountainWeight = 0.0f;
	};

	struct BiomeMap {
		static constexpr int32_t MARGIN = 64;
		static constexpr int32_t STEP = 2;

		static constexpr int32_t EXTENT = (64 + MARGIN * 2 + STEP - 1) / STEP + 1;

		std::vector<BiomeSample> samples{ EXTENT * EXTENT };

		const BiomeSample& get(int32_t x, int32_t z) const {
			int32_t safeX = std::clamp(x, 0, EXTENT - 1);
			int32_t safeZ = std::clamp(z, 0, EXTENT - 1);
			return samples[safeX * EXTENT + safeZ];
		}

		float boundryDistance(int32_t chunkX, int32_t chunkZ, size_t numberBiomes) const {
			float slot = get(chunkX, chunkZ).rawValue * (float)numberBiomes;
			float low = slot - std::floor(slot);
			float high = std::ceil(slot) - slot;
			return std::min(low, high);
		}

		bool isNearOcean(int32_t chunkX, int32_t chunkZ, const World& world) const;
	};

	struct ColumnContext {
		const Biome* biome = nullptr;

		uint32_t maxHeight = 0;
		bool isBeach = false;
		bool isRiver = false;
		float riverBlend = 0.0f;
		float oceanWeight = 0.0f;
		float mountainWeight = 0.0f;
		float beachWeight = 0.0f;

		float temperature = 0.0f;
		float humidity = 0.0f;
		uint32_t localY = 0.0f;
	};
public:
	World() = default;
	World(const World::CreateInfo& createInfo);
	~World();

	World(const World&) = delete;
	World& operator=(const World&) = delete;

	World(World&&) noexcept;
	World& operator=(World&&) noexcept;

	explicit operator bool() const noexcept;

	const Block& getBlock(const std::string& name) const noexcept { return m_namesToBlocks.at(name); }

	const gtl::flat_hash_map<glm::ivec3, Chunk*, IVec3Hash>& getChunks() const noexcept {
		return m_chunks;
	}

	const gtl::flat_hash_map<glm::ivec3, std::unique_ptr<Region>, IVec3Hash>& getRegions() const noexcept {
		return m_regions;
	}

	void processChunkUploads(uint32_t vao);

	void sortChunks(const glm::vec3& position, gtl::vector<Chunk*>& visibleChunks);

	Chunk* getChunk(glm::ivec3 chunkPosition) noexcept;

	void update(const glm::vec3& playerPosition) noexcept;

	void addChunkToRemeshingQueue(Chunk* chunk) noexcept;

	float getSurfaceAt(float worldX, float worldZ) const noexcept;
private:
	void cleanup();

	static void generateChunk(World& world);
	static void generateChunkMesh(World& world);

	void createRegion(const glm::i32vec3 regionPosition, Region*& region);

	void updateHeightMapForChunk(Chunk* chunk, Region* region);

	const Biome& getBiomeAt(int32_t worldX, int32_t worldZ) const;

	World::BiomeMap buildBiomeMap(glm::i32vec3 chunkPosition) const;
	World::ColumnContext classifyColumn(const BiomeMap& biomeMap, int32_t worldX, int32_t worldZ, glm::i32vec3 chunkPosition, int32_t localX, int32_t localZ) const noexcept;
	const Block& selectBlock(
		const ColumnContext& column,
		int32_t worldY,
		const Block& air,
		const Block& water,
		const Block& sand
	) const;

	const Biome* selectBiomeFromParameters(
		float temperature,
		float humidity,
		float continentalness
	) const noexcept;
private:
	static constexpr const int32_t SEA_LEVEL = 63;
private:
	std::unordered_map< std::string, Block> m_namesToBlocks{};

	gtl::flat_hash_map<glm::ivec3, Chunk*, IVec3Hash> m_chunks{};


	gtl::flat_hash_map<glm::ivec3, std::unique_ptr<Region>, IVec3Hash> m_regions{};

	std::vector<std::jthread> m_chunkGenerationThreads{};
	moodycamel::BlockingConcurrentQueue<Chunk*> m_chunkGenerationQueue{};

	std::vector<std::jthread> m_chunkMeshGenerationThreads{};
	moodycamel::BlockingConcurrentQueue<Chunk*> m_chunkMeshGenerationQueue{};
	moodycamel::ConcurrentQueue<Chunk*> m_chunkMeshGenerationHoldingQueue{};

	moodycamel::BlockingConcurrentQueue<ChunkUploadData> m_chunkUploadQueue{};

	FastNoiseLite m_noise{};
	FastNoiseLite m_biomeNoise{};
	FastNoiseLite m_transitionNoise{};

	FastNoiseLite m_contentalnessNoise{};
	FastNoiseLite m_tempuratureNoise{};
	FastNoiseLite m_humdityNoise{};
	FastNoiseLite m_errosionNoise{};
	FastNoiseLite m_ridgedNoise{};
	FastNoiseLite m_weirdnessNoise{};
	FastNoiseLite m_continentalWarpNoise{};

	FastNoiseLite m_riverNoise{};
	FastNoiseLite m_riverWarpNoise{};

	std::vector<Biome> m_biomes{};

	std::atomic<bool> m_isRunning = false;

	std::vector<Chunk*> m_pendingDeletion{};
	std::vector<Chunk*> m_deletionGraveyard{};
};