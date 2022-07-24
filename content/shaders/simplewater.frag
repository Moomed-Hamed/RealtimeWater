#version 430 core

in vec3 normal;

out vec4 FragColor;

void main()
{
	FragColor = vec4((normalize(normal) + 1.0) * 0.5, 1);
}