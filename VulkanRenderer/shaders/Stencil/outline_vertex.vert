#version 460 

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aNormal;

layout(binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 proj;
	float outlineWidth;
};
void main()
{
	vec4 outlinePosition = vec4(aPos + aNormal * outlineWidth, 1.0f);
	gl_Position = proj * view * model * outlinePosition;
}