#version 460

layout (binding = 0) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 proj;
};

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aNormal;

layout(location = 0) out vec4 pos;
layout(location = 1) out vec2 texcoord;
layout(location = 2) out vec3 norm;

void main()
{
	pos = proj * view * model * vec4(aPos, 1.0f);
	gl_Position = pos;
	norm = aNormal;
	texcoord = aTexcoord;
}