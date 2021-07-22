#version 450

layout(location = 0) in vec3 viewDir;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 outNormal;
layout(location = 3) in vec2 outTexcoord;
layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
} material;

const vec3 lightPosition = vec3(0.0f, 1.0f, -5.0f);
const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);


void main()
{
	// Phong Model
	vec3 albedo = texture(textureSampler, outTexcoord).rgb;

	// ambient
	vec3 ambient = albedo * material.ambient;
	
	// diffuse
	vec3 lightDirection = normalize(lightPosition - fragPos);
	float d = max(dot(outNormal, lightDirection), 0.0f);
	vec3 diffuse = albedo * (d * material.diffuse);

	// specular
	vec3 reflection = reflect(-lightDirection, outNormal);
	float spec = pow(max(dot(viewDir, reflection), 0.0f), material.shininess);
	vec3 specular = albedo * lightColor * (spec * material.specular);

	vec3 lighting = ambient + diffuse + specular;

	fragColor = vec4(lighting, 1.0f);
}