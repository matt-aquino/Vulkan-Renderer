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

layout(set = 2, binding = 1) uniform sampler2D  ambientTex;
layout(set = 2, binding = 2) uniform sampler2D  diffuseTex;
layout(set = 2, binding = 3) uniform sampler2D  specularTex;
layout(set = 2, binding = 4) uniform sampler2D  specularHighlightTex;
layout(set = 2, binding = 5) uniform sampler2D  normalTex;
layout(set = 2, binding = 6) uniform sampler2D  alphaTex;
layout(set = 2, binding = 7) uniform sampler2D  metallicTex;
layout(set = 2, binding = 8) uniform sampler2D  displacementTex;
layout(set = 2, binding = 9) uniform sampler2D  emissiveTex;
layout(set = 2, binding = 10) uniform sampler2D reflectionTex;
layout(set = 2, binding = 11) uniform sampler2D roughnessTex;
layout(set = 2, binding = 12) uniform sampler2D sheenTex;	

// data from vertex shader
layout(location = 0) in vec4 inSceneClipSpace;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inLightVec;
layout(location = 3) in vec2 inTexcoord;
layout(location = 4) out vec4 inLightSpacePos;

layout(constant_id = 0) const int debug = 0;

// output color
layout(location = 0) out vec4 fragColor;

vec3 lightColor = vec3(1.0);


float compute_shadow()
{
	// scale and perspective divide
	vec3 lightSpaceNDC = inLightSpacePos.xyz / inLightSpacePos.w; 
	
	// map to [0,1] range
	vec2 shadowCoord = lightSpaceNDC.xy * 0.5 + 0.5;
	//vec2 shadowCoord = lightSpaceNDC.xy;

	// check if current frag position is in shadow
	float closestDepth = texture(shadowMap, shadowCoord).r;
	float currentDepth = lightSpaceNDC.z;
	return (currentDepth > closestDepth) ? 1.0 : 0.0;
}

float textureProj(vec4 shadowCoord, vec2 offset)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + offset ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

void main()
{
	// base color
	float lambertian = max(dot(inNormal, inLightVec), 0.0);
	vec3 diffuse = texture(diffuseTex, inTexcoord).rgb;
	diffuse *= mat.diffuse * lambertian * lightColor;
	
	vec3 ambient = texture(ambientTex, inTexcoord).rgb * mat.ambient * lightColor;


	//float shadow = compute_shadow();
	float shadow = textureProj(inLightSpacePos / inLightSpacePos.w, vec2(0.0));

	fragColor = vec4((ambient + diffuse) * shadow, 1.0);

}