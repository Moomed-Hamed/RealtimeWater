#version 430 core

layout (location = 0) in vec4 position;
layout (location = 2) in vec4 tex_coord;

out vec4 tex;

void main()
{
	tex = tex_coord;
	gl_Position = position;
}