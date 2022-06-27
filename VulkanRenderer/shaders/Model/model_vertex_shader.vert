#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 normal;

layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
	vec3 cameraPosition;
} ubo;


layout(location = 0) out vec3 viewDir;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec2 outTexcoord;

void main()
{
	gl_Position = ubo.projection * ubo.view * ubo.model * position;
	fragPos = vec3(ubo.model * position);
	viewDir = normalize(ubo.cameraPosition - fragPos);
	outNormal = normalize(vec3(ubo.projection * ubo.view * ubo.model * normal));
	outTexcoord = texcoord;
}