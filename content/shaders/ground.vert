#version 430 core

layout (location = 0) in vec4 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoord;

out vec4 fWorldPosition;
out vec3 fNormal;
out vec2 fTexCoord;

layout(location = 0) uniform vec4 world_position;
layout(location = 1) uniform mat4 proj_view;
layout(location = 3) uniform mat3 NormalMatrix;

void main()
{
	fTexCoord = TexCoord;
	fNormal   = NormalMatrix * Normal;
	fWorldPosition = world_position + Position;
	gl_Position = proj_view * fWorldPosition;
}