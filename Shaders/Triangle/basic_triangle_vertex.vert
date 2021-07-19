#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor; 

layout(push_constant) uniform matrices
{
	mat4 model;
	mat4 view;
	mat4 projection;
} pushConstants;

layout(location = 0) out vec3 vertexColor;

void main()
{
	gl_Position = pushConstants.projection * pushConstants.view * pushConstants.model * vec4(inPosition, 1.0);
	vertexColor = inColor;
}