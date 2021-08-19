#version 460

layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 norm;
layout(binding = 1) uniform sampler2D albedo;

// render targets
layout(location = 0) out vec4 position;
layout(location = 1) out vec4 color;
layout(location = 2) out vec4 normal;

void main()
{
	position = pos;
	normal = vec4(norm, 1.0f);
	color = texture(albedo, texcoord);
}