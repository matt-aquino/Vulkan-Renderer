#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <chrono>
#include <time.h>

const glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);

enum class KeyboardInputs
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	FORWARD,
	BACKWARD
};

class Camera
{
public:
	static Camera* GetCamera()
	{
		if (camera == nullptr)
		{
			camera = new Camera(glm::vec3(0.0f, 2.0f, 5.0f));
			camera->UpdateViewMatrix();
		}

		return camera;
	}

	Camera(Camera& other) = delete;
	void operator=(const Camera&) = delete;

	void HandleInput(KeyboardInputs input, float dt);
	void RotateCamera(float xOffset, float yOffset);
	void UpdateViewMatrix();

	glm::mat4 GetViewMatrix();
	glm::vec3 GetCameraPosition() { return position; }
	float GetFOV() { return zoom; }

private:
	Camera(glm::vec3 pos);
	~Camera();

	
	void ChangeZoom(float dt);
	void ResetCamera();

	glm::vec3 startPosition;
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;

	float pitch;
	float yaw;
	float zoom;
protected:

	static Camera* camera;
};

#endif //!CAMERA_H