#include <game/gameplay/player/raycasting.hpp>
#include <game/world/blocks/block.hpp>
#include <game/world/world.hpp>

#include <game/world/Chunk.hpp>

glm::i32vec3 getChunkPosition(glm::vec3 position) {
	int32_t cx = static_cast<int32_t>(std::floor(position.x / 64.0f));
	int32_t cy = static_cast<int32_t>(std::floor(position.y / 64.0f));
	int32_t cz = static_cast<int32_t>(std::floor(position.z / 64.0f)); 

	return glm::i32vec3(cx, cy, cz);
}

glm::i32vec3 getBlockPosition(glm::vec3 position) {
	int32_t localX = ((int32_t)glm::floor(position.x) % Chunk::CHUNK_WIDTH + Chunk::CHUNK_WIDTH) % Chunk::CHUNK_WIDTH;
	int32_t localY = ((int32_t)glm::floor(position.y) % Chunk::CHUNK_HEIGHT + Chunk::CHUNK_HEIGHT) % Chunk::CHUNK_HEIGHT;
	int32_t localZ = ((int32_t)glm::floor(position.z) % Chunk::CHUNK_DEPTH + Chunk::CHUNK_DEPTH) % Chunk::CHUNK_DEPTH;

	return glm::i32vec3(localX, localY, localZ);
}

bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, World& world, HitResult& result) {
	glm::vec3 position = glm::floor(origin);
	glm::ivec3 step = glm::ivec3(glm::sign(direction));

	glm::vec3 tMax{ 0.0f };
	glm::vec3 tDelta{ 0.0f };

	for (size_t i = 0; i < 3; i++) {
		if (direction[i] == 0.0f) {
			tMax[i] = std::numeric_limits<float>::infinity();
			tDelta[i] = std::numeric_limits<float>::infinity();
			continue;
		}
		

		float nextBoundry = position[i] + (step[i] > 0 ? 1.0f : 0.0f);
		tMax[i] = (nextBoundry - origin[i]) / direction[i];
		tDelta[i] = 1.0f / glm::abs(direction[i]);
	}

	float distance = 0.0f;

	auto stepRay = [&]() {
		if (tMax.x < tMax.y && tMax.x < tMax.z) {
			position.x += step.x;
			distance = tMax.x;
			tMax.x += tDelta.x;
			result.faceNormal = step.x < 0 ? glm::ivec3(1, 0, 0) : glm::ivec3(-1, 0, 0);
			return;
		}

		if (tMax.y < tMax.z) {
			position.y += step.y;
			distance = tMax.y;
			tMax.y += tDelta.y;
			result.faceNormal = step.y < 0 ? glm::ivec3(0, 1, 0) : glm::ivec3(0, -1, 0);
			return;
		}

		position.z += step.z;
		distance = tMax.z;
		tMax.z += tDelta.z;
		result.faceNormal = step.z < 0 ? glm::ivec3(0, 0, 1) : glm::ivec3(0, 0, -1);
	};

	while (distance < maxDistance) {
		glm::vec3 chunkPosition = getChunkPosition(position);
		Chunk* chunk = world.getChunk(chunkPosition);

		if (chunk && chunk->isGenerated()) {

			glm::i32vec3 localPosition = getBlockPosition(position);
			const Block* block = chunk->getBlock(localPosition.x, localPosition.y, localPosition.z);
			if (!block->isAir() && !block->isWater()) {
				result.blockPosition = glm::ivec3(glm::floor(position));
				result.distance = distance;
				return true;
			}
		}

		stepRay();
	}

	return false;
}

bool placeBlock(const glm::vec3& origin, const glm::vec3& direction, const Block& block, World& world) {
	HitResult hitResult{};
	if (!raycast(origin, direction, 10.0f, world, hitResult)) {
		return false;
	}

	glm::ivec3 placePosition = hitResult.blockPosition + hitResult.faceNormal;

	if (placePosition == (glm::ivec3)glm::floor(origin) || 
		placePosition == (glm::ivec3)glm::floor(origin - glm::vec3(0.0f, 1.0f, 0.0f)) ||
		placePosition == (glm::ivec3)glm::floor(origin - glm::vec3(0.0f, 2.0f, 0.0f))) {
		return false;
	}

	glm::ivec3 chunkPosition = getChunkPosition(placePosition);

	Chunk* chunk = world.getChunk(chunkPosition);
	if (!chunk) {
		return false;
	}

	glm::i32vec3 localPosition = getBlockPosition(placePosition);

	chunk->setBlock(localPosition.x, localPosition.y, localPosition.z, block);
	world.addChunkToRemeshingQueue(chunk);

	return true;
}

bool breakBlock(const glm::vec3& origin, const glm::vec3& direction, World& world) {
	HitResult hitResult{};
	if (!raycast(origin, direction, 10.0f, world, hitResult)) {
		return false;
	}

	glm::ivec3 placePosition = hitResult.blockPosition;
	glm::ivec3 chunkPosition = getChunkPosition(placePosition);

	Chunk* chunk = world.getChunk(chunkPosition);
	if (!chunk) {
		return false;
	}

	const Block& air = world.getBlock("air");
	glm::i32vec3 localPosition = getBlockPosition(placePosition);

	chunk->setBlock(localPosition.x, localPosition.y, localPosition.z, air);
	world.addChunkToRemeshingQueue(chunk);
	return true;
}