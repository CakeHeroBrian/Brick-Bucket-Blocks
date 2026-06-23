#include <game/world/world.hpp>

#include <ranges>

#include <rendering/texture_manager.hpp>

#include <game/world/Chunk.hpp>

#include <glm/gtx/norm.hpp>

#include <chrono>

#include <glm/gtc/random.hpp>
#include <game/gameplay/player/raycasting.hpp>

#include <game/world/Region.hpp>

template<typename T>
constexpr T constexpr_exp(T x) {
	T sum = 1.0;
	T term = 1.0;
	for (int i = 1; i < 20; ++i) { // 20 iterations for double-precision stability
		term *= x / i;
		sum += term;
	}
	return sum;
}

static void enqueueMeshChunk(
	moodycamel::BlockingConcurrentQueue<Chunk*>& queue,
	Chunk* chunk) {

	queue.enqueue(chunk);
}

static void enqueueMeshChunks(
	moodycamel::BlockingConcurrentQueue<Chunk*>& queue,
	std::vector<Chunk*>& chunks) {

	queue.enqueue_bulk(chunks.data(), chunks.size());
}

World::World(const World::CreateInfo & createInfo) {
	m_isRunning.store(true, std::memory_order_release);

	for (const auto& blockInfo : createInfo.blockCreateInfos) {
		Block::CreateInfo blockCreateInfo{
			.sideData = std::move(blockInfo.sideData),
			.blockName = blockInfo.blockName,
			.type = blockInfo.type,
			.textureManager = createInfo.textureManager,
		};

		m_namesToBlocks.emplace(blockInfo.blockName, blockCreateInfo);
	}

	m_transitionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_transitionNoise.SetFrequency(0.02f);


	m_biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_biomeNoise.SetFrequency(0.003F);
	//m_biomeNoise.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);

	m_contentalnessNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_contentalnessNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_contentalnessNoise.SetFrequency(0.001f);
	m_contentalnessNoise.SetFractalOctaves(4);
	m_contentalnessNoise.SetFractalLacunarity(2.0f);
	m_contentalnessNoise.SetFractalGain(0.4f);

	m_tempuratureNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_tempuratureNoise.SetFrequency(0.0008f);
	m_tempuratureNoise.SetFractalOctaves(2);

	m_humdityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_humdityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_humdityNoise.SetFrequency(0.0012f);
	m_humdityNoise.SetFractalOctaves(3);
	m_humdityNoise.SetFractalGain(0.45f);

	m_errosionNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_errosionNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_errosionNoise.SetFrequency(0.0025f);
	m_errosionNoise.SetFractalOctaves(3);
	m_errosionNoise.SetFractalLacunarity(2.0f);
	m_errosionNoise.SetFractalGain(0.5f);

	m_ridgedNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_ridgedNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
	m_ridgedNoise.SetFrequency(0.005f);
	m_ridgedNoise.SetFractalOctaves(5);
	m_ridgedNoise.SetFractalLacunarity(2.2f);
	m_ridgedNoise.SetFractalGain(0.5f);

	m_weirdnessNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_weirdnessNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
	m_weirdnessNoise.SetFrequency(0.004f);
	m_weirdnessNoise.SetFractalOctaves(3);

	m_riverNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	m_riverNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_riverNoise.SetFrequency(0.002f);
	m_riverNoise.SetFractalOctaves(4);
	m_riverNoise.SetFractalLacunarity(2.0f);
	m_riverNoise.SetFractalGain(0.5f);

	m_riverWarpNoise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	m_riverWarpNoise.SetFrequency(0.008f);

	m_noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
	m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
	m_noise.SetFrequency(0.005f);
	m_noise.SetFractalLacunarity(2.0f);
	m_noise.SetFractalGain(0.5f);
	m_noise.SetFractalOctaves(5);

	m_continentalWarpNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	m_continentalWarpNoise.SetFrequency(0.003f);
	m_continentalWarpNoise.SetFractalOctaves(2);

	Biome ocean{};
	ocean.name = "Ocean";
	ocean.baseHeight = 55;
	ocean.amplitude = 15;
	ocean.frequency = 1.0f;
	ocean.topBlock = &getBlock("sand");
	ocean.subsurfaceBlock = &getBlock("sand");
	ocean.stoneBlock = &getBlock("stone");
	ocean.subsurfaceDepth = 3;
	// Ocean is selected by continentalness, not temp/humidity
	ocean.minTemperature = 0.0f;
	ocean.maxTemperature = 1.0f;
	ocean.minHumidity = 0.0f;
	ocean.maxHumidity = 1.0f;
	m_biomes.emplace_back(ocean);

	Biome desert{};
	desert.name = "Desert";
	desert.baseHeight = 68;
	desert.amplitude = 12;
	desert.frequency = 0.8f;
	desert.topBlock = &getBlock("sand");
	desert.subsurfaceBlock = &getBlock("sand");
	desert.stoneBlock = &getBlock("stone");
	desert.subsurfaceDepth = 5;
	// Hot and dry
	desert.minTemperature = 0.7f;
	desert.maxTemperature = 1.0f;
	desert.minHumidity = 0.0f;
	desert.maxHumidity = 0.3f;
	m_biomes.emplace_back(desert);

	Biome plains{};
	plains.name = "Plains";
	plains.baseHeight = 70;
	plains.amplitude = 30;
	plains.frequency = 1.0f;
	plains.topBlock = &getBlock("grass");
	plains.subsurfaceBlock = &getBlock("dirt");
	plains.stoneBlock = &getBlock("stone");
	plains.subsurfaceDepth = 3;
	// Warm and moderately dry
	plains.minTemperature = 0.4f;
	plains.maxTemperature = 0.7f;
	plains.minHumidity = 0.0f;
	plains.maxHumidity = 0.4f;
	m_biomes.emplace_back(plains);

	Biome plainHills{};
	plainHills.name = "Plain_Hills";
	plainHills.baseHeight = 80;
	plainHills.amplitude = 60;
	plainHills.frequency = 1.2f;
	plainHills.topBlock = &getBlock("grass");
	plainHills.subsurfaceBlock = &getBlock("dirt");
	plainHills.stoneBlock = &getBlock("stone");
	plainHills.subsurfaceDepth = 3;
	// Warm and humid
	plainHills.minTemperature = 0.4f;
	plainHills.maxTemperature = 1.0f;
	plainHills.minHumidity = 0.4f;
	plainHills.maxHumidity = 1.0f;  // BUG FIX: should be plainHills
	m_biomes.emplace_back(plainHills);

	Biome mountains{};
	mountains.name = "Mountains";
	mountains.baseHeight = 110;
	mountains.amplitude = 70;
	mountains.frequency = 5.0f;
	mountains.topBlock = &getBlock("grass");
	mountains.subsurfaceBlock = &getBlock("dirt");
	mountains.stoneBlock = &getBlock("stone");
	mountains.subsurfaceDepth = 3;
	// Cold (any humidity)
	mountains.minTemperature = 0.0f;
	mountains.maxTemperature = 0.4f;
	mountains.minHumidity = 0.0f;
	mountains.maxHumidity = 1.0f;
	m_biomes.emplace_back(mountains);

	gtl::vector<Chunk*> tempChunkVector{};
	const Block& air = getBlock("air");
	for (uint32_t chunkPositionX = 0; chunkPositionX < 32; chunkPositionX++) {
		for (uint32_t chunkPositionY = 0; chunkPositionY < 6; chunkPositionY++) {
			for (uint32_t chunkPositionZ = 0; chunkPositionZ < 32; chunkPositionZ++) {

				int32_t worldChunkX = (int32_t)chunkPositionX - 16;
				int32_t worldChunkZ = (int32_t)chunkPositionZ - 16;

				glm::ivec3 regionPosition = calculateRegionPosition(worldChunkX, worldChunkZ);
				//glm::ivec3 regionPosition(
				//	floorDiv(worldChunkX, 4),
				//	0,
				//	floorDiv(worldChunkZ, 4)
				//);

				auto regionIt = m_regions.find(regionPosition);
				Region* regionPtr = nullptr;
				if (regionIt != m_regions.end()) {
					regionPtr = regionIt->second.get();
				}
				else {
					createRegion(regionPosition, regionPtr);
				}
				
				glm::i32vec3 chunkPosition(worldChunkX, chunkPositionY, worldChunkZ);

				glm::ivec3 chunkPositionForRegion = {
					floorMod(worldChunkX, 4),
					(int32_t)chunkPositionY,
					floorMod(worldChunkZ, 4),
				};

				Chunk::CreateInfo chunkCreateInfo{};
				chunkCreateInfo.airBlock = &air;
				chunkCreateInfo.chunkPosition = chunkPosition;
				chunkCreateInfo.neightborChunks = { nullptr, nullptr , nullptr , nullptr , nullptr , nullptr };
				chunkCreateInfo.region = regionPtr;
				chunkCreateInfo.regionLocalBase = glm::i32vec2(
					chunkPositionForRegion.x * Chunk::CHUNK_WIDTH,
					chunkPositionForRegion.z * Chunk::CHUNK_DEPTH
				);
				Chunk* chunk = new Chunk(chunkCreateInfo);

				m_chunks[chunkPosition] = chunk;


				
				regionPtr->chunks[chunkPositionForRegion.x + (chunkPositionForRegion.z * 4) + (chunkPositionForRegion.y * 4 * 4)].store(chunk, std::memory_order_release);// = chunk;
				tempChunkVector.emplace_back(chunk);
			}
		}
	}

	for (uint32_t chunkPositionX = 0; chunkPositionX < 32; chunkPositionX++) {
		for (uint32_t chunkPositionY = 0; chunkPositionY < 6; chunkPositionY++) {
			for (uint32_t chunkPositionZ = 0; chunkPositionZ < 32; chunkPositionZ++) {
				glm::i32vec3 chunkPosition(chunkPositionX - 16, chunkPositionY, chunkPositionZ - 16);
				for (uint8_t face = 0; face < 6; face++) {
					glm::i32vec3 neighborPosition = chunkPosition + glm::i32vec3(Chunk::DIRECTIONS[face]);
					auto chunkIt = m_chunks.find(neighborPosition);
					if (chunkIt != m_chunks.end()) {
						m_chunks[chunkPosition]->setNeighborChunk(face, chunkIt->second);
					}
				}
			}
		}
		
	}

	sortChunks(glm::vec3(0.0), tempChunkVector);

	constexpr uint32_t generationThreads = 3;
	constexpr uint32_t meshThreads = 4;

	for (uint32_t i = 0; i < generationThreads; i++) {
		m_chunkGenerationThreads.emplace_back(generateChunk, std::ref(*this));
	}

	for (uint32_t i = 0; i < meshThreads; i++) {
		m_chunkMeshGenerationThreads.emplace_back(generateChunkMesh, std::ref(*this));
	}

	for (Chunk* chunk : tempChunkVector) {
		chunk->tryAddReference();
	}
	m_chunkGenerationQueue.enqueue_bulk(tempChunkVector.data(), tempChunkVector.size());
}

