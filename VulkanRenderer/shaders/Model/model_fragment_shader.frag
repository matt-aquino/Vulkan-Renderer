#version 460

layout(location = 0) in vec3 viewDir;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 outNormal;
layout(location = 3) in vec2 outTexcoord;

layout(location = 0) out vec4 fragColor;

layout (set = 2, binding = 0) uniform Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 transmittance;
	vec3 emission;
	
	float shininess;			// specular exponent
	float ior;				    // index of refraction
	float dissolve;			    // 1 == opaque; 0 == fully transparent 
	int illum;					// illumination model
	float roughness;            // [0, 1] default 0
	float metallic;             // [0, 1] default 0
	float sheen;                // [0, 1] default 0
	float clearcoat_thickness;  // [0, 1] default 0
	float clearcoat_roughness;  // [0, 1] default 0
	float anisotropy;           // aniso. [0, 1] default 0
	float anisotropy_rotation;  // anisor. [0, 1] default 0
	float pad0;
	int dummy;					// Suppress padding warning.
} material;

layout(set = 2, binding = 2) uniform sampler2D textureSampler;


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