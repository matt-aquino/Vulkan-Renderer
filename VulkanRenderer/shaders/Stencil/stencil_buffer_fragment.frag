#version 460

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec2 texcoords;
layout(location = 2) in vec3 viewDir;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D diffuse;
layout(binding = 2) uniform sampler2D normal;

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
	vec3 norm = texture(normal, texcoords).rgb;
	norm = normalize(norm);

	// Phong Model
	vec3 albedo = texture(diffuse, texcoords).rgb;

	// ambient
	vec3 ambient = albedo * material.ambient;
	
	// diffuse
	vec3 lightDirection = normalize(lightPosition - fragPos.xyz);
	float d = max(dot(norm, lightDirection), 0.0f);
	vec3 diffuse = albedo * (d * material.diffuse);

	// specular
	vec3 reflection = reflect(-lightDirection, norm);
	float spec = pow(max(dot(viewDir, reflection), 0.0f), material.shininess);
	vec3 specular = albedo * lightColor * (spec * material.specular);

	vec3 lighting = ambient + diffuse + specular;

	fragColor = vec4(lighting, 1.0f);
}