World::~World() {
	cleanup();
}

World::World(World&& other) noexcept : 
	m_namesToBlocks{std::move(other.m_namesToBlocks)}
{

}
World& World::operator=(World&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	if (*this) {
		cleanup();
	}

	m_namesToBlocks = std::move(other.m_namesToBlocks);
	
	other.m_namesToBlocks.clear();

	return *this;
}

World::operator bool() const noexcept {
	return !m_namesToBlocks.empty();
}

void World::processChunkUploads(uint32_t vao) {
	std::deque<ChunkUploadData> localQueue{};
	{
		ChunkUploadData tempUploadData{};
		for (uint32_t i = 0; i < 4 && m_chunkUploadQueue.try_dequeue(tempUploadData); i++) {

			localQueue.push_back(std::move(tempUploadData));
		}
	}

	while (!localQueue.empty()) {;
		ChunkUploadData chunkUploadData = std::move(localQueue.back());
		localQueue.pop_back();

		if (chunkUploadData.solidVertices.empty() || chunkUploadData.chunk->isDying()) {
			if (chunkUploadData.chunk) {
				chunkUploadData.chunk->subReference();
			}
			continue;
		}
		
		chunkUploadData.chunk->uploadChunkMesh(chunkUploadData.solidVertices, chunkUploadData.transparentVertices);
		chunkUploadData.chunk->setIsMeshGenerated(true);
		chunkUploadData.chunk->subReference();
	}
}

void World::sortChunks(const glm::vec3& position, gtl::vector<Chunk*>& visibleChunks) {
	std::ranges::sort(
		visibleChunks, [&position](const Chunk* a, const Chunk* b) {
			return glm::length2(glm::vec3(a->getPosition()) - position) < glm::length2(glm::vec3(b->getPosition()) - position);
		});
}

Chunk* World::getChunk(glm::ivec3 chunkPosition) noexcept {
	auto it = m_chunks.find(chunkPosition);
	return it == m_chunks.end() ? nullptr : it->second;
}

