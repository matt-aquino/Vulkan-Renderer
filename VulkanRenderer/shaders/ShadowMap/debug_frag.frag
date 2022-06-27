#version 460 core

layout (binding = 0) uniform sampler2D inTexture;
layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 fragColor;

layout (push_constant) uniform Light
{
	float near;
	float far;
} light;


float LinearizeDepth(float depth)
{
	float n = light.near;
	float f = light.far;

    float z = depth;// * 2.0 - 1.0; // Back to NDC 
    return (2.0 * n) / (f + n - z * (f - n));
}

void main()
{
	vec2 uv = vec2(inUV.x, 1.0 - inUV.y);
	float depth = texture(inTexture, uv).r;
	fragColor = vec4(vec3(1.0 - LinearizeDepth(depth)), 1.0);
	//fragColor = vec4(vec3(depth), 1.0);
}