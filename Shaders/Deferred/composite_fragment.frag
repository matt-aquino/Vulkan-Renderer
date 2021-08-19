#version 460 

layout(location = 1) in vec2 texcoord;

struct Light
{
	vec3 position;
	vec3 color;
	vec3 direction;
	float intensity;
	int type;
};

layout(binding = 0) buffer Lights
{
	Light[] lights;
};

uniform sampler2D positions;
uniform sampler2D colors;
uniform sampler2D normals;

vec4 calcPointLight(int lightIndex, vec4 fragPos, vec4 fragNormal, vec4 color)
{
	vec4 outColor = vec4(0.0f);
	float attenuation = 0.0f;
	vec3 lightPos = lights[lightIndex].position;
	vec3 lightColor = lights[lightIndex].color;
	float lightIntensity = lights[lightIndex].intensity;


	outColor.a = 1.0f;
	return outColor;
}

vec4 calcDirectionalLight(int lightIndex, vec4 fragPos, vec4 fragNormal, vec4 color)
{
	vec4 outColor = vec4(0.0f);
	vec3 lightColor = lights[lightIndex].color;
	vec3 lightDirection = lights[lightIndex].direction;
	float lightIntensity = lights[lightIndex].intensity;


	outColor.a = 1.0f;
	return outColor;
}


layout(location = 0) out vec4 fragColor;
void main()
{
	vec4 position = texture(positions, texcoord);
	vec4 normal = texture(normals, texcoord);
	vec4 color = texture(colors, texcoord);

	for (int i = 0; i < lights.length; i++)
	{
		if (lights[i].type == 0)
			fragColor += calcPointLight(i, position, normal, color);

		else if (lights[i].type == 1)
			fragColor += calcDirectionalLight(i, position, normal, color);
	}

}