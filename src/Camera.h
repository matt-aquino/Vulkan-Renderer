#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <chrono>
#include <time.h>

class Camera
{
public:
	Camera(glm::vec3 pos, glm::vec3 target);
	~Camera();

	enum class KeyboardInputs
	{
		UP,
		DOWN,
		LEFT, 
		RIGHT,
		FORWARD,
		BACKWARD
	};

	static void HandleInput(KeyboardInputs input);
	static glm::mat4 GetViewMatrix() { return glm::lookAt(Camera::position, position + direction, up); }

private:
	void UpdateCamera(glm::vec3 deltaPos);
	static glm::vec3 position;
	static glm::vec3 direction;
	static glm::vec3 right;
	static glm::vec3 up;

	const glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
	float pitch;
	float yaw;
};

#endif //!CAMERA_H