#include "window.h"

#define DRAW_DISTANCE 1024.0f

// -------------------- Shaders -------------------- //

struct Shader { GLuint id; };

void load(Shader* shader, const char* vert_path, const char* frag_path)
{
	char* vert_source = (char*)read_text_file_into_memory(vert_path);
	char* frag_source = (char*)read_text_file_into_memory(frag_path);

	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vert_source, NULL);
	glCompileShader(vert_shader);

	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_source, NULL);
	glCompileShader(frag_shader);

	free(vert_source);
	free(frag_source);

	{
		GLint log_size = 0;
		glGetShaderiv(vert_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(vert_shader, log_size, NULL, error_log);
			out("VERTEX SHADER ERROR:\n" << error_log);
			free(error_log);
		}

		log_size = 0;
		glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(frag_shader, log_size, NULL, error_log);
			out("FRAGMENT SHADER ERROR:\n" << error_log);
			free(error_log);
		}
	}

	shader->id = glCreateProgram();
	glAttachShader(shader->id, vert_shader);
	glAttachShader(shader->id, frag_shader);
	glLinkProgram (shader->id);

	GLsizei length = 0;
	char error[256] = {};
	glGetProgramInfoLog(shader->id, 256, &length, error);
	if(length > 0) out(error);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
}
void bind(Shader shader)
{
	glUseProgram(shader.id);
}
void free(Shader shader)
{
	glDeleteShader(shader.id);
}

struct Compute_Shader { GLuint id; };

void load(Compute_Shader* shader, const char* path)
{
	char* source = (char*)read_text_file_into_memory(path);

	GLuint comp_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(comp_shader, 1, &source, NULL);
	glCompileShader(comp_shader);

	free(source);

	{
		GLint log_size = 0;
		glGetShaderiv(comp_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(comp_shader, log_size, NULL, error_log);
			out(path);
			out("COMPUTE SHADER ERROR:\n" << error_log);
			free(error_log);
		}
	}

	shader->id = glCreateProgram();
	glAttachShader(shader->id, comp_shader);
	glLinkProgram(shader->id);

	GLsizei length = 0;
	char error[256] = {};
	glGetProgramInfoLog(shader->id, 256, &length, error);
	if (length) out(error);

	glDeleteShader(comp_shader);
}
void bind(Compute_Shader shader)
{
	glUseProgram(shader.id);
}

struct Shader_Compute_Render { GLuint id; };

void load(Shader_Compute_Render* shader, const char* vert, const char* frag, const char* comp)
{
	char* vert_source = (char*)read_text_file_into_memory(vert);
	char* frag_source = (char*)read_text_file_into_memory(frag);
	char* comp_source = (char*)read_text_file_into_memory(comp);

	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vert_source, NULL);
	glCompileShader(vert_shader);

	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_source, NULL);
	glCompileShader(frag_shader);

	GLuint comp_shader = glCreateShader(GL_COMPUTE_SHADER);
	glShaderSource(comp_shader, 1, &comp_source, NULL);
	glCompileShader(comp_shader);

	free(vert_source);
	free(frag_source);
	free(comp_source);

	{
		GLint log_size = 0;
		glGetShaderiv(vert_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(vert_shader, log_size, NULL, error_log);
			out("VERTEX SHADER ERROR:\n" << error_log);
			free(error_log);
		}

		log_size = 0;
		glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(frag_shader, log_size, NULL, error_log);
			out("FRAGMENT SHADER ERROR:\n" << error_log);
			free(error_log);
		}

		log_size = 0;
		glGetShaderiv(comp_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(comp_shader, log_size, NULL, error_log);
			out("FRAGMENT SHADER ERROR:\n" << error_log);
			free(error_log);
		}
	}

	shader->id = glCreateProgram();
	glAttachShader(shader->id, vert_shader);
	glAttachShader(shader->id, comp_shader);
	glAttachShader(shader->id, frag_shader);
	glLinkProgram(shader->id);

	GLsizei length = 0;
	char error[256] = {};
	glGetProgramInfoLog(shader->id, 256, &length, error);
	if (length > 0) out(error);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	glDeleteShader(comp_shader);
}
void bind(Shader_Compute_Render shader)
{
	glUseProgram(shader.id);
}

