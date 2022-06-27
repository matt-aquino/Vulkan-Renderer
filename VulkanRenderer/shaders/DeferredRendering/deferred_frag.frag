#version 460 core


layout (set = 2, binding = 0) uniform Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 transmittance;
	vec3 emission;

	float shininess;		
	float ior;				   
	float dissolve;			   
	int illum;				
	float roughness;           
	float metallic;            
	float sheen;               
	float clearcoat_thickness; 
	float clearcoat_roughness; 
	float anisotropy;          
	float anisotropy_rotation; 
	float pad0;
	int dummy;	
} material;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (location = 0) out vec4 colorOutput;
layout (location = 1) out vec4 normalOutput;
layout (location = 2) out vec4 positionOutput;

void main()
{
	colorOutput = vec4(material.diffuse, 1.0);
	normalOutput = vec4(normalize(inNormal), 1.0);
	positionOutput = vec4(inPos, 1.0);
}