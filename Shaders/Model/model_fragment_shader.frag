#version 450


layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform Material
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
} material;

void main()
{
	fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f); // dummy output
}