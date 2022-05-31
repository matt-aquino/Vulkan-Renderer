#version 460 core

layout(location = 0) in vec3 aPos;

layout(binding = 0) uniform Light
{
	mat4 depthVP; // view projection matrix from light's POV
};

layout(set = 1, binding = 0) uniform Mesh
{
	mat4 model; // per object model matrix
};

void main()
{
	gl_Position = depthVP * model * vec4(aPos, 1.0f);
}