void World::update(const glm::vec3 & playerPosition) noexcept {
	if (glm::any(glm::isnan(playerPosition)) ||
		glm::length2(playerPosition) > 1e12f) {
		return;
	}

	glm::i32vec3 chunkPosition = getChunkPosition(playerPosition);

	std::vector<Chunk*> stillInUse{};
	for (Chunk* chunk : m_deletionGraveyard) {
		chunk->incrementGraveyardAge();
		if (chunk->isInUse() || chunk->getGraveyardAge() < 3) {
			stillInUse.push_back(chunk);
		}
		else {
			delete chunk;
		}
	}
	m_deletionGraveyard = std::move(stillInUse);

	Chunk* heldChunk = nullptr;
	std::vector<Chunk*> chunksToRequeue{};
	while (m_chunkMeshGenerationHoldingQueue.try_dequeue(heldChunk)) {
		if (!heldChunk) {
			continue;
		}

		if (heldChunk->isDying()) {
			heldChunk->subReference();
			continue;
		}

		chunksToRequeue.emplace_back(heldChunk);
	}

	if (!chunksToRequeue.empty()) {
		m_chunkMeshGenerationQueue.enqueue_bulk(chunksToRequeue.data(), chunksToRequeue.size());
	}
	

	for (auto it = m_chunks.begin(); it != m_chunks.end();) {
		glm::i32vec3 pos = it->second->getPosition();
		int32_t dx = std::abs(pos.x - chunkPosition.x);
		int32_t dz = std::abs(pos.z - chunkPosition.z);

		if (dx > 16 + 2 || dz > 16 + 2) {
			Chunk* dying = it->second;
			it = m_chunks.erase(it);

			for (uint8_t face = 0; face < 6; face++) {
				Chunk* rawNeighbor = dying->getNeighbor(face);
				if (!rawNeighbor) {
					continue;
				}
				uint8_t oppositeFace = face ^ 1;
				rawNeighbor->clearNeighborIfMatching(oppositeFace, dying);
			}

			dying->markIsDying();

			dying->clearAllNeighbors();

			{
				glm::i32vec3 cp = dying->getPosition();

			//	glm::i32vec3 regionPos{ floorDiv(cp.x, 4), 0, floorDiv(cp.z, 4) };

				glm::i32vec3 regionPos = calculateRegionPosition(cp.x, cp.z);
				auto regionIt = m_regions.find(regionPos);
				if (regionIt != m_regions.end()) {
					Region* region = regionIt->second.get();
					int32_t localX = floorMod(cp.x, 4);
					int32_t localZ = floorMod(cp.z, 4);
					int32_t slot = localX + (localZ * 4) + (cp.y * 4 * 4);
					region->chunks[slot].store(nullptr, std::memory_order_release);// = nullptr;
				}
			}

			m_deletionGraveyard.push_back(dying);
		}
		else {
			++it;
		}
	}

	for (int32_t radius = 0; radius <= 16; radius++) {
		for (int32_t x = -radius; x <= radius; x++) {
			for (int32_t z = -radius; z <= radius; z++) {
				if (std::abs(x) != radius && std::abs(z) != radius) {
					continue;
				}

				for (int32_t y = 0; y < 6; y++) {
					glm::i32vec3 cp = {
						chunkPosition.x + x,
						y,
						chunkPosition.z + z
					};

					if (m_chunks.find(cp) != m_chunks.end()) continue;


					//glm::i32vec3 regionPos = {
					//	floorDiv(cp.x, 4),
					//	0,
					//	floorDiv(cp.z, 4)
					//};

					glm::i32vec3 regionPos = calculateRegionPosition(cp.x, cp.z);
					Region* regionPtr = nullptr;
					if (auto regionIterator = m_regions.find(regionPos); regionIterator != m_regions.end()) {
						regionPtr = regionIterator->second.get();
					}
					else {
						createRegion(regionPos, regionPtr);
					}


					glm::ivec3 chunkPosForRegion = {
						floorMod(cp.x, 4),
						cp.y,
						floorMod(cp.z, 4)
					};

					Chunk::CreateInfo chunkCreateInfo{};
					chunkCreateInfo.airBlock = &getBlock("air");
					chunkCreateInfo.chunkPosition = cp;
					chunkCreateInfo.neightborChunks = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
					chunkCreateInfo.region = regionPtr;
					chunkCreateInfo.regionLocalBase = glm::i32vec2(
						chunkPosForRegion.x * Chunk::CHUNK_WIDTH,
						chunkPosForRegion.z * Chunk::CHUNK_DEPTH
					);

					Chunk* chunk = new Chunk(chunkCreateInfo);
					m_chunks[cp] = chunk;

					regionPtr->chunks[
						chunkPosForRegion.x +
							(chunkPosForRegion.z * 4) +
							(chunkPosForRegion.y * 4 * 4)
					].store(chunk, std::memory_order_release);// = chunk;

					for (uint8_t face = 0; face < 6; face++) {
						glm::i32vec3 neighborPosition = cp + glm::i32vec3(Chunk::DIRECTIONS[face]);

						auto chunkIt = m_chunks.find(neighborPosition);
						if (chunkIt != m_chunks.end()) {
							Chunk* neighbor = chunkIt->second;
							if (neighbor && !neighbor->isDying()) {
								// Link new chunk to its existing neighbor
								chunk->setNeighborChunk(face, neighbor);

								// Link neighbor back to this new chunk
								uint8_t oppositeFace = face ^ 1;
								neighbor->setNeighborChunk(oppositeFace, chunk);

								// If the existing neighbor was already generated, notify the new chunk
								if (neighbor->isGenerated()) {
									chunk->incrementNeighborGenerated();
								}
							}
						}
					}

					if (chunk->tryAddReference()) {
						m_chunkGenerationQueue.enqueue(chunk);
					}
					else {
						delete chunk;
					}
				}
			}
		}
	}
}

void World::addChunkToRemeshingQueue(Chunk* chunk) noexcept {
	if (!chunk) {
		return;
	}

	if (chunk->isDying()) {
		return;
	}

	glm::i32vec3 centerPos = chunk->getPosition();
	std::vector<Chunk*> targetsToReset;
	targetsToReset.reserve(27);

	for (int32_t dx = -1; dx <= 1; dx++) {
		for (int32_t dy = -1; dy <= 1; dy++) {
			for (int32_t dz = -1; dz <= 1; dz++) {
				glm::i32vec3 targetPos = centerPos + glm::i32vec3(dx, dy, dz);
				Chunk* neighbor = getChunk(targetPos);
				if (neighbor) {
					targetsToReset.push_back(neighbor);
				}
			}
		}
	}

	for (Chunk* c : targetsToReset) {
		c->clearLight();
		c->setLightState(LightState::Unlit);
		c->setIsMeshGenerated(false);
	}
		
	chunk->setIsRemeshSource(true);

	for (Chunk* c : targetsToReset) {
		if (c->isDying()) {
			continue;
		}

		if (c->tryAddReference()) {
			enqueueMeshChunk(m_chunkMeshGenerationQueue, c);
		}
	}

	if (!chunk->isDying() && chunk->tryAddReference()) {
		enqueueMeshChunk(m_chunkMeshGenerationQueue, chunk);
	}

	//enqueueMeshChunks(m_chunkMeshGenerationQueue, targetsToReset);
	//enqueueMeshChunk(m_chunkMeshGenerationQueue, chunk);
}

