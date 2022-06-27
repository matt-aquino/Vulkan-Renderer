#version 460 core

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D colorTexture;
layout (binding = 1) uniform sampler2D normalsTexture;
layout (binding = 2) uniform sampler2D positionsTexture;

layout (location = 0) out vec4 fragColor;

struct SceneLight
{
	vec4 position;
	vec4 color;
	float radius;
	float intensity;
};

layout (binding = 3) uniform Lights
{
	SceneLight lights[100];
};

void main()
{
	vec3 albedo = texture(colorTexture, inUV).rgb;
	vec3 fragNormal = normalize(texture(normalsTexture, inUV).xyz);
	vec3 fragPos = texture(positionsTexture, inUV).xyz;

	vec3 outputColor = albedo * 0.1; // ambient light
	for (int i = 0; i < 100; i++)
	{
		vec3 lightDir = lights[i].position.xyz - fragPos;
		float dist = length(lightDir);
		lightDir = normalize(lightDir);
		
		// only illuminate if within a light's radius
		if (dist < lights[i].radius)
		{
			float attenuation = lights[i].radius / (pow(dist, 2.0) + 1.0);
			float lambert = max(0.0, dot(fragNormal, lightDir));

			vec3 diffuseLight = (lights[i].color.rgb * lights[i].intensity) * albedo * lambert * attenuation;
			outputColor += diffuseLight;
		}
	}

	fragColor = vec4(outputColor, 1.0);
}