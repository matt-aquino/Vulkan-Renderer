#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Light
{
public:
	Light();
	virtual ~Light() {}

	virtual void setLightPos(glm::vec3 newPos);

	glm::vec3 getLightColor();
	void setLightColor(glm::vec3 col);

	float getLightIntensity();
	void setLightIntensity(float in);

protected :
	enum class LightType
	{
		NONE = -1,
		POINT = 0,
		SPOTLIGHT,
		DIRECTIONAL,
		AREA
	};

	glm::vec3 position;
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 1.0f;
	LightType type = LightType::NONE;

	glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
};


class SpotLight : public Light
{
public:
	SpotLight();
	SpotLight(glm::vec3 position, glm::vec3 target = glm::vec3(0.0f), glm::vec3 col = glm::vec3(1.0f), float n = 1.0f, float f = 100.0f);
	virtual ~SpotLight();

	glm::vec3 getLightPos() { return position; }
	void setLightPos(glm::vec3 newPos) override;
	glm::mat4 getLightProj();
	glm::mat4 getLightView();

	float getNearPlane();
	float getFarPlane();
private:
	glm::mat4 proj = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
	glm::vec3 target = glm::vec3(0.0f);
	float nearPlane = 1.0f;
	float farPlane = 100.0f;
};

class DirectionalLight : public Light
{
public:
	DirectionalLight();
	DirectionalLight(float size = 10.0f, glm::vec3 target = glm::vec3(0.0f), glm::vec3 col = glm::vec3(1.0f));
	virtual ~DirectionalLight();

private:
	glm::mat4 proj = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);

	float distance = 10000.0f;
};

class PointLight : public Light
{
public:
	PointLight();
	PointLight(glm::vec3 position, glm::vec3 col = glm::vec3(1.0f));
	virtual ~PointLight();

	glm::vec3 getLightPos() { return position; }
private:
};

struct AreaLight : public Light
{
public:
	enum class AreaType
	{
		RECTANGLE = 0,
		TUBE,
		DISC,
		SPHERE
	};

	AreaLight();
	virtual ~AreaLight();
	glm::vec3 getLightPos() { return position; }

private:
	AreaType areaType = AreaType::RECTANGLE;
};