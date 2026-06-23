#include <game/world/Chunk.hpp>

#include <glad/glad.h>

#include <game/world/world.hpp>

#include <game/world/Region.hpp>

Chunk::Chunk(const Chunk::CreateInfo& createInfo) {
	uint8_t id = m_blockIdCounter;
	m_blockIdCounter++;
	m_blockToIdLookup[createInfo.airBlock] = id;
	m_localBlocks[id] = createInfo.airBlock;

	m_blocks.fill(id);
	m_localBlockRefCount[id] = static_cast<uint32_t>(m_blocks.size());
	m_transparenceyMask.fill(std::numeric_limits<uint64_t>::max());
	m_chunkPosition = createInfo.chunkPosition;
	m_transformMatrix = glm::translate(glm::mat4{ 1.0f }, glm::vec3(m_chunkPosition) * CHUNK_SIZE);

	m_chunkBoundingBoxMin = glm::vec3(m_chunkPosition) * CHUNK_SIZE;
	m_chunkBoundingBoxMax = m_chunkBoundingBoxMin + CHUNK_SIZE;

	m_region = createInfo.region;// .store(createInfo.region, std::memory_order_release);
	m_regionLocalBase = createInfo.regionLocalBase;

	glCreateBuffers(1, &m_meshDataId);
	glCreateBuffers(1, &m_transparentMeshData);

	
	glCreateBuffers(1, &m_drawCommandId);
	glCreateBuffers(1, &m_transparentDrawCommandId);
}

Chunk::~Chunk() {
	m_region = nullptr;// .store(nullptr, std::memory_order_seq_cst);

	clearAllNeighbors();


	if (m_meshDataId != INVALID_BUFFER_ID) {
		glDeleteBuffers(1, &m_meshDataId);
		m_meshDataId = INVALID_BUFFER_ID;
	}

	if (m_drawCommandId != INVALID_BUFFER_ID) {
		glDeleteBuffers(1, &m_drawCommandId);
		m_drawCommandId = INVALID_BUFFER_ID;
	}

	if (m_transparentMeshData != INVALID_BUFFER_ID) {
		glDeleteBuffers(1, &m_transparentMeshData);
		m_transparentMeshData = INVALID_BUFFER_ID;
	}

	if (m_transparentDrawCommandId != INVALID_BUFFER_ID) {
		glDeleteBuffers(1, &m_transparentDrawCommandId);
		m_transparentDrawCommandId = INVALID_BUFFER_ID;
	}
}

void Chunk::setBlock(uint8_t x, uint8_t y, uint8_t z, const Block& block) noexcept {
	const uint32_t maskIndex = y + (x * CHUNK_WIDTH);
	if (block.isTransparent()) {
		m_transparenceyMask[maskIndex] |= 1ull << z;
	}
	else {
		m_transparenceyMask[maskIndex] &= ~(1ull << z);
	}

	uint32_t index = calculateBlockIndex(x, y, z);
	const uint8_t oldId = m_blocks[index];

	const Block* oldBlock = m_localBlocks[oldId];

	uint8_t id = 0;
	auto blockIt = m_blockToIdLookup.find(&block);
	if (blockIt == m_blockToIdLookup.end()) {
		if (!m_availableIds.empty()) {
			id = m_availableIds.front();
			m_availableIds.pop();
		}
		else {
			id = m_blockIdCounter++;
		}

		m_blockToIdLookup[&block] = id;

		auto& textureCache = m_localBlockIdToTextures[id];
		for (size_t i = 0; i < 6; i++) {
			textureCache[i] = block[i];
		}
	}
	else {
		id = blockIt->second;
	}

	if (id != oldId) {
		m_localBlockRefCount[id]++;

		if (oldId != id && --m_localBlockRefCount[oldId] == 0) {
			m_blockToIdLookup.erase(oldBlock);
			m_localBlockIdToTextures[oldId] = {};
			m_availableIds.push(oldId);
		}

		m_blocks[index] = id;
		m_localBlocks[id] = &block;
	}

	if (m_isGenerated.load(std::memory_order_acquire)) {
		onBlockChange(x, y, z, oldBlock, &block);
	}
}

