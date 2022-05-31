#version 460 core

layout(location = 0) in vec3 inFragWorldNormal;
layout(location = 1) in vec3 inFragWorld;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform UBO
{
	mat4 proj;
	mat4 view;
	vec3 lightPos;
	vec3 viewPos;
};

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

void main()
{
	vec3 lightDir = lightPos - inFragWorld;
	vec3 view = normalize(viewPos - inFragWorld);
	vec3 normal = normalize(inFragWorldNormal);

	float attenuation = 1.0f / dot(lightDir, lightDir);
	lightDir = normalize(lightDir); // normalize after attenuation so values are correct

	// diffuse
	float lambertian = max(dot(lightDir, normal), 0.0);
	vec3 lightIntensity = lightColor * attenuation;
	vec3 diffuseLight = lightIntensity * lambertian;


	// blinn phong
	vec3 halfAngle = normalize(lightDir + view);
	float blinn = pow(max(dot(normal, halfAngle), 0.0), mat.shininess);
	vec3 specularLight = lightIntensity * blinn * mat.specular;


	fragColor = vec4(mat.ambient + (diffuseLight * mat.diffuse) + specularLight, 1.0);
}