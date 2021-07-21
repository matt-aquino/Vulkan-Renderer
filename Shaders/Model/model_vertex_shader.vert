#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;

layout(push_constant) uniform PushConstants
{
	mat4 model;
	mat4 view;
	mat4 projection;

} pushConstants;

void main()
{
	gl_Position = pushConstants.projection * pushConstants.view * pushConstants.model * vec4(position, 1.0);
}