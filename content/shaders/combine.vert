#version 430 core

layout (location = 0) in vec4 Position;
layout (location = 2) in vec2 TexCoord;

out vec2 tex_coord;

void main()
{
	tex_coord = TexCoord;
	gl_Position = Position;
}