float World::getSurfaceAt(float worldX, float worldZ) const noexcept {
	worldX = glm::floor(worldX);
	worldZ = glm::floor(worldZ);

	glm::i32vec3 chunkPosition = getChunkPosition(glm::vec3(worldX, 0.0f, worldZ));

	BiomeMap biomeMap = buildBiomeMap(chunkPosition);

	int32_t localX = floorMod((int32_t)worldX, Chunk::CHUNK_WIDTH);
	int32_t localZ = floorMod((int32_t)worldZ, Chunk::CHUNK_DEPTH);

	ColumnContext column = classifyColumn(biomeMap, (int32_t)worldX, (int32_t)worldZ, chunkPosition, localX, localZ);

	return (float)column.maxHeight + 1.0f;
}

// private

void World::cleanup() {
	m_isRunning.store(false, std::memory_order_release);

	m_namesToBlocks.clear();
}

void World::generateChunk(World& world) {
	const Block& airBlock = world.getBlock("air");
//	Block& grassBlock = world.getBlock("grass");
	//Block& stoneBlock = world.getBlock("stone");
//	Block& dirtBlock = world.getBlock("dirt");
	//Block& redWoolBlock = world.getBlock("red_wool");
	const Block& water = world.getBlock("water");
	const Block& sand = world.getBlock("sand");

	

	while (world.m_isRunning.load(std::memory_order_acquire)) {
		Chunk* chunk = nullptr;
		world.m_chunkGenerationQueue.wait_dequeue(chunk);
		//if (!world.m_chunkGenerationQueue.try_dequeue(chunk)) {
		//	std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//	continue;
		//}

		if (!chunk) [[unlikely]] {
			continue;
		}

		if (chunk->isDying()) {
			chunk->subReference();
			continue;
		}
		

		

		glm::i32vec3 chunkPosition = chunk->getPosition();


		BiomeMap biomeMap = world.buildBiomeMap(chunkPosition);

		

		Region* region = chunk->getRegion();
		int32_t localChunkX = floorMod(chunkPosition.x, 4);
		int32_t localChunkZ = floorMod(chunkPosition.z, 4);
		int32_t chunkBaseY = chunkPosition.y * Chunk::CHUNK_HEIGHT;

		for (uint32_t x = 0; x < Chunk::CHUNK_WIDTH; x++) {
			int32_t worldX = chunkPosition.x * Chunk::CHUNK_WIDTH + x;
			for (uint32_t z = 0; z < Chunk::CHUNK_DEPTH; z++) {
				int32_t worldZ = chunkPosition.z * Chunk::CHUNK_DEPTH + z;

				ColumnContext column = world.classifyColumn(biomeMap, worldX, worldZ, chunkPosition, x, z);

				int32_t highestPossibleY = std::max((int32_t)column.maxHeight, (int32_t)SEA_LEVEL);
				int32_t localHeighestY = highestPossibleY - chunkBaseY;

				for (int32_t y = std::max(0, localHeighestY); y < Chunk::CHUNK_HEIGHT; y++) {
					int32_t worldY = chunkBaseY + y;
					chunk->setBlockRaw(x, y, z, (worldY <= SEA_LEVEL) ? water : airBlock);
				}

				for (int32_t y = 0; y < std::min(localHeighestY, (int32_t)Chunk::CHUNK_HEIGHT); y++) {
					int32_t worldY = chunkBaseY + y;
					chunk->setBlockRaw(
						x, y, z,
						world.selectBlock(column, worldY, airBlock, water, sand)
					);
				}

				if (region) {
					int32_t regionX = localChunkX * Chunk::CHUNK_WIDTH + x;
					int32_t regionZ = localChunkZ * Chunk::CHUNK_DEPTH + z;
					uint32_t heighMapIndex = (uint32_t)regionZ + (regionX * (64 * 4));

					int32_t startScanY = std::min(localHeighestY, (int32_t)Chunk::CHUNK_HEIGHT - 1);

					for (int32_t y =startScanY; y >= 0; y--) {
						const Block* block = chunk->getBlock(x, y, z);
						if (block && block->getType() != Block::Type::Air) {
							uint16_t worldY = chunkBaseY + y;

							uint16_t currentHeight = region->heightmap[heighMapIndex].load(std::memory_order_relaxed);
							while (worldY > currentHeight) {
								if (region->heightmap[heighMapIndex].compare_exchange_weak(currentHeight, worldY)) {
									break;
								}
							}

							break;
						}
					}
				}
			}
		}

		chunk->setIsGenerated(true);

		for (uint8_t face = 0; face < 6; face++) {
			Chunk* neighbor = chunk->getNeighbor(face);
			if (!neighbor) {
				continue;
			}

			neighbor->incrementNeighborGenerated();

			if (neighbor->isGenerated() && neighbor->isMeshGenerated()) {
				neighbor->setIsMeshGenerated(false);
				if (neighbor->tryAddReference()) {
					enqueueMeshChunk(world.m_chunkMeshGenerationQueue, neighbor);
				}
			}
		}

		if (chunk->tryAddReference()) {
			enqueueMeshChunk(world.m_chunkMeshGenerationQueue, chunk);
		}

		chunk->subReference();
		//chunk->subReference();
		
	}
}