void Chunk::setBlockRaw(uint8_t x, uint8_t y, uint8_t z, const Block& block) noexcept {
	const uint32_t maskIndex = y + (x * CHUNK_WIDTH);
	if (block.isTransparent()) {
		m_transparenceyMask[maskIndex] |= 1ull << z;
	}
	else {
		m_transparenceyMask[maskIndex] &= ~(1ull << z);
	}

	uint32_t index = calculateBlockIndex(x, y, z);
	uint8_t id = 0;
	auto blockIt = m_blockToIdLookup.find(&block);
	if (blockIt == m_blockToIdLookup.end()) {
		id = m_blockIdCounter++;

		m_localBlocks[id] = &block;
		m_blockToIdLookup[&block] = id;

		auto& textureCache = m_localBlockIdToTextures[id];
		for (size_t i = 0; i < 6; i++) {
			textureCache[i] = block[i];
		}
	}
	else {
		id = blockIt->second;
	}

	m_blocks[index] = id;
}

VertexInfo Chunk::generateChunkMesh() noexcept {
	bool anyNonAirFound = false;
	for (uint8_t id : m_blocks) {
		if (!m_localBlocks[id]->isAir()) {
			anyNonAirFound = true;
			break;
		}
	}

	if (!anyNonAirFound) {
		return {};
	}

	static constexpr const size_t SOLID_VERTEX_RESERVE = 2 << 12;
	static constexpr const size_t TRANSPARENT_VERTEX_RESERVE = 2 << 10;

	static thread_local std::vector<VertexData> tempMesh{};
	static thread_local std::vector<VertexData> tempTransparentMesh{};

	tempMesh.clear();
	tempTransparentMesh.clear();

	if (tempMesh.capacity() < SOLID_VERTEX_RESERVE) {
		tempMesh.reserve(SOLID_VERTEX_RESERVE);
	}
	if (tempTransparentMesh.capacity() < TRANSPARENT_VERTEX_RESERVE) {
		tempTransparentMesh.reserve(TRANSPARENT_VERTEX_RESERVE);
	}
	//tempTransparentMesh.reserve(2 << 10);

	for (uint8_t x = 0; x < CHUNK_WIDTH; x++) {
		uint32_t xOffset = x * CHUNK_AREA;
		const bool onNegativeX = (x == 0);
		const bool onPositiveX = (x == CHUNK_END.x);

		const uint8_t xEdgeMask = (onNegativeX << 2) | (onPositiveX << 3);

		const uint32_t shiftedXPosition = uint32_t(x) << 26;

		for (uint8_t y = 0; y < CHUNK_HEIGHT; y++) {
			const uint32_t xyIndex = (y * CHUNK_WIDTH) + xOffset;
			const bool onNegativeY = (y == 0);
			const bool onPositiveY = (y == CHUNK_END.y);

			const uint8_t xyEdgeMask = xEdgeMask | (onPositiveY << 4) | (onNegativeY << 5);

			const uint32_t shiftedXYPosition = shiftedXPosition | uint32_t(y) << 20;

			for (uint8_t z = 0; z < CHUNK_DEPTH; z++) {
				uint32_t currentIndex = z + xyIndex;
				const uint8_t blockId = m_blocks[currentIndex];
				const Block* block = m_localBlocks[blockId];
				const std::array<uint16_t, 6>& localTextureCache = m_localBlockIdToTextures[blockId];

				if (block->isAir()) {
					continue;
				}
				
				const bool onNegativeZ = (z == 0);
				const bool onPositiveZ = (z == CHUNK_END.z);

				const uint8_t xyzEdgeMask = xyEdgeMask | onNegativeZ | (onPositiveZ << 1);

				uint32_t positionalFaceData = shiftedXYPosition | (uint32_t(z) << 14);

				bool isReducedHeight = false;
				if (block->isWater()) {
					Chunk* aboveChunk = onPositiveY ? m_neightborChunks[4] : this;
					if (aboveChunk) {
						uint32_t aboveIndex = calculateBlockIndex(x, onPositiveY ? 0 : y + 1, z);
						uint8_t aboveId = aboveChunk->m_blocks[aboveIndex];
						if (!aboveChunk->m_localBlocks[aboveId]->isWater()) {
							isReducedHeight = true;
						}
					}
				}

				uint8_t selfLight = getLightAt(currentIndex);

				if (block->isTransparent()) {
					for (uint8_t face = 0; face < 6; face++) {
						const glm::i8vec3 neighborPosition = glm::i8vec3(x, y, z) + DIRECTIONS[face];
						const glm::i8vec3 neighborChunkPosition = neighborPosition & CHUNK_END;

						Chunk* currentChunk = ((xyzEdgeMask >> face) & 1) ? m_neightborChunks[face] : this;

						if (!currentChunk) {
							continue;
						}

						uint32_t neighborIndex = calculateBlockIndex(neighborChunkPosition);

						const uint8_t neighborBlockId = currentChunk->m_blocks[neighborIndex];
						if (currentChunk == this) {
							if (neighborBlockId == blockId) {
								continue;
							}
						}
						else {
							if (currentChunk->m_localBlocks[neighborBlockId] == block) {
								continue;
							}
						}
						

						uint8_t neighborLight = currentChunk->getLightAt(neighborIndex);
						uint8_t sunlight = std::max(selfLight, neighborLight);

						const uint32_t faceData =
							positionalFaceData | (uint32_t(face) << 11) | uint32_t(localTextureCache[face]) << 1 | (uint32_t)(isReducedHeight ? 1 : 0);

						uint32_t lightData = 0 << 24 | 0 << 16 | 0 << 8 | 0 << 4 | (sunlight & 0xF) << 0;

						tempTransparentMesh.emplace_back(faceData, lightData);
					}
				}
				else {
					for (uint8_t face = 0; face < 6; face++) {
						const glm::i8vec3 neighborPosition = glm::i8vec3(x, y, z) + DIRECTIONS[face];
						const glm::i8vec3 neighborChunkPosition = neighborPosition & CHUNK_END;

						Chunk* currentChunk = ((xyzEdgeMask >> face) & 1) ? m_neightborChunks[face] : this;

						if (!currentChunk) {
							continue;
						}

						bool isNeighborTransparent = currentChunk->isBlockTransparent(neighborChunkPosition.x, neighborChunkPosition.y, neighborChunkPosition.z);

						if (!isNeighborTransparent) {
							continue;
						}

						uint32_t neighborIndex = calculateBlockIndex(neighborChunkPosition);

						uint8_t neighborLight = currentChunk->getLightAt(neighborIndex);
						uint8_t sunlight = std::max(selfLight, neighborLight);

						const uint32_t faceData =
							positionalFaceData | (uint32_t(face) << 11) | uint32_t(localTextureCache[face]) << 1;

						uint32_t lightData = 0 << 24 | 0 << 16 | 0 << 8 | 0 << 4 | (sunlight & 0xF) << 0;

						tempMesh.emplace_back(faceData, lightData);
					}
				}
			}
		}
	}

	return VertexInfo{ tempMesh, tempTransparentMesh };
}

