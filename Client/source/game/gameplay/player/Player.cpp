#include <game/gameplay/player/Player.hpp>

#include <core/input.hpp>

#include <game/world/world.hpp>
#include <game/world/Chunk.hpp>
#include <game/gameplay/player/raycasting.hpp>

#include <game/world/blocks/block.hpp>

Player::Player(const Player::CreateInfo& createInfo) {
	m_position = createInfo.position;

	Camera::CreateInfo cameraCreateInfo{};
	cameraCreateInfo.position = m_position + glm::vec3(0.0f, 0.8f, 0.0f);
	cameraCreateInfo.fov = createInfo.fov;
	cameraCreateInfo.farPlane = createInfo.farPlane;
	cameraCreateInfo.nearPlane = createInfo.nearPlane;

	m_camera = Camera(cameraCreateInfo);
}

Player::~Player() {

}

Player::Player(Player&& other) noexcept :
	m_camera{std::move(other.m_camera)},
	m_position{std::move(other.m_position)}
{

}

Player& Player::operator=(Player&& other) noexcept {
	m_camera = std::move(other.m_camera);
	m_position = other.m_position;

	return *this;
}

void Player::update(float deltaTime) noexcept {
	//m_camera.setPosition(getEyePosition());
	m_camera.update(deltaTime);
}

void Player::tickUpdate(float tickDelta, World& world) noexcept {
	m_previousPosition = m_position;

	glm::vec3 front = m_camera.getFront();
	glm::vec3 flatFront = glm::vec3(front.x, 0.0f, front.z);
	glm::vec3 right{ 0.0f };
	if (glm::length(flatFront) > 0.001f) {
		flatFront = glm::normalize(flatFront);
		right = glm::normalize(glm::cross(flatFront, Camera::WORLD_UP));
	}

	if (Input::isKeyDown(GLFW_KEY_SPACE) && (m_isOnGround || m_isInWater)) {
		if (m_isInWater) {
			glm::vec3 eyePosition = getEyePosition();
			glm::i32vec3 eyeChunkhunkPosition = getChunkPosition(eyePosition);
			glm::i32vec3 blockPosition = getBlockPosition(eyePosition);

			bool is2DeepWater = false;
			if (Chunk* eyeChunk = world.getChunk(eyeChunkhunkPosition)) {
				
				const Block* block = eyeChunk->getBlock(blockPosition.x, blockPosition.y, blockPosition.z);
				is2DeepWater = block && block->isWater();
			}

			bool shouldWaterBoost = false;
			if (!is2DeepWater) {
				if (Chunk* chunk = world.getChunk(getChunkPosition(m_position))) {
					for (int8_t x = -1; x < 2; x++) {
						for (int8_t z = -1; z < 2; z++) {
							Chunk* currentChunk = chunk;
							glm::i8vec3 neighborPosition = glm::i8vec3(getBlockPosition(m_position)) + glm::i8vec3(x, 0, z);
							if (!chunk->isBlockInBounds(neighborPosition)) {
								neighborPosition &= (glm::i8vec3)Chunk::CHUNK_END;
								currentChunk = world.getChunk(getChunkPosition(neighborPosition));
							}

							if (!currentChunk) {
								continue;
							}

							const Block* neighborBlock = currentChunk->getBlock(neighborPosition.x, neighborPosition.y, neighborPosition.z);
							if (neighborBlock && neighborBlock->hasPhysics()) {
								shouldWaterBoost = true;
								break;
							}
						}
					}
				}
			}
			m_velocity.y = shouldWaterBoost ? 6.0f : 2.0f;
		}
		else {
			m_velocity.y = 8.0f;
		}
		
	}

	if (m_isOnGround && m_velocity.y < 0.0f) {
		m_velocity.y = 0.0f;
	}

	m_isOnGround = false;

	glm::vec3 wishDirection{ 0.0f };
	if (Input::isKeyDown(GLFW_KEY_W)) {
		wishDirection += flatFront;
	}

	if (Input::isKeyDown(GLFW_KEY_S)) {
		wishDirection -= flatFront;
	}

	if (Input::isKeyDown(GLFW_KEY_A)) {
		wishDirection -= right;
	}

	if (Input::isKeyDown(GLFW_KEY_D)) {
		wishDirection += right;
	}

	if (glm::length(wishDirection) > 0.0f) {
		wishDirection = glm::normalize(wishDirection);
	}

	
	float acceleration = glm::mix(m_acceleration, m_waterAcceleration, m_waterBlend);
	m_velocity += wishDirection * acceleration * tickDelta;
	
	float gravity = glm::mix(18.0f, 4.0f, m_waterBlend);
	m_velocity.y -= gravity * tickDelta;

	
	float drag = glm::mix(m_drag, m_dragWater, m_waterBlend);
	glm::vec3 dragForce = glm::vec3(m_velocity.x, 0.0f, m_velocity.z) * drag * tickDelta;
	m_velocity -= dragForce;

	glm::vec2 horizontalVelocity = glm::vec2(m_velocity.x, m_velocity.z);
	float horizontalSpeed = glm::length(horizontalVelocity);

	float maxSpeed = glm::mix(m_maxSpeed, m_maxSpeedWater, m_waterBlend);
	if (horizontalSpeed > maxSpeed) {
		horizontalVelocity = (horizontalVelocity / horizontalSpeed) * maxSpeed;
		m_velocity.x = horizontalVelocity.x;
		m_velocity.z = horizontalVelocity.y;
	}

	glm::vec3 delta = m_velocity * tickDelta;
	m_position = resolveColision(m_position, delta, tickDelta,world);

	glm::i32vec3 chunkPosition = getChunkPosition(m_position);
	if (Chunk* chunk = world.getChunk(chunkPosition)) {
		glm::i32vec3 blockPosition = getBlockPosition(m_position);
		const Block* block = chunk->getBlock(blockPosition.x, blockPosition.y, blockPosition.z);

		m_isInWater = block && block->isWater();

		float blendTarget = m_isInWater ? 1.0f : 0.0f;
		float blendSpeed = (blendTarget > m_waterBlend) ? 10.0f : 5.0f;
		m_waterBlend += (blendTarget - m_waterBlend) * blendSpeed * tickDelta;
		m_waterBlend = glm::clamp(m_waterBlend, 0.0f, 1.0f);
	}
	
}

