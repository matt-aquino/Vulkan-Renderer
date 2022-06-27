#version 460 core

layout(location = 0) in vec4 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec4 aNormal;

layout(set = 0, binding = 0) uniform Light
{
	mat4 viewProj; // view projection matrix from light's POV
} light;

layout(set = 1, binding = 0) uniform Mesh
{
	mat4 model;  // per object model matrix
	mat4 normal; // not needed here, but keeps the alignment
}mesh;

void main()
{
	gl_Position = light.viewProj * mesh.model * aPos;
}