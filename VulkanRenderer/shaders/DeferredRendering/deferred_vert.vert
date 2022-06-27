#version 460 core

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec2 aTexcoord;
layout (location = 2) in vec4 aNormal;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 viewProj;
};

layout (set = 1, binding = 0) uniform Mesh
{
	mat4 model;
	mat4 normal;
} mesh;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;

void main()
{
	vec4 worldPos = mesh.model * aPos;

	outPos = worldPos.xyz;
	outNormal = (mesh.normal * aNormal).xyz;

	gl_Position = viewProj * worldPos;
}