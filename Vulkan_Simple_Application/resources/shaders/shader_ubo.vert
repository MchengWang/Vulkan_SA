#version 450

layout(location = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out fragColor;

void main()
{
	gl_Position = ubo.proj * ubo.model * ubo.view * vec4(inPosition, 1.0);
	fragColor = inColor;
}