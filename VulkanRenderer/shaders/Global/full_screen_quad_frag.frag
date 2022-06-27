#version 460 core

layout (binding = 0) uniform sampler2D uTexture;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 fragColor;

void main()
{
	fragColor = texture(uTexture, inUV);
}