void Chunk::uploadChunkMesh(const std::vector<VertexData>& solidVertices, const std::vector<VertexData>& transparentVertices) noexcept {
	glNamedBufferData(
		m_meshDataId,
		sizeof(solidVertices[0]) * solidVertices.size(),
		solidVertices.data(),
		GL_STATIC_DRAW
	);

	glNamedBufferData(
		m_transparentMeshData,
		sizeof(transparentVertices[0]) * transparentVertices.size(),
		transparentVertices.data(),
		GL_STATIC_DRAW
	);

	m_drawCount = solidVertices.size();
	m_transparentDrawCount = transparentVertices.size();

	DrawArraysIndirectCommand command{
		.count = 6,
		.instanceCount = m_drawCount,
		.first = 0,
		.baseInstance = 0
	};

	glNamedBufferData(
		m_drawCommandId,
		sizeof(DrawArraysIndirectCommand),
		&command,
		GL_STATIC_DRAW
	);

	DrawArraysIndirectCommand transparentCommand{
		.count = 6,
		.instanceCount = m_transparentDrawCount,
		.first = 0,
		.baseInstance = 0
	};

	glNamedBufferData(
		m_transparentDrawCommandId, 
		sizeof(transparentCommand),
		&transparentCommand,
		GL_STATIC_DRAW
	);
}

