#version 460 core

// depth map from shadow pass
layout(set = 0, binding = 1) uniform sampler2D shadowMap;

// material data
layout(set = 2, binding = 0) uniform Material
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
} mat;

// data from vertex shader
layout (location = 0) in vec3 inFragPos;
layout (location = 1) in vec3 inFragNormal;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec4 inLightSpacePos;
layout (location = 4) in vec3 inLightPos;

// output color
layout(location = 0) out vec4 fragColor;

vec3 lightColor = vec3(1.0);

float calculate_shadow(vec4 fragPosLightSpace)
{
	float shadow = 0.0;

	// convert lightspace position to [-1,1]
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// depth from depth map is [0,1], so transform NDC coords to [0,1]
	projCoords = projCoords * 0.5 + 0.5;

	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;

	if (currentDepth > 1.0)
		return 0.0;

	// frag pos is greater than sampled depth value, meaning light is blocked
	if (currentDepth > closestDepth)
		shadow = 1.0;

	return shadow;
}

void main()
{
	vec3 lightDir = normalize(inLightPos - inFragPos);
	vec3 normal = normalize(inFragNormal);

	float diff = max(dot(normal, lightDir), 0.0);

	vec3 ambientLight = mat.ambient * lightColor;
	vec3 diffuseLight = mat.diffuse * lightColor * diff;

	float shadow = calculate_shadow(inLightSpacePos);

	fragColor = vec4(ambientLight + (1.0 - shadow) * diffuseLight, 1.0);
}