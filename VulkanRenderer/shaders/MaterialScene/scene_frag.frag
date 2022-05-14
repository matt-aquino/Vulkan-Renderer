#version 460 core

layout(location = 0) in vec3 inLightPos;
layout(location = 1) in vec3 inViewPos;
layout(location = 2) in vec3 inFragPos;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

vec3 lightColor = vec3(1.0);

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

vec3 blinnPhong(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 diffuseColor, vec3 specularColor, float shininess)
{
	vec3 brdf = diffuseColor;
	vec3 halfway = normalize(viewDir + lightDir);
	float spec = max(dot(halfway, normal), 0.0);
	brdf += pow(spec, shininess) * specularColor;
	return brdf;
}

void main()
{
	vec3 lightDir = normalize(-inLightPos);
	vec3 viewDir = normalize(inViewPos - inFragPos);
	vec3 normal = normalize(inNormal);

	vec3 radiance = mat.ambient;
	float irradiance = max(dot(lightDir, normal), 0.0); // * lightIntensity?

	if (irradiance > 0.0)
	{
		vec3 brdf = blinnPhong(lightDir, viewDir, normal, mat.diffuse, mat.specular, mat.shininess);
		radiance += brdf * irradiance * lightColor;
	}

	//radiance = pow(radiance, vec3(1.0 / 2.2)); // gamma correction
	fragColor = vec4(radiance, 1.0);
}