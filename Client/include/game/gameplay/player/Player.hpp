#pragma once

#include <glm/glm.hpp>

#include <camera/camera.hpp>

class World;
class Block;

class Player {
public:
	struct CreateInfo {
		glm::vec3 position{ 0.0f };

		float fov = 70.0f;

		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
	};

	static constexpr const uint32_t PLAYER_BLOCK_BREAK_TICK_COOLDOWN = 6;
	static constexpr const uint32_t PLAYER_BLOCK_PLACE_INITIAL_DELAY = 4;
	static constexpr const uint32_t PLAYER_BLOCK_PLACE_TICK_COOLDOWN = 2;
public:
	Player() = default;
	Player(const CreateInfo& createInfo);
	~Player();

	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

	Player(Player&&) noexcept;
	Player& operator=(Player&&) noexcept;

	void update(float deltaTime) noexcept;
	void tickUpdate(float tickDelta, World& world) noexcept;
	void interpolate(float alpha);

	glm::vec3 getPosition() const noexcept { return m_position; }
	glm::vec3 getRenderPosition() const noexcept {
		return m_renderPosition;
	}
	glm::vec3 getEyePosition() const noexcept { return m_position + glm::vec3(0.0f, 0.8f, 0.0f); }
	glm::vec3 getLookingDirection() const noexcept { return m_camera.getFront(); }

	void setPosition(glm::vec3 newPosition) noexcept { m_position = newPosition; m_camera.setPosition(m_position); }

	glm::mat4 getPerspectiveMatrix(float aspectRatio) const noexcept;
	glm::mat4 getViewMatrix() const noexcept;

	uint32_t canBreakBlocks() const noexcept { return m_blockBreakCooldown == 0; }
	void applyBreakBlockCooldown() { m_blockBreakCooldown = PLAYER_BLOCK_BREAK_TICK_COOLDOWN; }
	void decrementBreakBlockCooldown() {
		if (m_blockBreakCooldown > 0) {
			m_blockBreakCooldown--;
		}
	}

	void startPlaceCharge() noexcept { m_blockPlaceCooldown = PLAYER_BLOCK_PLACE_INITIAL_DELAY; }
	void resetPlaceCharge() noexcept { m_blockPlaceCooldown = 0; }
	bool canPlaceBlock() const noexcept { return m_blockPlaceCooldown == 0; }
	void applyPlaceBlockCooldown() noexcept { m_blockPlaceCooldown = PLAYER_BLOCK_PLACE_TICK_COOLDOWN; }
	void decrementPlaceBlockCooldown() noexcept {
		if (m_blockPlaceCooldown > 0) {
			m_blockPlaceCooldown--;
		}
	}

	void placeBlock(const Block& block, World& world);
	void breakBlock(World& world);

	void resetVelocity() noexcept { m_velocity = glm::vec3(0.0f); }
private:
	//static constexpr float PLAYER_SPEED = 20.0f;
	static constexpr float PLAYER_SPEED = 100.0f;
	static constexpr float PALYER_SPEED_WATER = 10.0f;

	static constexpr glm::vec3 PLAYER_HALF_EXTENTS = glm::vec3{ 0.4f, 0.8f, 0.4f };

	static constexpr const float m_acceleration = 40.0f;
	static constexpr const float m_waterAcceleration = 20.0f;
	static constexpr const float m_maxSpeed = 8.0f;
	static constexpr const float m_maxSpeedWater = 4.0f;
	//static constexpr const float m_maxSpeed = 100.0f;
	static constexpr const float m_drag = m_acceleration / m_maxSpeed;
	static constexpr const float m_dragWater = m_waterAcceleration / m_maxSpeedWater;

	static constexpr float cornerThreshold = 0.4f;
	static constexpr float nudgeStrength = 0.05f;
private:
	bool colldiesWithWorld(const glm::vec3& position, World& world) const noexcept;
	bool doesBlockCollideWithPlayer(const glm::ivec3& blockPosition) const noexcept;

	glm::vec3 resolveColision(const glm::vec3& position, const glm::vec3& delta, float deltaTick, World& word) noexcept;
private:
	Camera m_camera{};

	glm::vec3 m_position{ 0.0f, 0.0f, 0.0f };
	glm::vec3 m_previousPosition{ 0.0f };
	glm::vec3 m_renderPosition{ 0.0f };

	glm::vec3 m_velocity{ 0.0f };
	
	bool m_isOnGround = false;
	bool m_isInWater = false;

	float m_waterBlend = 0.0f;

	uint32_t m_blockBreakCooldown = 0;
	uint32_t m_blockPlaceCooldown = 0;
};