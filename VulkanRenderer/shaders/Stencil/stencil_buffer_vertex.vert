#version 460

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexcoords;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec2 texcoords;
layout(location = 2) out vec3 viewDir;

layout(binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 proj;
	float outlineWidth;
	vec3 cameraPos;
	vec3 cameraFront;
};


void main()
{
	fragPos = proj * view * model * vec4(aPos, 1.0f);
	gl_Position = fragPos;
	viewDir = cameraPos - fragPos.xyz;
	texcoords = aTexcoords;
}