void World::generateChunkMesh(World& world) {
	while (world.m_isRunning.load(std::memory_order_acquire)) {
	
		Chunk* chunk = nullptr;
		world.m_chunkMeshGenerationQueue.wait_dequeue(chunk);
		//if (!world.m_chunkMeshGenerationQueue.try_dequeue(chunk)) {
		//	std::this_thread::sleep_for(std::chrono::milliseconds(100));
		//	continue;
		//}

		if (!chunk) [[unlikely]] {
			continue;
		}

		if (chunk->isDying()) [[unlikely]] {
			chunk->subReference();
			continue;
		}

		if (!chunk->isGenerated()) {
			chunk->subReference();
			continue;
		}

		if (!chunk->areNeighborsGenerated()) {
			world.m_chunkMeshGenerationHoldingQueue.enqueue(chunk);
		//	chunk->subReference();
			continue;
		}

		if (chunk->getLightState() == LightState::Unlit) {
			chunk->propagateLocalLight();
			chunk->setLightState(LightState::LocalLit);
		}

		if (chunk->getLightState() == LightState::LocalLit) {
			bool neighborsReadyForLighting = true;
			for (uint32_t i = 0; i < 6; i++) {
				Chunk* neighbor = chunk->getNeighbor(i);
				if (neighbor && neighbor->getLightState() == LightState::Unlit) {
					neighborsReadyForLighting = false;
					break;
				}
			}

			if (!neighborsReadyForLighting) {
				world.m_chunkMeshGenerationHoldingQueue.enqueue(chunk);
				//chunk->subReference();
				continue;
			}

			chunk->floodFillFromNeighbors();
			chunk->setLightState(LightState::FullyPropagated);

			
			for (uint32_t i = 0; i < 6; i++) {
				Chunk* neighbor = chunk->getNeighbor(i);
				if (neighbor && neighbor->getLightState() == LightState::LocalLit) {
					//Chunk* raw = neighbor.get();
					if (neighbor->tryAddReference()) {
						enqueueMeshChunk(world.m_chunkMeshGenerationQueue, neighbor);
					}
					
				}
			}
		}

		bool neighborsFullyLit = true;
		for (uint32_t i = 0; i < 6; i++) {
			Chunk* neighbor = chunk->getNeighbor(i);
			if (neighbor && neighbor->getLightState() != LightState::FullyPropagated) {
				neighborsFullyLit = false;
				break;
			}
		}

		if (!neighborsFullyLit) {
			world.m_chunkMeshGenerationHoldingQueue.enqueue(chunk);
		//	chunk->subReference();
			continue;
		}

		if (chunk->isRemeshSource()) {
			chunk->setIsRemeshSource(false);
			for (uint32_t face = 0; face < 6; face++) {
				Chunk* neighbor = chunk->getNeighbor(face);
				if (!neighbor) {
					continue;
				}

				neighbor->setIsMeshGenerated(false);
				if (neighbor->tryAddReference()) {
					enqueueMeshChunk(world.m_chunkMeshGenerationQueue, neighbor);
				}
			}
		}
		
		const auto& vertices = chunk->generateChunkMesh();

		if (vertices.solidVertices.empty()) {
			chunk->setIsMeshGenerated(true);
			chunk->subReference();
			continue;
		}
		if (chunk->tryAddReference()) {
			world.m_chunkUploadQueue.enqueue(ChunkUploadData{ std::move(vertices.solidVertices), std::move(vertices.transparentVertices), chunk });
		}

		chunk->subReference();
	}
} 

void World::createRegion(const glm::i32vec3 regionPosition, Region*& outRegion) {
	auto region = std::make_unique<Region>();
	region->position = regionPosition;
	
	region->bbMin = glm::vec3(regionPosition) * Chunk::CHUNK_SIZE * glm::vec3(4.0f, 6.0f, 4.0f);
	region->bbMax = region->bbMin + Chunk::CHUNK_SIZE * glm::vec3(4.0f, 6.0f, 4.0f);

	outRegion = region.get();
	m_regions[regionPosition] = std::move(region);
}

void World::updateHeightMapForChunk(Chunk* chunk, Region* region) {
	glm::i32vec3 chunkPosition = chunk->getPosition();

	int32_t localChunkX = floorMod(chunkPosition.x, 4);
	int32_t localChunkZ = floorMod(chunkPosition.z, 4);
	int32_t chunkBaseY = chunkPosition.y * Chunk::CHUNK_HEIGHT;

	for (int32_t x = 0; x < Chunk::CHUNK_WIDTH; x++) {
		int32_t regionX = localChunkX * Chunk::CHUNK_WIDTH + x;

		uint32_t regionXOffset = regionX * (64 * 4);
		for (int32_t z = 0; z < Chunk::CHUNK_DEPTH; z++) {
			int32_t regionZ = localChunkZ * Chunk::CHUNK_DEPTH + z;

			uint32_t heighMapIndex = (uint32_t)regionZ + regionXOffset;

			for (int32_t y = Chunk::CHUNK_HEIGHT - 1; y >= 0; y--) {
				const Block* block = chunk->getBlock(x, y, z);
				if (block && block->getType() != Block::Type::Air) {
					uint16_t worldY = chunkBaseY + y;

					uint16_t currentHeight = region->heightmap[heighMapIndex].load(std::memory_order_relaxed);
					while (worldY > currentHeight) {
						if (region->heightmap[heighMapIndex].compare_exchange_weak(currentHeight, worldY)) {
							break;
						}
					}

					break;
				}
			}
		}
	}
}

const World::Biome& World::getBiomeAt(int32_t worldX, int32_t worldZ) const {
	float value = m_biomeNoise.GetNoise((float)worldX, (float)worldZ);
	value = (value + 1.0f) * 0.5f;

	size_t index = (size_t)(value * m_biomes.size());
	index = std::clamp(index, 0uz, m_biomes.size() - 1uz);
	return m_biomes[index];
}

