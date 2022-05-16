#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aNormal;

layout(set = 0, binding = 0) uniform UBO
{
	mat4 proj;
	mat4 view;
	vec3 lightPos;
	vec3 viewPos;
};

layout(set = 1, binding = 0) uniform Mesh
{
	mat4 model;
	mat4 normal;
};

layout(location = 0) out vec3 outWorldNormal;
layout(location = 1) out vec3 outVertPos;
layout(location = 2) out vec3 outLightPos;
layout(location = 3) out vec3 outViewPos;

void main()
{
	outWorldNormal = vec3(normal * vec4(aNormal, 0.0));

	vec4 vertPos = model * vec4(aPos, 1.0);
	outVertPos = vertPos.xyz / vertPos.w;
	gl_Position = proj * view * vertPos;

	outLightPos = lightPos;
	outViewPos = viewPos;
}