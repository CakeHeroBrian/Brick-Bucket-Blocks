#include <camera/camera.hpp>

#include <core/input.hpp>
#include <GLFW/glfw3.h>

Camera::Camera(const Camera::CreateInfo& createInfo) :
	m_position{createInfo.position},
	m_fov{createInfo.fov},
	m_nearPlane{createInfo.nearPlane},
	m_farPlane{createInfo.farPlane}
{

}

Camera::Camera(Camera&& other) noexcept :
	m_position{other.m_position },
	m_front{other.m_front},
	m_yaw{other.m_yaw},
	m_pitch{other.m_pitch},
	m_fov{other.m_fov},
	m_nearPlane{other.m_nearPlane},
	m_farPlane{other.m_farPlane}
{

}

Camera& Camera::operator=(Camera&& other) noexcept {
	m_position = other.m_position;
	m_front = other.m_front;
	m_yaw = other.m_yaw;
	m_pitch = other.m_pitch;
	m_fov = other.m_fov;
	m_nearPlane = other.m_nearPlane;
	m_farPlane = other.m_farPlane;

	return *this;
}

void Camera::update(float deltaTime) {
	m_yaw += glm::radians(Input::deltaMouseX * CAMERA_SENSITIVITY);
	m_yaw = std::fmod(m_yaw + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
	m_pitch += glm::radians(Input::deltaMouseY * CAMERA_SENSITIVITY);
	m_pitch = glm::clamp(m_pitch, MIN_PITCH_RADIANS, MAX_PITCH_RADIANS);

	m_front = {
		glm::cos(m_yaw) * glm::cos(m_pitch),
		glm::sin(m_pitch),
		glm::sin(m_yaw) * glm::cos(m_pitch)
	};
	m_front = glm::normalize(m_front);
}

glm::mat4 Camera::getPersepectiveMatrix(float aspectRatio) const noexcept {
	glm::mat4 perspective = glm::perspective(
		m_fov,
		aspectRatio,
		m_nearPlane,
		m_farPlane
	);

	return perspective;
}

glm::mat4 Camera::getViewMatrix() const noexcept {
	return glm::lookAt(
		m_position,
		m_position + m_front,
		WORLD_UP
	);
}