World::BiomeMap World::buildBiomeMap(glm::i32vec3 chunkPosition) const {
	BiomeMap map{};

	constexpr int32_t BiomeMapExtent2 = BiomeMap::EXTENT * BiomeMap::EXTENT;

	thread_local std::vector<float> xSamples(BiomeMapExtent2);
	thread_local std::vector<float> zSamples(BiomeMapExtent2);

	int32_t index = 0;
	for (int32_t biomeX = 0; biomeX < BiomeMap::EXTENT; biomeX++) {
		for (int32_t biomeZ = 0; biomeZ < BiomeMap::EXTENT; biomeZ++) {
			xSamples[index] = float(chunkPosition.x * Chunk::CHUNK_WIDTH - BiomeMap::MARGIN + biomeX * BiomeMap::STEP);
			zSamples[index] = float(chunkPosition.z * Chunk::CHUNK_DEPTH - BiomeMap::MARGIN + biomeZ * BiomeMap::STEP);
			index++;
		}
	}

	thread_local std::vector<float> terrainValues(BiomeMapExtent2);
	thread_local std::vector<float> continentalValues(BiomeMapExtent2);
	thread_local std::vector<float> errosionValues(BiomeMapExtent2);
	thread_local std::vector<float> ridgeValues(BiomeMapExtent2);
	thread_local std::vector<float> weirdnessValues(BiomeMapExtent2);
	thread_local std::vector<float> temperatureValues(BiomeMapExtent2);
	thread_local std::vector<float> humidityValues(BiomeMapExtent2);
	thread_local std::vector<float> riverValues(BiomeMapExtent2);
	thread_local std::vector<float> transitionValues(BiomeMapExtent2);

	constexpr float CONTINENTAL_WARP_STRENGTH = 50.0f;
	constexpr float RIVER_WARP_STRENGTH = 48.0f;

	for (int32_t i = 0; i < BiomeMapExtent2; i++) {
		terrainValues[i] = m_noise.GetNoise(xSamples[i], zSamples[i]);

		float warpX = m_continentalWarpNoise.GetNoise(xSamples[i] * 0.7f, zSamples[i] * 0.7f) * CONTINENTAL_WARP_STRENGTH;
		float warpZ = m_continentalWarpNoise.GetNoise(xSamples[i] * 0.7f + 137.0f, zSamples[i] * 0.7f + 249.0f) * CONTINENTAL_WARP_STRENGTH;
		continentalValues[i] = m_contentalnessNoise.GetNoise(xSamples[i] + warpX, zSamples[i] + warpZ);

		errosionValues[i] = m_errosionNoise.GetNoise(xSamples[i], zSamples[i]);

		float ridgeWarpX = m_riverWarpNoise.GetNoise(xSamples[i] * 0.5f, zSamples[i] * 0.5f) * 40.0f;
		float ridgeWarpZ = m_riverWarpNoise.GetNoise(xSamples[i] * 0.5f + 91.0f, zSamples[i] * 0.5f) * 40.0f;
		ridgeValues[i] = m_ridgedNoise.GetNoise(xSamples[i] + ridgeWarpX, zSamples[i] + ridgeWarpZ);

		weirdnessValues[i] = m_weirdnessNoise.GetNoise(xSamples[i], zSamples[i]);

		temperatureValues[i] = m_tempuratureNoise.GetNoise(xSamples[i] + 1117.0f, zSamples[i] - 523.0f);
		humidityValues[i] = m_humdityNoise.GetNoise(xSamples[i] - 821.0f, zSamples[i] + 391.0f);

		float riverWarpedX = m_riverWarpNoise.GetNoise(xSamples[i], zSamples[i] * 0.7f) * RIVER_WARP_STRENGTH;
		float riverWarpedZ = m_riverWarpNoise.GetNoise(xSamples[i] * 0.7f + 31.7f, zSamples[i] + 47.3f) * RIVER_WARP_STRENGTH;
		riverValues[i] = m_riverNoise.GetNoise(xSamples[i] + riverWarpedX, zSamples[i] + riverWarpedZ);

		transitionValues[i] = m_transitionNoise.GetNoise(xSamples[i], zSamples[i]);
	}

	index = 0;
	for (int32_t biomeX = 0; biomeX < BiomeMap::EXTENT; biomeX++) {
		for (int32_t biomeZ = 0; biomeZ < BiomeMap::EXTENT; biomeZ++) {
			float temperature = (temperatureValues[index] + 1.0f) * 0.5f;
			float humidity = (humidityValues[index] + 1.0f) * 0.5f;
			float continentalness = (continentalValues[index] + 1.0f) * 0.5f;
			float terrainNoise = (terrainValues[index] + 1.0f) * 0.5f;
			float transition = (transitionValues[index] + 1.0f) * 0.5f;

			const Biome* biome = selectBiomeFromParameters(temperature, humidity, continentalness);

			
			float oceanThreshold = 0.30f + transition * 0.15f;
			float rawOceanWeight = glm::smoothstep(oceanThreshold, oceanThreshold + 0.12f, continentalness);

			
			float rawMountainWeight = glm::smoothstep(0.55f, 0.35f, temperature);

			map.samples[biomeX * BiomeMap::EXTENT + biomeZ] = {
				.biome = biome,
				.rawValue = temperature,
				.terrainHeight = terrainNoise,
				.continentalBase = continentalness,
				.errosionBase = (errosionValues[index] + 1.0f) * 0.5f,
				.ridgeBase = (ridgeValues[index] + 1.0f) * 0.5f,
				.weirdnessBase = (weirdnessValues[index] + 1.0f) * 0.5f,

				.blendedOceanWeight = 0.0f,   
				.blendedMountainWeight = 0.0f, 

				.blendedRiverNoise = (riverValues[index] + 1.0f) * 0.5f,

				.temperatureBase = temperature,
				.humidityBase = humidity,

				.rawOceanWeight = rawOceanWeight,
				.rawMountainWeight = rawMountainWeight,
			};
			index++;
		}
	}

	constexpr int32_t KERNEL_RADIUS = 10;
	constexpr int32_t KERNEL_SIZE = KERNEL_RADIUS * 2 + 1;
	static constexpr const auto kernelWeights = []() {
		std::array<std::array<float, KERNEL_SIZE>, KERNEL_SIZE> weights{};
		float totalWeight = 0.0f;
		for (int32_t kernelX = -KERNEL_RADIUS; kernelX <= KERNEL_RADIUS; kernelX++) {
			for (int32_t kernelZ = -KERNEL_RADIUS; kernelZ <= KERNEL_RADIUS; kernelZ++) {
				float d2 = float(kernelX * kernelX + kernelZ * kernelZ);
				weights[kernelX + KERNEL_RADIUS][kernelZ + KERNEL_RADIUS] = constexpr_exp(-d2 / float(KERNEL_RADIUS * KERNEL_RADIUS) * 3.0f);
				totalWeight += weights[kernelX + KERNEL_RADIUS][kernelZ + KERNEL_RADIUS];
			}
		}

		for (int32_t kernelX = 0; kernelX < KERNEL_SIZE; kernelX++) {
			for (int32_t kernelZ = 0; kernelZ < KERNEL_SIZE; kernelZ++) {
				weights[kernelX][kernelZ] /= totalWeight;
			}
		}
		return weights;
		}();

	// Blur pass 1: X direction
	for (int32_t biomeX = 0; biomeX < BiomeMap::EXTENT; biomeX++) {
		for (int32_t biomeZ = 0; biomeZ < BiomeMap::EXTENT; biomeZ++) {
			float totalBase = 0.0f;
			float totalAmplifier = 0.0f;
			float totalContinentalness = 0.0f;
			float totalErosion = 0.0f;
			float totalRidge = 0.0f;
			float totalWeirdness = 0.0f;
			float totalRiver = 0.0f;
			float totalTemperature = 0.0f;
			float totalHumidity = 0.0f;
			float totalOceanWeight = 0.0f;     
			float totalMountainWeight = 0.0f;  
			//float totalWeight = 0.0f;

			for (int32_t kernelX = -KERNEL_RADIUS; kernelX <= KERNEL_RADIUS; kernelX++) {
				for (int32_t kernelZ = -KERNEL_RADIUS; kernelZ <= KERNEL_RADIUS; kernelZ++) {
					float weight = kernelWeights[kernelX + KERNEL_RADIUS][kernelZ + KERNEL_RADIUS];
					const BiomeSample& sample = map.get(biomeX + kernelX, biomeZ + kernelZ);

					totalBase += sample.biome->baseHeight * weight;
					totalAmplifier += sample.biome->amplitude * weight;
					totalContinentalness += sample.continentalBase * weight;
					totalErosion += sample.errosionBase * weight;
					totalRidge += sample.ridgeBase * weight;
					totalWeirdness += sample.weirdnessBase * weight;
					totalRiver += sample.blendedRiverNoise * weight;
					totalTemperature += sample.temperatureBase * weight;
					totalHumidity += sample.humidityBase * weight;
					totalOceanWeight += sample.rawOceanWeight * weight;
					totalMountainWeight += sample.rawMountainWeight * weight;
					//totalWeight += weight;
				}
				
			}

			int32_t sampleIndex = biomeX * BiomeMap::EXTENT + biomeZ;
			auto& sample = map.samples[sampleIndex];

			sample.blendedBase = totalBase;
			sample.blendedAmpplifier = totalAmplifier;
			sample.blendedContinentalness = totalContinentalness;
			sample.blendedErrosion = totalErosion;
			sample.blendedRidge = totalRidge;
			sample.blendedWeirdness = totalWeirdness;
			sample.blendedRiverNoise = totalRiver;
			sample.blendedTemperature = totalTemperature;
			sample.blendedHumidity = totalHumidity;

			sample.blendedOceanWeight = totalOceanWeight;
			sample.blendedMountainWeight = totalMountainWeight; 
		}
	}

	return map;
}
float bilinearInterpolation(
	float sample00, float weight00, float sample01, float weight01,
	float sample10, float weight10, float sample11, float weight11) noexcept {

	return (sample00) * weight00 + (sample10 * weight10) + (sample01 * weight01) + (sample11 * weight11);
}

