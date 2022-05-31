#include "Light.h"

#pragma region BASE
void Light::setLightColor(glm::vec3 col)
{
	color = col;
}

glm::vec3 Light::getLightPos()
{
	return position;
}

void Light::setLightPos(glm::vec3 newPos)
{
	position = newPos;
}

glm::vec3 Light::getLightColor()
{
	return color;
}

float Light::getLightIntensity()
{
	return intensity;
}

void Light::setLightIntensity(float in)
{
	intensity = in;
}
#pragma endregion

#pragma region SPOTLIGHT
SpotLight::SpotLight()
{
	type = LightType::SPOTLIGHT;
}

SpotLight::SpotLight(glm::vec3 position, glm::vec3 targ, glm::vec3 col, float n, float f)
{
	type = LightType::SPOTLIGHT;
	pos = position;
	nearPlane = n;
	farPlane = f;

	proj = glm::perspective(glm::radians(45.0f), 1.0f, nearPlane, farPlane);
	proj[1][1] *= -1;
	target = targ;
	view = glm::lookAt(pos, target, worldUp);
	color = col;
}

SpotLight::~SpotLight()
{

}

glm::mat4 SpotLight::getLightProj()
{
	return proj;
}

glm::mat4 SpotLight::getLightView()
{
	return view;
}


float SpotLight::getNearPlane()
{
	return nearPlane;
}

float SpotLight::getFarPlane()
{
	return farPlane;
}

void SpotLight::setLightPos(glm::vec3 newPos)
{
	pos = newPos;
	view = glm::lookAt(pos, target, worldUp);
}
#pragma endregion

#pragma region DIRECTIONAL
DirectionalLight::DirectionalLight()
{
	type = LightType::DIRECTIONAL;
}

DirectionalLight::DirectionalLight(float size, glm::vec3 target, glm::vec3 col)
{
	type = LightType::DIRECTIONAL;
	proj = glm::ortho(-size, size, -size, size);
	proj[1][1] *= -1;
	view = glm::lookAt(glm::vec3(distance), target, worldUp);
	color = col;
}
DirectionalLight::~DirectionalLight()
{
}


#pragma endregion

#pragma region POINTLIGHT

PointLight::PointLight()
{
	type = LightType::POINT;
}

PointLight::PointLight(glm::vec3 position, glm::vec3 col)
{
	type = LightType::POINT;
	pos = position;
	color = col;
}

PointLight::~PointLight()
{
}

#pragma endregion

#pragma region AREALIGHT
AreaLight::AreaLight()
{
	type = LightType::AREA;
}

AreaLight::~AreaLight()
{
}


#pragma endregion