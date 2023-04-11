#include "renderer.h"

void setAttribPointer(GLuint vertexArrayObject, GLuint location, GLuint buffer, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint offset) {
	assert((glIsBuffer(buffer) != GL_FALSE));

	GLint previousBuffer;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
	{
		GLint previousVAO;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);
		{
			glBindVertexArray(vertexArrayObject);
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glEnableVertexAttribArray(location);
			glVertexAttribPointer(location, size, type, normalized, stride, reinterpret_cast<void*>(offset));
		}
		glBindVertexArray(previousVAO);
	}
	glBindBuffer(GL_ARRAY_BUFFER, previousBuffer);
}

GLuint import_texture(const char* path)
{
	int width, height, n;
	byte* data = stbi_load(path, &width, &height, &n, 4);

	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateMipmap(GL_TEXTURE_2D);

	return id;
}
GLuint create_subsurf_texture()
{
	const int size = 3;
	struct { GLubyte r, g, b; } pixel_data[3];
	pixel_data[0] = { 2, 204, 147 };
	pixel_data[1] = { 2, 127, 199 };
	pixel_data[2] = { 1, 9  , 100 };
	
	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, id);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, size, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel_data);

	glBindTexture(GL_TEXTURE_1D, 0);

	return id;
}
GLuint createCubemap()
{
	GLuint id = {};
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	int w, h, c;
	byte* PositiveX = stbi_load("content/textures/sky_posx.jpg", &w, &h, &c, 4);
	byte* NegativeX = stbi_load("content/textures/sky_negx.jpg", &w, &h, &c, 4);
	byte* PositiveY = stbi_load("content/textures/sky_posy.jpg", &w, &h, &c, 4);
	byte* NegativeY = stbi_load("content/textures/sky_negy.jpg", &w, &h, &c, 4);
	byte* PositiveZ = stbi_load("content/textures/sky_posz.jpg", &w, &h, &c, 4);
	byte* NegativeZ = stbi_load("content/textures/sky_negz.jpg", &w, &h, &c, 4);

	auto n = 2048;

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, PositiveX);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, NegativeX);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, PositiveY);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, NegativeY);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, PositiveZ);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, n, n, 0, GL_RGBA, GL_UNSIGNED_BYTE, NegativeZ);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return id;
}

struct Framebuffer {
	GLuint color; // texture
	GLuint depth; // texture
	GLuint id;
};

Framebuffer make_framebuffer(const unsigned width, const unsigned height)
{
	Framebuffer framebuffer = {};

	glGenTextures(1, &framebuffer.color);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &framebuffer.depth);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer.depth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &framebuffer.id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.color, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT , GL_TEXTURE_2D, framebuffer.depth, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return framebuffer;
}

enum Attrib {
	Position = 0,
	Normal = 1,
	TexCoord = 2,
	VertexPosition = 3
};

struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 texCoord;
};