void Player::interpolate(float alpha) {
	m_renderPosition = glm::mix(m_previousPosition, m_position, alpha);
	m_camera.setPosition(m_renderPosition + glm::vec3(0.0f, 0.8f, 0.0f));
}

glm::mat4 Player::getPerspectiveMatrix(float aspectRatio) const noexcept {
	return m_camera.getPersepectiveMatrix(aspectRatio);
}

glm::mat4 Player::getViewMatrix() const noexcept {
	return m_camera.getViewMatrix();
}

void Player::placeBlock(const Block& block, World& world) {
	glm::vec3 origin = getEyePosition();
	glm::vec3 direction = m_camera.getFront();

	HitResult hitResult{};
	if (!raycast(origin, direction, 10.0f, world, hitResult)) {
		return;
	}

	glm::ivec3 placePosition = hitResult.blockPosition + hitResult.faceNormal;
	
	if (doesBlockCollideWithPlayer(placePosition)) {
		return;
	}

	glm::ivec3 chunkPosition = getChunkPosition(placePosition);

	Chunk* chunk = world.getChunk(chunkPosition);
	if (!chunk) {
		return;
	}

	glm::i32vec3 localPosition = getBlockPosition(placePosition);

	chunk->setBlock(localPosition.x, localPosition.y, localPosition.z, block);
	world.addChunkToRemeshingQueue(chunk);
}

void Player::breakBlock(World& world) {
	glm::vec3 origin = getEyePosition();
	glm::vec3 direction = m_camera.getFront();

	HitResult hitResult{};
	if (!raycast(origin, direction, 10.0f, world, hitResult)) {
		return;
	}

	glm::ivec3 placePosition = hitResult.blockPosition;
	glm::ivec3 chunkPosition = getChunkPosition(placePosition);

	Chunk* chunk = world.getChunk(chunkPosition);
	if (!chunk) {
		return;
	}

	const Block& air = world.getBlock("air");
	glm::i32vec3 localPosition = getBlockPosition(placePosition);

	chunk->setBlock(localPosition.x, localPosition.y, localPosition.z, air);
	world.addChunkToRemeshingQueue(chunk);
}

// private

bool Player::colldiesWithWorld(const glm::vec3& position, World& world) const noexcept {
	glm::ivec3 mins = glm::ivec3(glm::floor(position - PLAYER_HALF_EXTENTS));
	glm::ivec3 maxs = glm::ivec3(glm::floor((position + PLAYER_HALF_EXTENTS) - glm::vec3(0.001f)));

	for (int32_t x = mins.x; x <= maxs.x; x++) {
		for (int32_t y = mins.y; y <= maxs.y; y++) {
			for (int32_t z = mins.z; z <= maxs.z; z++) {
				glm::vec3 currentPosition = glm::vec3(x, y, z);
				glm::ivec3 chunkPosition = getChunkPosition(currentPosition);

				Chunk* chunk = world.getChunk(chunkPosition);
				if (!chunk) {
					return false;
				}

				glm::i32vec3 localPosition = getBlockPosition(currentPosition);

				const Block* block = chunk->getBlock(localPosition.x, localPosition.y, localPosition.z);
				if (block->hasPhysics()) {
					return true;
				}
			}
		}
	}

	return false;
}