void Chunk::propagateLocalLight() noexcept {
	m_sunlightStorage.fill(0);
	m_lightSeeds.clear();

	if (!m_region) {
		for (uint32_t i = 0; i < CHUNK_VOLUME; i++) {
			setLightAt(i, 15);
		}

		m_lightState.store(LightState::LocalLit, std::memory_order_release);
		return;
	}

	constexpr int32_t REGION_BLOCK_WIDTH = 4 * 64;

	for (int32_t x = 0; x < (int32_t)CHUNK_WIDTH; x++) {
		int32_t regionX = m_regionLocalBase.x + x;

		for (int32_t z = 0; z < (int32_t)CHUNK_DEPTH; z++) {
			int32_t regionZ = m_regionLocalBase.y + z;
			int32_t heightMapIndex = regionZ + (regionX * REGION_BLOCK_WIDTH);

			uint16_t surfaceHeight = m_region->heightmap[heightMapIndex].load(std::memory_order_relaxed);
			for (int32_t y = (int32_t)CHUNK_HEIGHT - 1; y >= 0; y--) {
				int32_t worldY = m_chunkPosition.y * CHUNK_HEIGHT + y;
				if (worldY <= surfaceHeight) {
					break;
				}

				if (!isBlockTransparent(x, y, z)) {
					break;
				}

				uint32_t index = calculateBlockIndex(x, y, z);

				uint8_t level = (worldY > surfaceHeight) ? 15 : 0;
				if (getLightAt(index) >= level) {
					continue;
				}

				setLightAt(index, level);
				if (level != 15) {
					continue;
				}

				bool hasUnlitNeighbor = false;
				for (uint8_t face = 0; face < 6; face++) {
					if (face == 4) {
						continue;
					}

					glm::i8vec3 neighborPos = glm::i8vec3(x, y, z) + DIRECTIONS[face];

					if (!isBlockInBounds(neighborPos)) {
						hasUnlitNeighbor = true;
						break;
					}

					int32_t neighborWorldY = m_chunkPosition.y * CHUNK_HEIGHT + neighborPos.y;
					int32_t neighborRegionX = m_regionLocalBase.x + neighborPos.x;
					int32_t neighborRegionZ = m_regionLocalBase.y + neighborPos.z;
					int32_t neighborHMIndex = neighborRegionZ + (neighborRegionX * REGION_BLOCK_WIDTH);
					uint16_t neighborSurface = m_region->heightmap[neighborHMIndex].load(std::memory_order_relaxed);

					if (isBlockTransparent(neighborPos.x, neighborPos.y, neighborPos.z)
						&& neighborWorldY <= neighborSurface) {
						hasUnlitNeighbor = true;
						break;
					}
				}

				if (hasUnlitNeighbor) {
					m_lightSeeds.emplace_back(
						((uint32_t)x << 26) | ((uint32_t)y << 20) |
						((uint32_t)z << 14) | level
					);
				}
			}
		}
	}
	
	m_lightState.store(LightState::LocalLit, std::memory_order_release);
}

void Chunk::floodFillFromNeighbors() noexcept {
	struct LightNode {
		Chunk* chunk;
		uint32_t packed; // x << 26 | y << 20 | z << 14 | level
	};

	static thread_local std::vector<LightNode> queue;
	static constexpr const size_t QUEUE_RESERVCE_SIZE = 2 << 12;
	queue.clear();
	if (queue.capacity() < QUEUE_RESERVCE_SIZE) {
		queue.reserve(QUEUE_RESERVCE_SIZE);
	}
//	queue.reserve(2 << 12);
	size_t head = 0;

	auto trySeed = [&](Chunk* targetChunk, int32_t x, int32_t y, int32_t z, uint8_t level)-> void {
		if (!targetChunk || level == 0) {
			return;
		}

		if (!targetChunk->isBlockTransparent(x, y, z)) {
			return;
		}

		uint32_t index = targetChunk->calculateBlockIndex(x, y, z);

		if (targetChunk->getLightAt(index) < level) {
			targetChunk->setLightAt(index, level);
			uint32_t packed = ((uint32_t)x << 26) | ((uint32_t)y << 20) | ((uint32_t)z << 14) | (level & 0xF);
			queue.emplace_back(targetChunk, packed);
		}
	};

	for (uint32_t packed : m_lightSeeds) {
		queue.emplace_back(this, packed);
	}
	m_lightSeeds.clear();

	while (head < queue.size()) {
		LightNode node = queue[head++];

		uint8_t level = node.packed & 0xF;
		if (level == 0) {
			continue;
		}

		uint8_t newLevel = level - 1;

		int8_t x = (node.packed >> 26) & 63;
		int8_t y = (node.packed >> 20) & 63;
		int8_t z = (node.packed >> 14) & 63;

		for (int32_t face = 0; face < 6; face++) {
			glm::i8vec3 newPosition = glm::i8vec3(x, y, z) + DIRECTIONS[face];

			Chunk* targetChunk = node.chunk;
			if (!isBlockInBounds(newPosition) && targetChunk) {
				newPosition &= CHUNK_END;
				targetChunk = targetChunk->m_neightborChunks[face];
			}

			if (!targetChunk) {
				continue;
			}

			uint32_t newIndex = calculateBlockIndex(newPosition);
			if (targetChunk->getLightAt(newIndex) >= newLevel) {
				continue;
			}

			trySeed(targetChunk, newPosition.x, newPosition.y, newPosition.z, newLevel);
		}
	}

	m_lightState.store(LightState::FullyPropagated, std::memory_order_release);
}

