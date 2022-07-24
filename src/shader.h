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
	float COORDINATE_STRETCH = 5.0f;
	float EFUNCTION_STRETCH = 10.0f;
	float EFUNCTION_WEIGHT = -0.7f;
	float NOISE_WEIGHT = 0.015f;
	float NOISE_STRETCH = 6.0f;
	float HEIGHT = 0.6f;

	auto correctedCoordinate = (vec2{ 1.0 } - coordinate) * COORDINATE_STRETCH - vec2{ 0.0f };
	auto efunction = (1.0f / sqrt(2.0f * PI)) * exp(-(1.0f / EFUNCTION_STRETCH) * correctedCoordinate.x * correctedCoordinate.x);

	return static_cast<float>(HEIGHT * (0.1 + EFUNCTION_WEIGHT * efunction + NOISE_WEIGHT * glm::simplex(coordinate * NOISE_STRETCH)));
};
std::vector<Vertex> createMeshVertices(unsigned dimension, bool water)
{
	std::vector<Vertex> vertices;
	unsigned dimension1 = dimension + 1;
	for(int x = 0; x < dimension1; x++) {
	for(int y = 0; y < dimension1; y++)
	{
		auto normalizedPosition = vec2(x / float(dimension), y / float(dimension));

		const auto NORMAL_EPSILON = 0.1f / dimension;
		auto epsilonPositionX = normalizedPosition + vec2{ NORMAL_EPSILON, 0 };
		auto epsilonPositionY = normalizedPosition + vec2{ 0, NORMAL_EPSILON };

		auto currentHeight  = water ? 0.f : heightFunction(normalizedPosition);
		auto epsilonHeightX = water ? 0.f : heightFunction(epsilonPositionX);
		auto epsilonHeightY = water ? 0.f : heightFunction(epsilonPositionY);

		auto position = vec3{ normalizedPosition.x, currentHeight, normalizedPosition.y };

		auto toEpsilonX = vec3{ epsilonPositionX.x, epsilonHeightX, epsilonPositionX.y } - position;
		auto toEpsilonY = vec3{ epsilonPositionY.x, epsilonHeightY, epsilonPositionY.y } - position;

		auto normal = glm::normalize(glm::cross(toEpsilonY, toEpsilonX));

		auto texCoord = normalizedPosition;
		vertices.push_back(Vertex{ position, normal, texCoord });
	} }
	return vertices;
}
std::vector<GLuint> createMeshIndices(unsigned dimension)
{
	std::vector<GLuint> indices;
	unsigned verticesPerRow = dimension + 1;
	for(int x = 0; x < dimension; x++) {
	for(int y = 0; y < dimension; y++)
	{
		// Triangle 1
		indices.push_back(y * verticesPerRow + x);
		indices.push_back(y * verticesPerRow + (x + 1));
		indices.push_back((y + 1) * verticesPerRow + (x + 1));

		// Triangle 2
		indices.push_back(y * verticesPerRow + x);
		indices.push_back((y + 1) * verticesPerRow + (x + 1));
		indices.push_back((y + 1) * verticesPerRow + x);
	} }

	return indices;
}

struct Mesh
{
	GLuint positions[2];
	GLuint normals;
	GLuint tex_coords;
	GLuint elementArrayBuffer;
	GLuint VAO[2];
	GLuint num_indices;
	GLuint mesh_size;
};

void init(Mesh& mesh, int size, bool water = false)
{
	auto vertices = createMeshVertices(size, water);
	auto indices = createMeshIndices(size);

	auto vertexCount = (size + 1) * (size + 1);
	mesh.num_indices = size * size * 6;
	mesh.mesh_size = size;

	std::vector<vec4> positionData;
	for (int i = 0; i < vertices.size(); i++)
	{
		positionData.push_back(vec4{ vertices[i].position, 0.0 });
	}

	std::vector<vec4> normalData;
	for (int i = 0; i < vertices.size(); i++)
	{
		normalData.push_back(vec4{ vertices[i].normal, 0.0 });
	}

	std::vector<vec2> texCoordData;
	for (int i = 0; i < vertices.size(); i++)
	{
		texCoordData.push_back(vertices[i].texCoord);
	}

	// Position Buffer
	glGenBuffers(2, mesh.positions);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.positions[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.positions[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Normal Buffer
	glGenBuffers(1, &mesh.normals);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.normals);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * vertexCount, normalData.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// TexCoord Buffer
	glGenBuffers(1, &mesh.tex_coords);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.tex_coords);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * vertexCount, texCoordData.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Element Buffer
	glGenBuffers(1, &mesh.elementArrayBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementArrayBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh.num_indices, indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(2, mesh.VAO);

	// VAO 1
	setAttribPointer(mesh.VAO[0], Attrib::Position, mesh.positions[0], 3, GL_FLOAT, GL_FALSE, sizeof(vec4), 0);
	setAttribPointer(mesh.VAO[0], Attrib::Normal  , mesh.normals     , 3, GL_FLOAT, GL_TRUE , sizeof(vec4), 0);
	setAttribPointer(mesh.VAO[0], Attrib::TexCoord, mesh.tex_coords  , 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);

	// VAO 2
	setAttribPointer(mesh.VAO[1], Attrib::Position, mesh.positions[1], 3, GL_FLOAT, GL_FALSE, sizeof(vec4), 0);
	setAttribPointer(mesh.VAO[1], Attrib::Normal  , mesh.normals     , 3, GL_FLOAT, GL_TRUE , sizeof(vec4), 0);
	setAttribPointer(mesh.VAO[1], Attrib::TexCoord, mesh.tex_coords  , 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
}

void swap_buffers(Mesh& mesh)
{
	std::swap(mesh.positions[0], mesh.positions[1]);
	std::swap(mesh.VAO[0], mesh.VAO[1]);
}

void render(Mesh& mesh)
{
	glBindVertexArray(mesh.VAO[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.elementArrayBuffer);
	glDrawElements(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, nullptr);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// what the fuck is this
//#define for0(VAR, MAX) for (std::remove_const<decltype(MAX)>::type VAR = 0; VAR < (MAX); VAR++)