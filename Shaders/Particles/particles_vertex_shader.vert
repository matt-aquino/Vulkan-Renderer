#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
}push; 

layout(location = 0) out vec3 pColor;
void main()
{
	gl_PointSize = 1.0f;
	gl_Position = push.mvp * vec4(position, 1.0);
	pColor = color;
}