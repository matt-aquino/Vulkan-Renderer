#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aNormal;

layout(set = 0, binding = 0) uniform UBO
{
	mat4 proj;		 //	camera projection
	mat4 view;		 //	camera view 
	vec3 lightPos;	 //	point light position
	vec3 viewPos;	 //	camera position
};

layout(set = 1, binding = 0) uniform Mesh
{
	mat4 model;
	mat4 normal;
};

layout(location = 0) out vec3 outFragNormalWorld;
layout(location = 1) out vec3 outFragWorld;

void main()
{
	vec4 vertWorld = model * vec4(aPos, 1.0);
	outFragWorld = vertWorld.xyz;
	outFragNormalWorld = vec3(normal * vec4(aNormal, 0.0));

	gl_Position = proj * view * vertWorld;
}