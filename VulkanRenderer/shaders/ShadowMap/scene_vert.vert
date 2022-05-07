#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexcoord;
layout(location = 2) in vec3 aNormal;

layout(set = 0, binding = 0) uniform Scene
{
	mat4 proj;
	mat4 view;
	mat4 lightViewProj;
	vec3 lightPos;
};

layout (set = 1, binding = 0) uniform Mesh
{
	mat4 model;
};

layout (constant_id = 0) const int debug = 0;

layout(location = 0) out vec4 outSceneClipSpace;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outLightVec;
layout(location = 3) out vec2 outTexcoord;
layout(location = 4) out vec4 outLightSpacePos;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );
	
const mat4 otherBias = mat4( 
	0.5, 0.0, 0.0, 0.5,
	0.0, 0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.5,
	0.0, 0.0, 0.0, 1.0 );

void main()
{
	vec4 worldPos = model * vec4(aPos, 1.0);
	vec4 sceneClipSpace = proj * view * worldPos; // from the camera's POV
	vec4 lightClipSpace = lightViewProj * worldPos;  // from the light's POV

	outSceneClipSpace = (debug == 0) ? sceneClipSpace : lightClipSpace;

	gl_Position = outSceneClipSpace;

	outNormal = normalize(aNormal); 
	outLightVec = normalize(lightPos - worldPos.xyz); 
	outTexcoord = aTexcoord;
	//outLightSpacePos = biasMat * lightClipSpace;
	outLightSpacePos = otherBias * lightClipSpace;
}