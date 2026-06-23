#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
	struct CreateInfo {
		glm::vec3 position{ 0.0f };
		float fov = 70.0f;

		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
	};

	static constexpr const glm::vec3 WORLD_UP = glm::vec3{ 0.0f, 1.0f, 0.0f };
public:
	Camera() = default;
	Camera(const Camera::CreateInfo& createInfo);
	~Camera() = default;

	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;

	Camera(Camera&& other) noexcept;
	Camera& operator=(Camera&& other) noexcept;

	void update(float deltaTime);

	glm::vec3 getPosition() const noexcept { return m_position; }
	glm::vec3 getFront() const noexcept { return m_front; }
	float getYaw() const noexcept { return m_yaw; }
	float getPitch() const noexcept { return m_pitch; }
	float getFOV() const noexcept { return m_fov; }
	float getNearPlane() const noexcept { return m_nearPlane; }
	float getFarPlane() const noexcept { return m_farPlane; }

	void setPosition(const glm::vec3& newPosition) noexcept { m_position = newPosition; }
	void setFOV(float newFOV) noexcept { m_fov = newFOV; }
	void setNearPlane(float newNearPlane) noexcept { m_nearPlane = newNearPlane; }
	void setFarPlane(float newFarPlane) noexcept { m_farPlane = newFarPlane; }

	glm::mat4 getPersepectiveMatrix(float aspectRatio) const noexcept;
	glm::mat4 getViewMatrix() const noexcept;
private:
	static constexpr const float CAMERA_SPEED = 20.0f;

	static constexpr const float MAX_PITCH_DEGREES = 89.0f;
	static constexpr const float MAX_PITCH_RADIANS = glm::radians(MAX_PITCH_DEGREES);
	static constexpr const float MIN_PITCH_DEGREES = -89.0f;
	static constexpr const float MIN_PITCH_RADIANS = glm::radians(MIN_PITCH_DEGREES);

	static constexpr const float CAMERA_SENSITIVITY = 0.05f;
private:
	glm::vec3 m_position{ 0.0f };
	glm::vec3 m_front{ 0.0f, 0.0f, 1.0f };

	float m_yaw = 0.0f;
	float m_pitch = 0.0f;

	float m_fov = 70.0f;

	float m_nearPlane = 0.1f;
	float m_farPlane = 1000.0f;
};