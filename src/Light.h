#ifndef LIGHT_H
#define LIGHT_H

#include "glm/glm.hpp"

class Light
{
public:
	Light();
	~Light();

	enum class LightType
	{
		POINT = 0,
		DIRECTIONAL = 1,
		SPOTLIGHT = 2,
		AREA = 3
	};

	glm::vec3 GetPosition() { return position; }
	glm::vec3 GetColor() { return color * intensity; }


private:
	LightType type;
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f);
	glm::vec3 color = glm::vec3(0.0f);
	float intensity = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
};



#endif //!LIGHT_H