// make sure you bind a shader *before* calling these!
void set_int  (Shader shader, const char* name, int value  )
{
	glUniform1i(glGetUniformLocation(shader.id, name), value);
}
void set_float(Shader shader, const char* name, float value)
{
	glUniform1f(glGetUniformLocation(shader.id, name), value);
}
void set_vec2 (Shader shader, const char* name, vec2 value)
{
	glUniform2f(glGetUniformLocation(shader.id, name), value.x, value.y);
}
void set_vec3 (Shader shader, const char* name, vec3 value )
{
	glUniform3f(glGetUniformLocation(shader.id, name), value.x, value.y, value.z);
}
void set_vec4 (Shader shader, const char* name, vec4 value)
{
	glUniform4f(glGetUniformLocation(shader.id, name), value.x, value.y, value.z, value.w);
}
void set_mat3 (Shader shader, const char* name, mat3 value)
{
	glUniformMatrix3fv(glGetUniformLocation(shader.id, name), 1, GL_FALSE, (float*)&value);
}
void set_mat4 (Shader shader, const char* name, mat4 value )
{
	glUniformMatrix4fv(glGetUniformLocation(shader.id, name), 1, GL_FALSE, (float*)&value);
}

void mesh_add_attrib_float(GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 1, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}
void mesh_add_attrib_vec2 (GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}
void mesh_add_attrib_vec3 (GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}
void mesh_add_attrib_mat3 (GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);

	++attrib_id;
	offset += sizeof(vec3);
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);

	++attrib_id;
	offset += sizeof(vec3);
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}

// -------------------- Textures ------------------- //

GLuint load_texture(const char* path)
{
	GLuint id = {};
	int width, height, num_channels;
	byte* image;

	stbi_set_flip_vertically_on_load(true);

	image = stbi_load(path, &width, &height, &num_channels, 0);
	if (image == NULL) out("ERROR : '" << path << "' NOT FOUND!");

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(image);

	return id;
}
GLuint load_texture_png(const char* path)
{
	GLuint id = {};
	int width, height, num_channels;
	byte* image;

	stbi_set_flip_vertically_on_load(false);

	image = stbi_load(path, &width, &height, &num_channels, 0);
	if (image == NULL) out("ERROR : '" << path << "' NOT FOUND!");

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(image);

	return id;
}
void bind_texture(GLuint texture, uint texture_unit = 0)
{
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(GL_TEXTURE_2D, texture);
}

// -------------------- 3D Camera ------------------ //

#define DIR_FORWARD	0
#define DIR_BACKWARD	1
#define DIR_LEFT	2
#define DIR_RIGHT	3

struct Camera
{
	vec3 position;
	vec3 front, right, up;
	float yaw, pitch;
	float trauma;
};

void camera_update_dir(Camera* camera, float dx, float dy, float dtime, float sensitivity = 0.003)
{
	// camera shake
	float trauma = camera->trauma;

	static uint offset = random_uint() % 16;

	if (camera->trauma > 1) camera->trauma = 1;
	if (camera->trauma > 0) camera->trauma -= dtime;
	else
	{
		camera->trauma = 0;
		offset = random_uint() % 16;
	}

	float p1 = ((perlin((trauma + offset + 0) * 1000) * 2) - 1) * trauma;
	float p2 = ((perlin((trauma + offset + 1) * 2000) * 2) - 1) * trauma;
	float p3 = ((perlin((trauma + offset + 2) * 3000) * 2) - 1) * trauma;

	float shake_yaw   = ToRadians(p1);
	float shake_pitch = ToRadians(p2);
	float shake_roll  = ToRadians(p3);

	camera->yaw   += (dx * sensitivity) / TWOPI;
	camera->pitch += (dy * sensitivity) / TWOPI;

	float yaw   = camera->yaw + shake_yaw;
	float pitch = camera->pitch + shake_pitch;

	// it feels a little different (better?) if we let the shake actually move the camera a little
	//camera->yaw   += shake_yaw;
	//camera->pitch += shake_pitch;

	// updating camera direction
	if (camera->pitch >  PI / 2.01) camera->pitch =  PI / 2.01;
	if (camera->pitch < -PI / 2.01) camera->pitch = -PI / 2.01;

	camera->front.y = sin(pitch);
	camera->front.x = cos(pitch) * cos(yaw);
	camera->front.z = cos(pitch) * sin(yaw);

	camera->front = normalize(camera->front);
	camera->right = normalize(cross(camera->front, vec3(0, 1, 0)));
	camera->up    = normalize(cross(camera->right, camera->front));

	mat3 roll = glm::rotate(shake_roll, camera->front);
	camera->up = roll * camera->up;
}
void camera_update_pos(Camera* camera, int direction, float distance)
{
	if (direction == DIR_FORWARD ) camera->position += camera->front * distance;
	if (direction == DIR_LEFT    ) camera->position -= camera->right * distance;
	if (direction == DIR_RIGHT   ) camera->position += camera->right * distance;
	if (direction == DIR_BACKWARD) camera->position -= camera->front * distance;
}

// -------------------- Other ------------------ //

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
	Position       = 0,
	Normal         = 1,
	TexCoord       = 2,
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