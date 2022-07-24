#version 430 core

in vec4 tex;

out vec4 FragColor;

layout(location = 1) uniform mat4 inverse_view_proj;

layout(binding = 0) uniform samplerCube sky_cubemap;

void main()
{
	// view ray
	vec4 view_front = vec4(tex.xy * 2.0 - 1.0, -1.0, 1.0);
	vec4 view_back  = vec4(tex.xy * 2.0 - 1.0,  1.0, 1.0);
	
	// convert to world coordinates
	view_front = inverse_view_proj * view_front;
	view_back  = inverse_view_proj * view_back;
	
	// not sure what this is
	vec3 front = view_front.xyz / view_front.w;
	vec3 back  = view_back.xyz  / view_back.w;
	
	vec3 view_ray = normalize(front - back) * -1;
	
	FragColor = texture(sky_cubemap, view_ray);
}