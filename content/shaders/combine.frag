#version 430 core

in vec2 tex_coord;

out vec4 FragColor;

layout(binding = 0) uniform sampler2D BackgroundColor;
layout(binding = 1) uniform sampler2D BackgroundDepth;
layout(binding = 2) uniform sampler2D WaterColor;
layout(binding = 3) uniform sampler2D WaterDepth;

void main()
{
	float water_depth      = texture2D(WaterDepth, tex_coord).r;
	float background_depth = texture2D(BackgroundDepth, tex_coord).r;
	
	if (water_depth < background_depth)
		FragColor = texture2D(WaterColor, tex_coord);
	else
		FragColor = texture2D(BackgroundColor, tex_coord);
}