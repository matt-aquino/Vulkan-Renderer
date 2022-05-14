#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aNormal;

layout(location = 0) out vec3 outLightPos;
layout(location = 1) out vec3 outViewPos;
layout(location = 2) out vec3 outFragPos;
layout(location = 3) out vec3 outNormal;

layout(set = 0, binding = 0) uniform UBO
{
	mat4 proj;
	mat4 view;
	vec3 lightPos;
	vec3 viewPos;
};

layout(set = 1, binding = 0) uniform Mesh
{
	mat4 model;
};

void main()
{
	outLightPos = lightPos;
	outViewPos = viewPos;
	//outNormal = transpose(inverse(mat3(model))) * aNormal;
	outNormal = aNormal;

	vec4 vertPos = model * vec4(aPos, 1.0);
	outFragPos = vertPos.xyz / vertPos.w;
	
	gl_Position = proj * view * vertPos;
}