void Chunk::removeLight(int32_t x, int32_t y, int32_t z) noexcept {
	struct LightNode {
		Chunk* chunk;
		uint32_t packed;
	};

	static thread_local std::vector<LightNode> removalQueue;
	removalQueue.clear();
	static constexpr const size_t QUEUE_RESERVCE_SIZE = 2 << 12;
	if (removalQueue.capacity() < QUEUE_RESERVCE_SIZE) {
		removalQueue.reserve(QUEUE_RESERVCE_SIZE);
	}

	static thread_local std::vector<LightNode> relightQueue;
	relightQueue.clear();
	//static constexpr const size_t QUEUE_RESERVCE_SIZE = 2 << 12;
	if (relightQueue.capacity() < QUEUE_RESERVCE_SIZE) {
		relightQueue.reserve(QUEUE_RESERVCE_SIZE);
	}

	//gtl::vector<LightNode> removalQueue{};
//gtl::vector<LightNode> relightQueue{};
	//removalQueue.reserve(256);
	//relightQueue.reserve(256);
	size_t removalHead = 0;
	size_t relightHead = 0;

	uint32_t index = calculateBlockIndex(x, y, z);
	uint8_t oldLight = getLightAt(index);
	if (oldLight == 0) {
		return;
	}
	setLightAt(index, 0);
	removalQueue.emplace_back(this, (uint32_t)x << 26 | (uint32_t)y << 20 | (uint32_t)z << 14 | oldLight );

	while (!removalQueue.empty()) {
		LightNode node = removalQueue[removalHead++];

		uint8_t lightValue = node.packed & 0xF;
		int32_t localX = (node.packed >> 26) & 63;
		int32_t localY = (node.packed >> 20) & 63;
		int32_t localZ = (node.packed >> 14) & 63;

		for (int8_t face = 0; face < 6; face++) {
			glm::i8vec3 newPosition = glm::i8vec3(localX, localY, localZ) + DIRECTIONS[face];

			Chunk* targetChunk = node.chunk;
			if (!isBlockInBounds(newPosition)) {
				targetChunk = targetChunk->m_neightborChunks[face];
			}
			newPosition &= CHUNK_END;

			if (!targetChunk) {
				continue;
			}

			uint32_t neighborIndex = calculateBlockIndex(newPosition);
			uint8_t neighborValue = targetChunk->getLightAt(neighborIndex);
			if (neighborValue == 0) {
				continue;
			}

			if (neighborValue < lightValue) {
				targetChunk->setLightAt(neighborIndex, 0);
				removalQueue.emplace_back(targetChunk, (uint32_t)newPosition.x << 26 | (uint32_t)newPosition.y << 20 | (uint32_t)newPosition.z << 14 | neighborValue);
			}
			else {
				relightQueue.emplace_back(targetChunk, (uint32_t)newPosition.x << 26 | (uint32_t)newPosition.y << 20 | (uint32_t)newPosition.z << 14 | neighborValue);
			}
		}
	}

	while (relightHead < relightQueue.size()) {
		LightNode node = relightQueue[relightHead++];

		uint8_t lightValue = node.packed & 0xF;
		int32_t localX = (node.packed >> 26) & 63;
		int32_t localY = (node.packed >> 20) & 63;
		int32_t localZ = (node.packed >> 14) & 63;

		if (lightValue == 0) {
			continue;
		}

		glm::i8vec3 localPosition = glm::i8vec3(localX, localY, localZ);

		uint32_t localIndex = calculateBlockIndex(localPosition);

		if (!node.chunk->isBlockTransparent(localX, localY, localZ)) {
			continue;
		}

		if (node.chunk->getLightAt(localIndex) >= lightValue) {
			continue;
		}

		node.chunk->setLightAt(localIndex, lightValue);

		uint8_t newLevel = lightValue - 1;
		if (newLevel == 0) {
			continue;
		}

		for (int32_t face = 0; face < 6; face++) {
			glm::i8vec3 newPosition = localPosition + DIRECTIONS[face];

			Chunk* targetChunk = node.chunk;
			if (!isBlockInBounds(newPosition)) {
				targetChunk = targetChunk->m_neightborChunks[face];
			}

			newPosition &= CHUNK_END;

			if (!targetChunk) continue;

			uint32_t neighborIndex =calculateBlockIndex(newPosition);
			uint8_t neighborValue = targetChunk->getLightAt(neighborIndex);
			if (neighborValue < newLevel) {
				relightQueue.emplace_back(targetChunk, (uint32_t)newPosition.x << 26 | (uint32_t)newPosition.y << 20 | (uint32_t)newPosition.z << 14 | newLevel);
			}
		}
	}
}

