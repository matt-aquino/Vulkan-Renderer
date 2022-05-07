#include "Camera.h"

Camera* Camera::camera = nullptr;

Camera::Camera(glm::vec3 pos)
{
	position = pos;
	startPosition = position;
	pitch = -15.0f;
	yaw = -90.0f;
	zoom = 45.0f;
	forward = glm::vec3(0.0f, 0.0f, -1.0f);
	up = WorldUp;
	right = glm::vec3(1.0f, 0.0f, 0.0f);
}

Camera::~Camera()
{
	delete camera;
}

void Camera::HandleInput(KeyboardInputs input, float dt)
{
	glm::vec3 deltaPos = glm::vec3(0.0f);


	switch (input)
	{
		case KeyboardInputs::UP:
			deltaPos = up;
			break;

		case KeyboardInputs::DOWN:
			deltaPos = -up;
			break;

		case KeyboardInputs::LEFT:
			deltaPos = -right;
			break;

		case KeyboardInputs::RIGHT:
			deltaPos = right;
			break;

		case KeyboardInputs::FORWARD:
			deltaPos = forward;
			break;

		case KeyboardInputs::BACKWARD:
			deltaPos = -forward;
			break;

		default:
			break;
	}

	deltaPos *= dt;
	position += deltaPos;
}


glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(position, position + forward, up);
}
void Camera::RotateCamera(float xOffset, float yOffset)
{
	pitch += -yOffset;
	glm::clamp(pitch, -89.0f, 89.0f);

	yaw += xOffset;

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	forward.x = cos(glm::radians(yaw) * cos(glm::radians(pitch)));
	forward.y = sin(glm::radians(pitch));
	forward.z = sin(glm::radians(yaw) * cos(glm::radians(pitch)));
	forward = glm::normalize(forward);
	right = glm::normalize(glm::cross(forward, WorldUp));
	up = glm::normalize(glm::cross(right, forward));
}

void Camera::ResetCamera()
{
	// reset all camera values
	position = startPosition;
	pitch = 0.0f;
	yaw = 90.0f;
	zoom = 45.0f;
	forward = glm::vec3(0.0f, 0.0f, 1.0f);
	up = WorldUp;
	right = glm::vec3(1.0f, 0.0f, 0.0f);
}