float heightFunction(vec2 coordinate)
{
	return 0 - coordinate.y;// sin(TWOPI * 8 * coordinate.x);
	float COORDINATE_STRETCH = 5.0f;
	float EFUNCTION_STRETCH = 10.0f;
	float EFUNCTION_WEIGHT = -0.7f;
	float NOISE_WEIGHT = 0.015f;
	float NOISE_STRETCH = 6.0f;
	float HEIGHT = 0.6f;

	auto correctedCoordinate = (vec2{ 1.0 } - coordinate) * COORDINATE_STRETCH - vec2{ 0.0f };
	auto efunction = (1.0f / sqrt(2.0f * PI)) * exp(-(1.0f / EFUNCTION_STRETCH) * correctedCoordinate.x * correctedCoordinate.x);

	float height = static_cast<float>(HEIGHT * (0.1 + EFUNCTION_WEIGHT * efunction + NOISE_WEIGHT * glm::simplex(coordinate * NOISE_STRETCH)));

	static float big = 0;
	if (height > big)
	{
		big = height;
		out(big);
	}

	return height;
};
void create_mesh(uint resolution, bool water, vec4* positions, vec4* normals, vec2* tex_coords, uint* indices)
{
	float DX = 0.1f / resolution; // offset for calculating normals

	uint i = 0;
	for (uint x = 0; x <= resolution; x++) {
	for (uint y = 0; y <= resolution; y++)
	{
		vec2 normalized_position = vec2(x, y) / float(resolution);

		// height of the vertex
		float height = water ? 0.f : heightFunction(normalized_position);
		vec3 position = { normalized_position.x, height, normalized_position.y };

		// normal vector
		vec2 dx_pos = normalized_position + vec2{ DX, 0  };
		vec2 dy_pos = normalized_position + vec2{ 0 , DX };

		float dx_height = water ? 0.f : heightFunction(dx_pos);
		float dy_height = water ? 0.f : heightFunction(dy_pos);

		vec3 dx_dir = vec3{ dx_pos.x, dx_height, dx_pos.y } - position;
		vec3 dy_dir = vec3{ dy_pos.x, dy_height, dy_pos.y } - position;

		vec3 normal = normalize(cross(dy_dir, dx_dir));
		vec2 texture_coordinate = normalized_position;

		positions  [i  ] = vec4(position, 0);
		normals    [i  ] = vec4(normal  , 0);
		tex_coords [i++] = texture_coordinate;
	} }

	// indices

	uint vertices_per_row = resolution + 1;

	i = 0;
	for (uint x = 0; x < resolution; x++) {
	for (uint y = 0; y < resolution; y++)
	{
		// Triangle 1
		indices[i++] = (y + 0) * vertices_per_row + (x + 0);
		indices[i++] = (y + 0) * vertices_per_row + (x + 1);
		indices[i++] = (y + 1) * vertices_per_row + (x + 1);

		// Triangle 2
		indices[i++] = (y + 0) * vertices_per_row + (x + 0);
		indices[i++] = (y + 1) * vertices_per_row + (x + 1);
		indices[i++] = (y + 1) * vertices_per_row + (x + 0);
	} }

}

struct Mesh
{
	GLuint positions[2];
	GLuint normals;
	GLuint tex_coords;
	GLuint elementArrayBuffer;
	GLuint VAO;
	GLuint num_indices;
	GLuint mesh_size;
};

void init(Mesh& mesh, uint resolution, bool water = false)
{
	uint num_vertices = (resolution + 1) * (resolution + 1);
	uint num_indices  = resolution * resolution * 6;

	vec4* positions  = Alloc(vec4, num_vertices);
	vec4* normals    = Alloc(vec4, num_vertices);
	vec2* tex_coords = Alloc(vec2, num_vertices);
	uint* indices    = Alloc(uint, num_indices);

	create_mesh(resolution, water, positions, normals, tex_coords, indices);

	mesh.num_indices = num_indices;
	mesh.mesh_size   = resolution;

	// Position Buffers (2 of them)
	glGenBuffers(2, mesh.positions);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.positions[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * num_vertices, positions, GL_DYNAMIC_COPY);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.positions[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * num_vertices, positions, GL_DYNAMIC_COPY);

	// Normal Buffer
	glGenBuffers(1, &mesh.normals);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * num_vertices, normals, GL_STATIC_DRAW);

	// TexCoord Buffer
	glGenBuffers(1, &mesh.tex_coords);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.tex_coords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * num_vertices, tex_coords, GL_STATIC_DRAW);

	// Element Buffer
	glGenBuffers(1, &mesh.elementArrayBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementArrayBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh.num_indices, indices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &mesh.VAO);

	setAttribPointer(mesh.VAO, Attrib::Position, mesh.positions[0], 3, GL_FLOAT, GL_FALSE, sizeof(vec4), 0);
	setAttribPointer(mesh.VAO, Attrib::Normal  , mesh.normals     , 3, GL_FLOAT, GL_TRUE , sizeof(vec4), 0);
	setAttribPointer(mesh.VAO, Attrib::TexCoord, mesh.tex_coords  , 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
}

void render(Mesh& mesh)
{
	static bool flip = false;
	if (flip = !flip)
		std::swap(mesh.positions[0], mesh.positions[1]);

	glBindVertexArray(mesh.VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementArrayBuffer);
	glDrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// what the fuck is this
//#define for0(VAR, MAX) for (std::remove_const<decltype(MAX)>::type VAR = 0; VAR < (MAX); VAR++)