bool Player::doesBlockCollideWithPlayer(const glm::ivec3& blockPosition) const noexcept {
	glm::ivec3 mins = glm::ivec3(glm::floor(m_position - PLAYER_HALF_EXTENTS));
	glm::ivec3 maxs = glm::ivec3(glm::floor((m_position + PLAYER_HALF_EXTENTS) - glm::vec3(0.001f)));

	return (blockPosition.x >= mins.x) && (blockPosition.x <= maxs.x) &&
		(blockPosition.y >= mins.y) && (blockPosition.y <= maxs.y) &&
		(blockPosition.z >= mins.z) && (blockPosition.z <= maxs.z);
}

glm::vec3 Player::resolveColision(const glm::vec3& position, const glm::vec3& delta, float deltaTick, World& world) noexcept {
	glm::vec3 resolved = position;

	static constexpr float cornerThreshold = 0.5f;
	static constexpr float nudgeStrength = 0.05f;

	static auto stepCollision = [&](float deltaComponent, const glm::vec3& positiveAxis, float& resolvedComponent) -> void {
		float step = glm::sign(deltaComponent) * 0.001f;
		while (!colldiesWithWorld(resolved + (positiveAxis * step), world)) {
			resolvedComponent += step;
		}
	};

	glm::vec3 tryX = resolved + glm::vec3(delta.x, 0.0f, 0.0f);
	if (!colldiesWithWorld(tryX, world)) {
		resolved = tryX;
	}
	else {
		float zOffset = resolved.z - glm::floor(resolved.z) - 0.5f;
		if (glm::abs(zOffset) < cornerThreshold) {
			float t = 1.0f - (glm::abs(zOffset) / cornerThreshold);
			glm::vec3 nudge = tryX;
			nudge.z -= glm::sign(zOffset) * nudgeStrength * t;
			if (!colldiesWithWorld(nudge, world)) {
				resolved = nudge;
			}
			else {
				stepCollision(delta.x, glm::vec3(1.0f, 0.0, 0.0), resolved.x);
			}
			m_velocity.x = 0.0f;
		}
		else {
			stepCollision(delta.x, glm::vec3(1.0f, 0.0, 0.0), resolved.x);
		}
		m_velocity.x = 0.0f;
		
	}

	glm::vec3 tryY = resolved + glm::vec3(0.0f, delta.y, 0.0f);
	if (!colldiesWithWorld(tryY, world)) {
		resolved = tryY;
	}
	else {
		stepCollision(delta.y, glm::vec3(0.0f, 1.0, 0.0), resolved.y);

		if (delta.y < 0.0f) {
			m_isOnGround = true;
		}

		m_velocity.y = 0.0f;
	}

	glm::vec3 tryZ = resolved + glm::vec3(0.0f, 0.0f, delta.z);
	if (!colldiesWithWorld(tryZ, world)) {
		resolved = tryZ;
	}
	else {
		float xOffset = resolved.x - glm::floor(resolved.x) - 0.5f;
		if (glm::abs(xOffset) < cornerThreshold) {
			float t = 1.0f - (glm::abs(xOffset) / cornerThreshold);
			glm::vec3 nudge = tryZ;
			nudge.x -= glm::sign(xOffset) * nudgeStrength * t;
			if (!colldiesWithWorld(nudge, world)) {
				resolved = nudge;
			}
			else {
				stepCollision(delta.z, glm::vec3(0.0f, 0.0f, 1.0f), resolved.z);
			}
			m_velocity.z = 0.0f;
		}
		else {
			stepCollision(delta.z, glm::vec3(0.0f, 0.0f, 1.0f), resolved.z);
		}
		m_velocity.z = 0.0f;
	}

	if (m_velocity.x == 0.0f && m_velocity.z == 0.0f && (delta.x != 0.0f || delta.z != 0.0f)) {
		if (delta.z != 0.0f) {
			glm::vec3 tryZSlide = resolved + glm::vec3(0.0f, 0.0f, delta.z);
			if (!colldiesWithWorld(tryZSlide, world)) {
				resolved = tryZSlide;
				m_velocity.z = delta.z / deltaTick;
			}
		}
		if (delta.x != 0.0f) {
			glm::vec3 tryXSlide = resolved + glm::vec3(delta.x, 0.0f, 0.0f);
			if (!colldiesWithWorld(tryXSlide, world)) {
				resolved = tryXSlide;
				m_velocity.x = delta.x / deltaTick;
			}
		}
	}

	return resolved;
}