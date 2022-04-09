#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;


layout(binding = 1) uniform UBO
{
	mat4 model;
	mat4 view;
	mat4 proj;
};

layout(location = 0) out vec3 pColor;
void main()
{
	gl_PointSize = 1.0f;
	gl_Position = proj * view * model * vec4(position, 1.0);
	pColor = position * color;
}