World::ColumnContext World::classifyColumn(const BiomeMap& biomeMap, int32_t worldX, int32_t worldZ, glm::i32vec3 chunkPosition, int32_t localX, int32_t localZ) const noexcept {
	constexpr float chunkStartInSamples = (float)BiomeMap::MARGIN / (float)BiomeMap::STEP;
	float chunkX = chunkStartInSamples + ((float)localX / (float)BiomeMap::STEP);
	float chunkZ = chunkStartInSamples + ((float)localZ / (float)BiomeMap::STEP);
	int32_t chunkX0 = (int32_t)chunkX;
	int32_t chunkZ0 = (int32_t)chunkZ;
	float factorX = chunkX - chunkX0;
	float factorZ = chunkZ - chunkZ0;

	const BiomeSample& s00 = biomeMap.get(chunkX0, chunkZ0);
	const BiomeSample& s10 = biomeMap.get(chunkX0 + 1, chunkZ0);
	const BiomeSample& s01 = biomeMap.get(chunkX0, chunkZ0 + 1);
	const BiomeSample& s11 = biomeMap.get(chunkX0 + 1, chunkZ0 + 1);

	float w00 = (1 - factorX) * (1 - factorZ);
	float w10 = factorX * (1 - factorZ);
	float w01 = (1 - factorX) * factorZ;
	float w11 = factorX * factorZ;

	float oceanWeight = s00.blendedOceanWeight * w00
		+ s10.blendedOceanWeight * w10
		+ s01.blendedOceanWeight * w01
		+ s11.blendedOceanWeight * w11;

	float mountainWeight = s00.blendedMountainWeight * w00
		+ s10.blendedMountainWeight * w10
		+ s01.blendedMountainWeight * w01
		+ s11.blendedMountainWeight * w11;

	float blendedBase = s00.blendedBase * w00
		+ s10.blendedBase * w10
		+ s01.blendedBase * w01
		+ s11.blendedBase * w11;

	float blendedAmplifier = s00.blendedAmpplifier * w00
		+ s10.blendedAmpplifier * w10
		+ s01.blendedAmpplifier * w01
		+ s11.blendedAmpplifier * w11;

	float terrainNoise = s00.terrainHeight * w00
		+ s10.terrainHeight * w10
		+ s01.terrainHeight * w01
		+ s11.terrainHeight * w11;

	float continentalness = s00.blendedContinentalness * w00
		+ s10.blendedContinentalness * w10
		+ s01.blendedContinentalness * w01
		+ s11.blendedContinentalness * w11;

	float smoothContinental = glm::smoothstep(0.15f, 0.9f, continentalness);
	float continentalBase = glm::mix(30.0f, 90.0f, smoothContinental);
	continentalBase = glm::mix(continentalBase, 45.0f, oceanWeight);
	float continentalBlendFactor = glm::smoothstep(0.3f, 0.6f, smoothContinental);

	float finalBase = glm::mix(blendedBase, continentalBase, continentalBlendFactor);

	float errosion = s00.blendedErrosion * w00
		+ s10.blendedErrosion * w10
		+ s01.blendedErrosion * w01
		+ s11.blendedErrosion * w11;

	float mountainProtection = glm::smoothstep(0.1f, 0.5f, mountainWeight);

	float erodedAmplifier = blendedAmplifier * glm::mix(glm::mix(1.0f, 0.15f, errosion), 1.0f, mountainProtection);
	float oceanFlattening = glm::smoothstep(0.0f, 0.6f, oceanWeight) * (1.0f - mountainProtection);
	erodedAmplifier *= glm::mix(1.0f, 0.2f, oceanFlattening);

	float ridgedNoise = s00.blendedRidge * w00
		+ s10.blendedRidge * w10
		+ s01.blendedRidge * w01
		+ s11.blendedRidge * w11;

	float ridgeWeight = glm::clamp(mountainProtection - errosion * 0.5f, 0.0f, 1.0f);
	float detailNoise = glm::mix(terrainNoise, ridgedNoise, ridgeWeight);

	float weirdness = s00.blendedWeirdness * w00
		+ s10.blendedWeirdness * w10
		+ s01.blendedWeirdness * w01
		+ s11.blendedWeirdness * w11;

	float signedWeirdness = weirdness * 2.0f - 1.0f;
	float peaksAndValleys = 1.0f - std::abs(std::abs(signedWeirdness) * 3.0f - 2.0f);
	peaksAndValleys = glm::clamp(peaksAndValleys, -1.0f, 1.0f);

	float terrainEnergy = glm::mix(1.0f, 0.35f, errosion);
	float peaksAndValleysAmplitude = glm::mix(10.0f, 45.0f, mountainProtection);
	float peaksAndValleyContribution = peaksAndValleys * peaksAndValleysAmplitude * terrainEnergy * (1.0f - oceanWeight);

	float maxHeightFactor = finalBase
		+ detailNoise * erodedAmplifier - (erodedAmplifier * 0.5f)
		+ peaksAndValleyContribution;

	float temperature = s00.blendedTemperature * w00
		+ s10.blendedTemperature * w10
		+ s01.blendedTemperature * w01
		+ s11.blendedTemperature * w11;

	float humidity = s00.blendedHumidity * w00
		+ s10.blendedHumidity * w10
		+ s01.blendedHumidity * w01
		+ s11.blendedHumidity * w11;

	float heightBasedTemperature = glm::clamp(temperature - glm::max(0.0f, (maxHeightFactor - 80.0f) * 0.0035f), 0.0f, 1.0f);
	heightBasedTemperature = glm::mix(heightBasedTemperature, heightBasedTemperature * 0.85f, mountainWeight);

	const Biome* biome = selectBiomeFromParameters(temperature, humidity, continentalness);

	if (oceanWeight > 0.0f) {
		float oceanFloor = glm::mix(maxHeightFactor, float(SEA_LEVEL - 8), oceanWeight);
		maxHeightFactor = glm::mix(maxHeightFactor, oceanFloor, glm::smoothstep(0.0f, 1.0f, oceanWeight));
	}

	float riverSample = s00.blendedRiverNoise * w00
		+ s10.blendedRiverNoise * w10
		+ s01.blendedRiverNoise * w01
		+ s11.blendedRiverNoise * w11;

	float ridge = 1.0f - std::abs(riverSample);
	ridge = ridge * ridge;

	constexpr float RIVER_THRESHOLD = 0.7f;
	constexpr float RIVER_DEPTH = 5.0f;
	float riverBlend = 0.0f;
	bool isRiver = false;

	float riverSuppression = glm::smoothstep(0.05f, 0.35f, oceanWeight) + glm::smoothstep(0.05f, 0.4f, mountainWeight);
	riverSuppression = glm::clamp(riverSuppression, 0.0f, 1.0f);

	if (ridge > RIVER_THRESHOLD && riverSuppression < 0.5f) {
		float rawBlend = (ridge - RIVER_THRESHOLD) / (1.0f - RIVER_THRESHOLD);
		rawBlend = rawBlend * rawBlend;

		riverBlend = rawBlend * (1.0f - riverSuppression * 2.0f);
		riverBlend = glm::clamp(riverBlend, 0.0f, 1.0f);

		float riverFloor = float(SEA_LEVEL - RIVER_DEPTH);
		float carvedHeight = maxHeightFactor + riverBlend * (riverFloor - maxHeightFactor);
		if (carvedHeight < maxHeightFactor) {
			maxHeightFactor = carvedHeight;
			isRiver = maxHeightFactor <= float(SEA_LEVEL + 4);
		}
	}

	float shoreBlend = glm::smoothstep(0.05f, 0.45f, oceanWeight);

	if (shoreBlend > 0.0f && mountainWeight > 0.0f) {
		float overlapStrength = shoreBlend * glm::smoothstep(0.0f, 0.5f, mountainWeight);
		float coastalTarget = glm::mix(float(SEA_LEVEL - 2), float(SEA_LEVEL + 20), 1.0f - shoreBlend);

		float blendStrength = glm::smoothstep(0.0f, 1.0f, overlapStrength);
		maxHeightFactor = glm::mix(maxHeightFactor, coastalTarget, blendStrength * 0.85f);
	}

	bool isShore = shoreBlend > 0.0f
		&& maxHeightFactor > float(SEA_LEVEL - 4)
		&& maxHeightFactor < float(SEA_LEVEL + 6)
		//&& mountainWeight < 0.05f
		&& !isRiver;

	bool isBeach = isShore && (maxHeightFactor >= float(SEA_LEVEL) * 2.0f * shoreBlend);

	return {
		.biome = biome,
		.maxHeight = (uint32_t)std::max(0.0f, maxHeightFactor),
		.isBeach = isBeach,
		.isRiver = isRiver,
		.riverBlend = riverBlend,
		.oceanWeight = oceanWeight,
		.mountainWeight = mountainWeight,
		.temperature = heightBasedTemperature,
		.humidity = humidity
	};
}

