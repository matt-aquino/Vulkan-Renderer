#include "Camera.h"

Camera::Camera(glm::vec3 pos, glm::vec3 target)
{
	position = pos;
	direction = position - target;
	UpdateCamera(glm::vec3(0.0f));
	pitch = 0.0f;
	yaw = -90.0f;
}

Camera::~Camera()
{
}

void Camera::HandleInput(KeyboardInputs input)
{
	glm::vec3 deltaPos = glm::vec3(0.0f);

	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count(); // grab time since start of application


	switch (input)
	{
	case KeyboardInputs::UP:
		deltaPos = glm::vec3(0.0f, 1.0f, 0.0f);
		break;

	case KeyboardInputs::DOWN:
		deltaPos = glm::vec3(0.0f, -1.0f, 0.0f);
		break;

	case KeyboardInputs::LEFT:
		deltaPos = glm::vec3(-1.0f, 0.0f, 0.0f);
		break;

	case KeyboardInputs::RIGHT:
		deltaPos = glm::vec3(1.0f, 0.0f, 0.0f);
		break;

	case KeyboardInputs::FORWARD:
		deltaPos = glm::vec3(0.0f, 0.0f, 1.0f);
		break;

	case KeyboardInputs::BACKWARD:
		deltaPos = glm::vec3(0.0f, 0.0f, -1.0f);
		break;

	default:
		break;
	}

	deltaPos *= dt;
	UpdateCamera(deltaPos);
}

void Camera::UpdateCamera(glm::vec3 deltaPos)
{
	position += deltaPos;

	// update view matrix
	direction.x = cos(glm::radians(yaw) * cos(glm::radians(pitch)));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw) * cos(glm::radians(pitch)));
	direction = glm::normalize(direction);
	right = glm::normalize(glm::cross(direction, WorldUp));
	up = glm::normalize(glm::cross(right, direction));
}