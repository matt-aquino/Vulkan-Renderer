#version 460 core

layout(location = 0) in vec4 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec4 aNormal;

layout(set = 0, binding = 0) uniform Scene
{
	mat4 proj;
	mat4 view;
	mat4 lightViewProj;
	vec4 lightPos;
} scene;

layout (set = 1, binding = 0) uniform Mesh
{
	mat4 model;
	mat4 normal;
} mesh;

// output for fragment shader
layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outFragNormal;
layout (location = 2) out vec3 outViewVec;
layout (location = 3) out vec4 outLightSpacePos;
layout (location = 4) out vec3 outLightPos;

void main()
{
	vec4 worldPos = mesh.model * aPos;
	vec4 worldNormal = mesh.normal * aNormal;

	vec4 sceneSpace = scene.proj * scene.view * worldPos;
	outLightSpacePos = scene.lightViewProj * worldPos;

	outFragPos = worldPos.xyz;
	outFragNormal = worldNormal.xyz;
	outLightPos = scene.lightPos.xyz;

	gl_Position = sceneSpace;
}