void Chunk::clearLight() noexcept {
	m_sunlightStorage.fill(0);
}

// private
void Chunk::onBlockChange(int32_t localX, int32_t localY, int32_t localZ,
	const Block* oldBlock, const Block* newBlock) {

	if (!m_region || !m_isGenerated.load(std::memory_order_acquire)) {
		return;
	}

	int32_t worldY = m_chunkPosition.y * CHUNK_HEIGHT + localY;
	int32_t regionX = m_regionLocalBase.x + localX;
	int32_t regionZ = m_regionLocalBase.y + localZ;

	uint16_t currentHeight = getRegionPositionHeight(regionX, regionZ, m_region);

	bool oldOpaque = oldBlock && !oldBlock->isTransparent();
	bool newOpaque = newBlock && !newBlock->isTransparent();

	if (!oldOpaque && newOpaque) {
		if (worldY > currentHeight) {
			setRegionPositionHeight(regionX, regionZ, m_region, worldY);
		}
	}
	else if (oldOpaque && !newOpaque) {
		if (worldY == currentHeight) {
			setRegionPositionHeight(regionX, regionZ, m_region, rescalColumn(regionX, regionZ));
		}
	}
}

uint16_t Chunk::rescalColumn(int32_t regionX, int32_t regionZ) {
	int32_t localX = floorMod(regionX, CHUNK_WIDTH);
	int32_t localZ = floorMod(regionZ, CHUNK_DEPTH);

	for (int32_t y = (5 * CHUNK_HEIGHT) + CHUNK_HEIGHT - 1; y >= 0; y--) {
		int32_t chunkY = y / CHUNK_HEIGHT;
		int32_t localY = y % CHUNK_HEIGHT;

		int32_t regionChunkIndex = calulateChunkIndexInRegion(m_chunkPosition.x, chunkY, m_chunkPosition.z);

		if (regionChunkIndex < 0 || regionChunkIndex >= (int32_t)m_region->chunks.size()) {
			continue;
		}

		Chunk* chunk = m_region->chunks[regionChunkIndex];
		if (!chunk) {
			continue;
		}
		if (!chunk->isGenerated()) {
			continue;
		}

		if (!chunk->isBlockTransparent(localX, localY, localZ)) {
			return y;
		}
	}
	return 0;
}

uint8_t Chunk::getSunlight(int32_t worldX, int32_t worldY, int32_t worldZ) {
	int32_t localX = worldX - m_chunkPosition.x * (int32_t)CHUNK_WIDTH;
	int32_t localY = worldY - m_chunkPosition.y * (int32_t)CHUNK_HEIGHT;
	int32_t localZ = worldZ - m_chunkPosition.z * (int32_t)CHUNK_DEPTH;

	if (isBlockInBounds(localX, localY, localZ)) {
		uint32_t index = calculateBlockIndex(localX, localY, localZ);
		return getLightAt(index);
	}

	return 15;
}