const Block& World::selectBlock(
	const ColumnContext& column,
	int32_t worldY,
	const Block& air,
	const Block& water,
	const Block& sand
) const {

	if (worldY >= (int32_t)column.maxHeight) {
		return (worldY <= SEA_LEVEL) ? water : air;
	}
	
	int32_t depth = (int32_t)column.maxHeight - worldY;

	bool useSand = column.isBeach
		|| (!column.isRiver
			//&& column.mountainWeight < 0.1f
			&& (int32_t)column.maxHeight <= SEA_LEVEL)
		|| (column.oceanWeight > 0.2f
			//&& column.mountainWeight < 0.1f 
			&& (int32_t)column.maxHeight <= SEA_LEVEL + 2);
	if (useSand) {
		int32_t sandDepth = column.biome->subsurfaceDepth;
		if (depth <= sandDepth) {
			return sand;
		}

		return *column.biome->stoneBlock;
	}

	if (column.isRiver) {
		if (depth == 1) {
			return sand;
		}

		if (depth <= column.biome->subsurfaceDepth + 2) {
			return *column.biome->subsurfaceBlock;
		}

		return *column.biome->stoneBlock;
	}

	if (depth == 1) {
		return *column.biome->topBlock;
	}

	if (depth <= column.biome->subsurfaceDepth) {
		return *column.biome->subsurfaceBlock;
	}

	return *column.biome->stoneBlock;
}

const World::Biome* World::selectBiomeFromParameters(
	float temperature,
	float humidity,
	float continentalness
) const noexcept {

	if (continentalness < 0.2f) return &m_biomes[0]; 

	const Biome* best = nullptr;
	float bestDist = std::numeric_limits<float>::max();

	for (size_t i = 1; i < m_biomes.size(); i++) {
		const Biome& b = m_biomes[i];
		float midT = (b.minTemperature + b.maxTemperature) * 0.5f;
		float midH = (b.minHumidity + b.maxHumidity) * 0.5f;
		float dt = temperature - midT;
		float dh = humidity - midH;
		float dist = dt * dt + dh * dh;
		if (dist < bestDist) {
			bestDist = dist;
			best = &b;
		}
	}
	return best;
}