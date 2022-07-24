#version 430 core

layout (location = 0) in vec4 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoord;

out vec3 normal;

layout(location = 0) uniform vec4 world_position;
layout(location = 1) uniform mat4 proj_view;

void main() {
	normal = Normal;
	gl_Position = proj_view * (world_position + Position);
}