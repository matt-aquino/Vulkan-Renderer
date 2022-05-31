#version 460 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D shadowSampler;

layout(push_constant) uniform Depths
{
	float near;
	float far;
};

float linearizeDepth(float depth)
{
	return (2.0 * depth) / (far + near - depth * (far - near));
}

void main()
{
	vec2 uv = vec2(inUV.x, 1.0 - inUV.y); // flip y coord
	float depth = texture(shadowSampler, uv).r;
	fragColor = vec4(vec3(depth), 1.0);
	//fragColor = vec4(vec3(1.0 - linearizeDepth(depth)), 1.0);
	//fragColor = vec4(uv, 0.0, 1.0); return; // test if quad output is rendering
}