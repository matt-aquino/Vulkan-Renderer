#version 460

struct Particle
{
	vec3 position;
	vec3 velocity;
	vec3 color;
};

layout(binding = 0) readonly buffer ParticlesBuffer
{
	Particle particles[];

} particlesBuffer;

layout(push_constant) uniform PushConstants
{
	mat4 mvp;
};

layout(location = 0) out vec3 pColor;
void main()
{
	gl_PointSize = 10.0f;
	gl_Position = mvp * vec4(particlesBuffer.particles[gl_BaseInstance].position, 1.0);
	pColor = particlesBuffer.particles[gl_BaseInstance].color;
}