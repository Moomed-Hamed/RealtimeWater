// ------------------------------------------------------------------------------------------------------
//
// Description: Watersiumulation using a compute shader
//
// Author: Henrik Patjens
// Date: 5.1.2015
//
// ------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------
// Default Libraries
// ------------------------------------------------------------------------------------------------------
#include <cstdlib>
#include <cstdio>
#include <cstdarg>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <numeric>
#include <set>
#include <streambuf>
#include <string>
#include <tuple>
#include <vector>

// ------------------------------------------------------------------------------------------------------
// External Libraries
// ------------------------------------------------------------------------------------------------------
#include "glew.h"
#include "glfw3.h"

#define GLM_FORCE_RADIANS
#include "glm.hpp"
#include "gtc/noise.hpp"
#include "gtc/type_ptr.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtx/transform.hpp"

#include "corona.h"

// ------------------------------------------------------------------------------------------------------
// Defines
// ------------------------------------------------------------------------------------------------------
#define for0(VAR, MAX) for (std::remove_const<decltype(MAX)>::type VAR = 0; VAR < (MAX); VAR++)
#define breakcase break; case
#define breakdefault break; default
#define intern static

const auto IDENTITY = glm::mat4(1.0f);

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

struct Attributes {
	enum {
		Position = 0,
		Normal = 1,
		TexCoord = 2,
		VertexPosition = 3
	};
};

struct Geometry {
	GLuint arrayBuffer;
	GLuint vertexArrayObject;
};

intern Geometry c_unitQuad;

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

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

void initialize() {
	// Unit Quad
	{
		Vertex unitQuadVertices[] {
			{ { -1, -1, 0 }, { 0, 1, 0 }, { 0, 0 } },
			{ {  1, -1, 0 }, { 0, 1, 0 }, { 1, 0 } },
			{ {  1,  1, 0 }, { 0, 1, 0 }, { 1, 1 } },

			{ { -1, -1, 0 }, { 0, 1, 0 }, { 0, 0 } },
			{ {  1,  1, 0 }, { 0, 1, 0 }, { 1, 1 } },
			{ { -1,  1, 0 }, { 0, 1, 0 }, { 0, 1 } },
		};

		// Array Buffer
		glGenBuffers(1, &c_unitQuad.arrayBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, c_unitQuad.arrayBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuadVertices), unitQuadVertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Vertex Array Object
		glGenVertexArrays(1, &c_unitQuad.vertexArrayObject);
		setAttribPointer(c_unitQuad.vertexArrayObject, Attributes::Position, c_unitQuad.arrayBuffer, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		setAttribPointer(c_unitQuad.vertexArrayObject, Attributes::TexCoord, c_unitQuad.arrayBuffer, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoord));
	}
}

void renderUnitQuad()  {
	glBindVertexArray(c_unitQuad.vertexArrayObject);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

struct Errors {
	enum { EverythingFine = 0, Fatal = 1 };
};

void quit(const char *format, ...) {
	va_list arg;
	int done;

	va_start(arg, format);
	done = fprintf(stderr, format, arg);
	va_end(arg);

#if defined(_DEBUG) && defined(_MSC_VER)
	__debugbreak;
#endif

	exit(Errors::Fatal);
}

enum class RenderMode { 
	Normal, 
	Background, 
	Water, 
	WaterMap, 
	TopView 
};

struct Direction { 
	enum { 
		Front, 
		Back, 
		Left, 
		Right, 
		Up, 
		Down, 
		DirectionCount 
	}; 
};

struct FramebufferData {
	GLuint colorTexture;
	GLuint depthTexture;
	GLuint id;
};

struct ProgramData {
	GLuint id;
	std::map<std::string, GLint> uniforms;
};


std::string loadFile(const std::string& filename)
{
	std::ifstream t(filename);
	std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
	return str;
}

void checkShaderLog(GLuint shaderId)
{
	GLint length;
	glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);

	if (length > 1)
	{
		std::vector<GLchar> logBuffer;
		logBuffer.resize(length);
		glGetShaderInfoLog(shaderId, length, &length, logBuffer.data());

		quit("Shaderlog: %s\n", logBuffer.data());
	}
}

void checkProgramLog(GLuint programId)
{
	GLint length;
	glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &length);

	if (length > 1)
	{
		std::vector<GLchar> logBuffer;
		logBuffer.resize(length);
		glGetProgramInfoLog(programId, length, &length, logBuffer.data());

		quit("Programlog: %s\n", logBuffer.data());
	}
}

GLuint createProgram(std::set<std::tuple<std::string, GLenum>> shaderConfiguration)
{
	auto programId = glCreateProgram();

	std::vector<GLuint> shaders;

	for (auto& shader : shaderConfiguration)
	{
		auto shaderFileName = std::get<0>(shader);
		auto shaderType = std::get<1>(shader);
		
		auto shaderSource = loadFile(shaderFileName);
		const char * shaderSourcePtr = shaderSource.c_str();

		auto shaderId = glCreateShader(shaderType);
		shaders.push_back(shaderId);

		glShaderSource(shaderId, 1, &shaderSourcePtr, nullptr);
		glCompileShader(shaderId);
		checkShaderLog(shaderId);

		glAttachShader(programId, shaderId);
	}

	glLinkProgram(programId);
	checkProgramLog(programId);

	for (auto& shader : shaders)
		glDeleteShader(shader);

	return programId;
}

struct WaterSimulationProgram {
	void initialize() {
		id = createProgram({
			std::make_tuple("content/Watersimulation/water.vert", GL_VERTEX_SHADER),
			std::make_tuple("content/Watersimulation/water.frag", GL_FRAGMENT_SHADER) 
		});

		auto addUniform = [&](const std::string& name){ return uniforms[name] = glGetUniformLocation(id, name.c_str()); };

		glUseProgram(id);

		// Uniforms
		glUniformMatrix4fv(addUniform("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
		glUniformMatrix4fv(addUniform("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
		glUniformMatrix4fv(addUniform("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
		glUniformMatrix3fv(addUniform("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4{1.0f}));
		addUniform("WaterSurface");
		addUniform("BackgroundColorTexture");
		addUniform("BackgroundDepthTexture");
		addUniform("FramebufferSize");
		addUniform("NoiseTexture");
		addUniform("NoiseNormalTexture");
		addUniform("SkyCubemap");
		addUniform("DeltaTime");
		addUniform("Time");
		addUniform("SubSurfaceScatteringTexture");
		addUniform("FoamTexture");
		addUniform("TopViewDepthTexture");
		addUniform("TopViewMatrix");
		addUniform("TopProjectionMatrix");

		glUseProgram(0);
	}

	void projectionMatrix(const glm::mat4& m) { glUniformMatrix4fv(uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(m)); }
	void worldMatrix(const glm::mat4& m) { glUniformMatrix4fv(uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(m)); }
	void viewMatrix(const glm::mat4& m) { glUniformMatrix4fv(uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(m)); }
	void normalMatrix(const glm::mat3& m) { glUniformMatrix3fv(uniforms.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(m)); }

	GLuint id;
	std::map<std::string, GLint> uniforms;	
};

struct {
	GLFWwindow* window;

	bool wireframe;
	bool normals;
	bool manualCamera;
	RenderMode renderMode;
	FramebufferData backgroundFramebuffer;
	FramebufferData waterFramebuffer;
	FramebufferData waterMapFramebuffer;
	FramebufferData topFramebuffer;
	glm::ivec2 topViewSize;
	glm::ivec2 waterMapSize;
	glm::mat4 topViewMatrix;
	glm::mat4 topProjectionMatrix;
	glm::mat4 projectionMatrix;

	glm::vec2 framebufferSize;

	GLuint debugTexture1;
	GLuint grassTexture;
	GLuint sandTexture;
	GLuint noiseTexture;
	GLuint noiseNormalTexture;
	GLuint causticTexture;
	GLuint subsurfaceScatteringTexture;

	GLuint skyCubemap;

	WaterSimulationProgram waterSimulationProgram;

	void initialize() {
		waterSimulationProgram.initialize();
	}

	glm::vec3 lightPos;
	glm::mat4 lightProjectionMatrix;
	glm::mat4 lightViewMatrix;

	glm::mat4 camera;
	glm::vec2 mousePosition;
	bool rightMouseButtonPressed;
	bool moveDirection[Direction::DirectionCount];
} appData;

std::vector<Vertex> createMeshVertices(const unsigned dimension, std::function<float(glm::vec2)> heightFunction)
{
	std::vector<Vertex> vertices;
	auto cellSize = 1.0f / dimension;
	for0(x, dimension + 1)
		for0(y, dimension + 1)
		{
			const auto NORMAL_EPSILON = cellSize / 10.0f;

			auto normalizedPosition = glm::vec2{ x * cellSize, y * cellSize }; // [0,1]
			auto epsilonPositionX = normalizedPosition + glm::vec2{ NORMAL_EPSILON, 0 };
			auto epsilonPositionY = normalizedPosition + glm::vec2{ 0, NORMAL_EPSILON };

			auto currentHeight = heightFunction(normalizedPosition);
			auto epsilonHeightX = heightFunction(epsilonPositionX);
			auto epsilonHeightY = heightFunction(epsilonPositionY);
			
			auto position = glm::vec3{ normalizedPosition.x, currentHeight, normalizedPosition.y };

			auto toEpsilonX = glm::vec3{ epsilonPositionX.x, epsilonHeightX, epsilonPositionX.y } - position;
			auto toEpsilonY = glm::vec3{ epsilonPositionY.x, epsilonHeightY, epsilonPositionY.y } -position;

			auto normal = glm::normalize(glm::cross(toEpsilonY, toEpsilonX));

			auto texCoord = normalizedPosition;
			vertices.push_back(Vertex{ position, normal, texCoord });
		}
	return vertices;
}

std::vector<GLuint> createMeshIndices(const unsigned dimension)
{
	std::vector<GLuint> indices;
	const GLuint verticesPerRow = dimension + 1;
	for0(x, dimension)
		for0(y, dimension)
		{
			// triangle 1
			indices.push_back(y * verticesPerRow + x);
			indices.push_back(y * verticesPerRow + (x + 1));
			indices.push_back((y + 1) * verticesPerRow + (x + 1));

			// triangle 2
			indices.push_back(y * verticesPerRow + x);
			indices.push_back((y + 1) * verticesPerRow + (x + 1));
			indices.push_back((y + 1) * verticesPerRow + x);

		}
	return indices;
}

class Terrain
{
public:
	Terrain(int width, int height, std::function<float(glm::vec2)> heightFunction)
		: m_width{ width }, m_height{height }
	{
		m_data.resize(width * height);

		// initialize with height function
		for0(x, width)
			for0(y, height)
				m_data[index(x, y)] = heightFunction(glm::vec2{ x / (float) m_width, y / (float) m_height });
	}

	int width() const { return m_width; }
	int height() const { return m_height; }
	float data(int x, int y) const { return m_data[index(x, y)]; }

private:
	std::vector<float> m_data;
	int m_width;
	int m_height;

	int index(int x, int y) const { return y*m_width + x; }
};

class Mesh
{
public:
	Mesh(const Terrain& terrain)
		: m_dimension{ terrain.width() }
	{
		auto dimension = terrain.width();

		auto vertices = createMeshVertices(dimension, [](glm::vec2 v){ return 0; });
		auto indices = createMeshIndices(dimension);

		auto vertexCount = (dimension + 1) * (dimension + 1);
		m_indexCount = dimension * dimension * 6;

		auto c = [&](int v){ return std::min<int>(v, dimension - 1); };

		for0(i, vertices.size())
		{
			auto x = i % (dimension + 1);
			auto y = i / (dimension + 1);
			vertices[i].position.y = terrain.data(c(x), c(y));
		}			

		for0(i, vertices.size())
		{
			auto x = i % (dimension + 1);
			auto y = i / (dimension + 1);
			auto index = [&](int x, int y){ return y*(dimension + 1) + x; };
			auto toPositiveX = vertices[index(std::min<int>(x + 1, dimension), y)].position - vertices[i].position;
			auto toPositiveY = vertices[index(x, std::min<int>(y + 1, dimension))].position - vertices[i].position;
			vertices[i].normal = x < dimension && y < dimension ? glm::normalize(glm::cross(toPositiveX, toPositiveY)) : glm::vec3{ 0, 1, 0 };
		}

		std::vector<glm::vec4> positionData;
		for0(i, vertices.size())
			positionData.push_back(glm::vec4{ vertices[i].position, 0.0 });

		std::vector<glm::vec4> normalData;
		for0(i, vertices.size())
			normalData.push_back(glm::vec4{ vertices[i].normal, 0.0 });

		std::vector<glm::vec2> texCoordData;
		for0(i, vertices.size())
			texCoordData.push_back(vertices[i].texCoord);

		// Position Buffer
		glGenBuffers(2, m_positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Normal Buffer
		glGenBuffers(1, &m_normalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexCount, normalData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// TexCoord Buffer
		glGenBuffers(1, &m_texCoordBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * vertexCount, texCoordData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Element Buffer
		glGenBuffers(1, &m_elementArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* m_indexCount, indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	Mesh(int dimension, std::function<float(glm::vec2)> heightFunction)
		: m_dimension{ dimension }
	{
		auto vertices = createMeshVertices(dimension, heightFunction);
		auto indices = createMeshIndices(dimension);

		auto vertexCount = (dimension + 1) * (dimension + 1);
		m_indexCount = dimension * dimension * 6;

		std::vector<glm::vec4> positionData;
		for0(i, vertices.size())
			positionData.push_back(glm::vec4{ vertices[i].position, 0.0 });

		std::vector<glm::vec4> normalData;
		for0(i, vertices.size())
			normalData.push_back(glm::vec4{ vertices[i].normal, 0.0f });

		std::vector<glm::vec2> texCoordData;
		for0(i, vertices.size())
			texCoordData.push_back(vertices[i].texCoord);

		// Position Buffer
		glGenBuffers(2, m_positionBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexCount, positionData.data(), GL_DYNAMIC_COPY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Normal Buffer
		glGenBuffers(1, &m_normalBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertexCount, normalData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// TexCoord Buffer
		glGenBuffers(1, &m_texCoordBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * vertexCount, texCoordData.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Element Buffer
		glGenBuffers(1, &m_elementArrayBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* m_indexCount, indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void render() const
	{		
		glBindBuffer(GL_ARRAY_BUFFER, m_positionBuffer[0]);
		glEnableVertexAttribArray(Attributes::Position);
		glVertexAttribPointer(Attributes::Position, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

		glBindBuffer(GL_ARRAY_BUFFER, m_normalBuffer);
		glEnableVertexAttribArray(Attributes::Normal);
		glVertexAttribPointer(Attributes::Normal, 3, GL_FLOAT, GL_TRUE, sizeof(glm::vec4), reinterpret_cast<void*>(sizeof(glm::vec4)));

		glBindBuffer(GL_ARRAY_BUFFER, m_texCoordBuffer);
		glEnableVertexAttribArray(Attributes::TexCoord);
		glVertexAttribPointer(Attributes::TexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), reinterpret_cast<void*>(sizeof(glm::vec4) * 2));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementArrayBuffer);

		glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glDisableVertexAttribArray(Attributes::Position);
		glDisableVertexAttribArray(Attributes::Normal);
		glDisableVertexAttribArray(Attributes::TexCoord);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	int dimension() const { return m_dimension; }

	GLuint positionBuffer1() const { return m_positionBuffer[0]; }
	GLuint positionBuffer2() const { return m_positionBuffer[1]; }
	GLuint normalBuffer() const { return m_normalBuffer; }
	void swapBuffer() { std::swap(m_positionBuffer[0], m_positionBuffer[1]); }

private:
	int m_dimension;
	GLuint m_positionBuffer[2];
	GLuint m_normalBuffer;
	GLuint m_texCoordBuffer;
	GLuint m_elementArrayBuffer;
	GLuint m_vertexArrayObject;
	unsigned m_indexCount;
};

struct perPixelLinkedLists
{
	GLuint headPointerTexture;
	GLuint clearHeadPointerBuffer;
	GLuint fragmentStorageAtomicCounter;
	GLuint fragmentStorageTexture;
	unsigned framebufferWidth;
	unsigned framebufferHeight;
};


FramebufferData createGeneralFramebuffer(const unsigned width, const unsigned height)
{
	FramebufferData framebufferData;

	glGenTextures(1, &framebufferData.colorTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebufferData.colorTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &framebufferData.depthTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebufferData.depthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &framebufferData.id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferData.id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferData.colorTexture, 0);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebufferData.depthTexture, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return framebufferData;
}

GLFWwindow* createContext() {
	if (!glfwInit()) {
		quit("Could not initialize GLFW.");
	}		

	GLFWwindow* window = glfwCreateWindow(1200, 800, "Hello World", NULL, NULL);
	if (!window) {
		glfwTerminate();
		quit("Could not create the window.");
	}

	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit()) {
		if (!GLEW_ARB_debug_output) {
			printf("Could not load ARB_debug_output!");
		}
		quit("Could not initialize GLEW.");
	}

	glfwSetKeyCallback(window, [](GLFWwindow*, int key, int, int action, int modifier) {
		if (action != GLFW_REPEAT) {
			switch (key) {
				breakcase GLFW_KEY_W: appData.moveDirection[Direction::Front] = action == GLFW_PRESS;
				breakcase GLFW_KEY_A: appData.moveDirection[Direction::Left] = action == GLFW_PRESS;
				breakcase GLFW_KEY_S: appData.moveDirection[Direction::Back] = action == GLFW_PRESS;
				breakcase GLFW_KEY_D: appData.moveDirection[Direction::Right] = action == GLFW_PRESS;
				breakcase GLFW_KEY_SPACE: appData.moveDirection[Direction::Up] = action == GLFW_PRESS;
			}
		}
		appData.moveDirection[Direction::Down] = modifier == GLFW_MOD_SHIFT;

		if (action != GLFW_PRESS) {
			return;
		}

		switch (key) {
			breakcase GLFW_KEY_F1: appData.wireframe = !appData.wireframe; printf("Wireframe: %s\n", appData.wireframe ? "on" : "off");
			breakcase GLFW_KEY_F2: appData.normals = !appData.normals; printf("Normals: %s\n", appData.normals ? "on" : "off");
			breakcase GLFW_KEY_F3: appData.manualCamera = !appData.manualCamera; printf("Camera: %s", appData.manualCamera ? "on" : "off");
			breakcase GLFW_KEY_F5: appData.renderMode = RenderMode::Normal;
			breakcase GLFW_KEY_F6: appData.renderMode = RenderMode::Background;
			breakcase GLFW_KEY_F7: appData.renderMode = RenderMode::WaterMap;
			breakcase GLFW_KEY_F8: appData.renderMode = RenderMode::Water;
			breakcase GLFW_KEY_F9: appData.renderMode = RenderMode::TopView;
			breakcase GLFW_KEY_ESCAPE: exit(0);
		}
	});

	glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) { 
		appData.rightMouseButtonPressed = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS; 
	});

	glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) { 
		appData.mousePosition = glm::vec2{ x, y };	
	});

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);

		glDeleteFramebuffers(1, &appData.backgroundFramebuffer.id);
		appData.backgroundFramebuffer = createGeneralFramebuffer(width, height);
		glDeleteFramebuffers(1, &appData.waterFramebuffer.id);
		appData.waterFramebuffer = createGeneralFramebuffer(width, height);

		appData.framebufferSize = glm::vec2{ width, height };
	});

	if (GLEW_ARB_debug_output) {
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
			if (type == GL_DEBUG_TYPE_ERROR_ARB) {
				quit("GLEW_ARB_debug_output: %s\n", message);
			}
		}, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
	}

	return window;
}

ProgramData createSimpleWaterProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/simplewater.vert", GL_VERTEX_SHADER),
		std::make_tuple("content/Watersimulation/simplewater.frag", GL_FRAGMENT_SHADER) });

	auto addUniform = [&](const std::string& name){ return programData.uniforms[name] = glGetUniformLocation(programData.id, name.c_str()); };

	glUseProgram(programData.id);

	// Uniforms
	glUniformMatrix4fv(addUniform("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix3fv(addUniform("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4{ 1.0f }));

	glUseProgram(0);

	return programData;
}

ProgramData createGroundProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/ground.vert", GL_VERTEX_SHADER),
		std::make_tuple("content/Watersimulation/ground.frag", GL_FRAGMENT_SHADER) });

	auto addUniform = [&](const std::string& name){ return programData.uniforms[name] = glGetUniformLocation(programData.id, name.c_str()); };

	glUseProgram(programData.id);

	// Uniforms
	glUniformMatrix4fv(addUniform("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix3fv(addUniform("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4{ 1.0f }));
	addUniform("LightPosition");
	addUniform("WaterMapDepth");
	addUniform("WaterMapNormals");
	addUniform("WaterMapProjectionMatrix");
	addUniform("WaterMapViewMatrix");
	addUniform("GrassTexture");
	addUniform("GrassTextureScale");
	addUniform("SandTexture");
	addUniform("SandTextureScale");
	addUniform("NoiseNormalTexture");
	addUniform("CausticTexture");
	addUniform("SubSurfaceScatteringTexture");
	addUniform("Time");

	glUseProgram(0);

	return programData;
}

ProgramData createTextureProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/texture.vert", GL_VERTEX_SHADER),
		std::make_tuple("content/Watersimulation/texture.frag", GL_FRAGMENT_SHADER) });

	auto addUniform = [&](const std::string& name){ return programData.uniforms[name] = glGetUniformLocation(programData.id, name.c_str()); };

	glUseProgram(programData.id);

	// Uniforms
	glUniformMatrix4fv(addUniform("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	addUniform("Texture");

	glUseProgram(0);

	return programData;
}

ProgramData createCombineProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/combine.vert", GL_VERTEX_SHADER),
		std::make_tuple("content/Watersimulation/combine.frag", GL_FRAGMENT_SHADER) });

	auto addUniform = [&](const std::string& name){ return programData.uniforms[name] = glGetUniformLocation(programData.id, name.c_str()); };

	glUseProgram(programData.id);

	// Uniforms
	addUniform("BackgroundTexture");
	addUniform("BackgroundDepthTexture");
	addUniform("WaterTexture");
	addUniform("WaterDepthTexture");

	glUseProgram(0);

	return programData;
}

ProgramData createNormalsProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/normals.vert", GL_VERTEX_SHADER),
		std::make_tuple("content/Watersimulation/normals.geom", GL_GEOMETRY_SHADER),
		std::make_tuple("content/Watersimulation/normals.frag", GL_FRAGMENT_SHADER) });

	auto addUniform = [&](const std::string& name){ return programData.uniforms[name] = glGetUniformLocation(programData.id, name.c_str()); };

	glUseProgram(programData.id);

	// Uniforms
	glUniformMatrix4fv(addUniform("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix3fv(addUniform("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat3{ 1.0f }));
	addUniform("NormalLength");
	addUniform("NormalColor");

	glUseProgram(0);

	return programData;
}

ProgramData createSkyProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/sky.vert", GL_VERTEX_SHADER),
		std::make_tuple("content/Watersimulation/sky.frag", GL_FRAGMENT_SHADER) });

	auto addUniform = [&](const std::string& name){ return programData.uniforms[name] = glGetUniformLocation(programData.id, name.c_str()); };

	glUseProgram(programData.id);

	// Uniforms
	glUniformMatrix4fv(addUniform("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(addUniform("InverseViewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	addUniform("SkyCubemap");

	glUseProgram(0);

	return programData;
}

ProgramData createWaterSimulationProgram()
{
	ProgramData programData;

	programData.id = createProgram({
		std::make_tuple("content/Watersimulation/watersimulation.comp", GL_COMPUTE_SHADER) });

	return programData;
}

GLuint createSubsurfaceScatteringTexture()
{
	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, id);

	struct pixel  { GLubyte r, g, b; };

	const int size = 3;
	std::vector<pixel> data;
	data.push_back(pixel{ 2, 204, 147 });
	data.push_back(pixel{ 2, 127, 199 });
	data.push_back(pixel{ 1, 9, 100 });

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());

	glBindTexture(GL_TEXTURE_1D, 0);

	return id;
}

GLuint createDebugTexture(int width, int height, int cellSize = 32)
{
	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);

	struct pixel  { GLubyte r, g, b, a; };

	std::vector<pixel> data;
	data.resize(sizeof(pixel) * width * height);
	for0(x, width)
		for0(y, height)
			data[y*width + x] = pixel
			{ 
				GLubyte(((x / cellSize) + (y / cellSize)) % 2 == 0 ? 0xFF : 0x00),
				GLubyte((((x / cellSize) + (y / cellSize)) % 2 == 1) && (y / cellSize) % 2 == 0 ? 0xFF : 0x00),
				GLubyte((((x / cellSize) + (y / cellSize)) % 2 == 1) && (x / cellSize) % 2 == 0 ? 0xFF : 0x00),
				GLubyte(1)
			};

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

corona::Image* importImage(const std::string& filename)
{
	auto getExtension = [&](){ return filename.substr(filename.find_last_of(".") + 1); };

	auto getFormat = [&]()
	{
		auto extension = getExtension();
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

		if (extension == "jpg" || extension == "jpeg")
			return corona::FileFormat::FF_JPEG;
		else if (extension == "bmp")
			return corona::FileFormat::FF_BMP;
		else if (extension == "gif")
			return corona::FileFormat::FF_GIF;
		else if (extension == "png")
			return corona::FileFormat::FF_PNG;
		else if (extension == "pcx")
			return corona::FileFormat::FF_PCX;
		else if (extension == "tga")
			return corona::FileFormat::FF_TGA;
		throw std::string{ "impossible" };
	};

	auto format = getFormat();

	auto imageOriginal = corona::OpenImage(filename.c_str(), format);
	auto imageConverted = corona::ConvertImage(imageOriginal, corona::PixelFormat::PF_R8G8B8A8);

	return imageConverted;
}

GLuint importTexture(const std::string& filename)
{
	auto imageConverted = importImage(filename);

	auto dataPtr = static_cast<char*>(imageConverted->getPixels());
	auto pixelCount = imageConverted->getHeight() * imageConverted->getWidth();

	std::vector<unsigned char> data{ dataPtr, dataPtr + pixelCount * 4 };

	auto width = static_cast<unsigned>(imageConverted->getWidth());
	auto height = static_cast<unsigned>(imageConverted->getHeight());

	GLuint id;
	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); // <---- test this

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // GL_REPEAT

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataPtr);

	//glGenerateMipmap(GL_TEXTURE_2D); // <---- test this

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

glm::mat3 from(const glm::mat4 m)
{	
	auto mptr = glm::value_ptr(m);
	float a[]
	{
		mptr[0], mptr[1], mptr[2],
		mptr[4], mptr[5], mptr[6],
		mptr[8], mptr[9], mptr[10]
	};
	return glm::make_mat3(a);
}

void render(const float deltaTime, const glm::mat4& projectionMatrix,
	const Mesh& waterMesh, const Mesh& groundMesh, 
	const ProgramData& simpleWaterProgram, const ProgramData& groundProgram, 
	const ProgramData& textureProgram, const ProgramData& combineProgram, 
	const ProgramData& normalsProgram, const ProgramData& skyProgram)
{
	const float CAMERA_DISTANCE = 0.8f;

	float time = float(glfwGetTime()) * 0.2f;
	auto x = cos(time) * CAMERA_DISTANCE;
	auto z = sin(time) * CAMERA_DISTANCE;

	glm::mat4 viewMatrix;
	if (appData.manualCamera)
		viewMatrix = glm::inverse(appData.camera);
	else
		viewMatrix = glm::lookAt(glm::vec3(x, 0.2f, z), glm::vec3(0, -0.2f, 0), glm::vec3(0, 1, 0));
	
	if (appData.wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	auto waterWorldMatrix = glm::translate(glm::scale(IDENTITY, glm::vec3{ 2.0f }), glm::vec3{ -0.5f, 0, -0.5f });
	auto waterNormalMatrix = from(glm::transpose(glm::inverse(waterWorldMatrix)));
	auto groundWorldMatrix = glm::translate(glm::scale(IDENTITY, glm::vec3{ 2.0f }), glm::vec3{ -0.5f, 0, -0.5f });
	auto groundNormalMatrix = from(glm::transpose(glm::inverse(groundWorldMatrix)));

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//
	// render water map
	//
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.waterMapFramebuffer.id);
	glClearColor(0.1f, 0.2f, 0.7f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(simpleWaterProgram.id);
	{
		glViewport(0, 0, appData.waterMapSize.x, appData.waterMapSize.y);
		glUniformMatrix4fv(simpleWaterProgram.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(waterWorldMatrix));
		glUniformMatrix4fv(simpleWaterProgram.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(appData.lightViewMatrix));
		glUniformMatrix4fv(simpleWaterProgram.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.lightProjectionMatrix));
		waterMesh.render();
		glViewport(0, 0, GLsizei(appData.framebufferSize.x), GLsizei(appData.framebufferSize.y));
	}
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	//
	// render top view
	//
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.topFramebuffer.id);
	glClearColor(0.1f, 0.2f, 0.7f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(simpleWaterProgram.id);
	{
		glViewport(0, 0, appData.topViewSize.x, appData.topViewSize.y);
		glUniformMatrix4fv(simpleWaterProgram.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
		glUniformMatrix4fv(simpleWaterProgram.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(appData.topViewMatrix));
		glUniformMatrix4fv(simpleWaterProgram.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.topProjectionMatrix));
		groundMesh.render();
		glViewport(0, 0, GLsizei(appData.framebufferSize.x), GLsizei(appData.framebufferSize.y));
	} 
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	//
	// render ground
	//
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.backgroundFramebuffer.id);
	glClearColor(0.1f, 0.5f, 0.2f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	glUseProgram(skyProgram.id);
	{
		auto inverseViewProjectionMatrix = glm::inverse(projectionMatrix * viewMatrix);
		glUniformMatrix4fv(skyProgram.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
		glUniformMatrix4fv(skyProgram.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(skyProgram.uniforms.at("InverseViewProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(inverseViewProjectionMatrix));
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, appData.skyCubemap);
		glUniform1i(skyProgram.uniforms.at("SkyCubemap"), 0);
		
		glDisable(GL_DEPTH_TEST);
		renderUnitQuad();
		glEnable(GL_DEPTH_TEST);
	}
	glUseProgram(groundProgram.id);
	{
		glUniformMatrix4fv(groundProgram.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(groundWorldMatrix));
		glUniformMatrix4fv(groundProgram.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
		glUniformMatrix4fv(groundProgram.uniforms.at("WaterMapProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.lightProjectionMatrix));
		glUniformMatrix4fv(groundProgram.uniforms.at("WaterMapViewMatrix"), 1, GL_FALSE, glm::value_ptr(appData.lightViewMatrix));
		glUniform3f(groundProgram.uniforms.at("LightPosition"), appData.lightPos.x, appData.lightPos.y, appData.lightPos.z);
		glUniform1f(groundProgram.uniforms.at("Time"), time);
		

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, appData.waterMapFramebuffer.depthTexture);
		glUniform1i(groundProgram.uniforms.at("WaterMapDepth"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, appData.waterMapFramebuffer.colorTexture);
		glUniform1i(groundProgram.uniforms.at("WaterMapNormals"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, appData.grassTexture);
		glUniform1i(groundProgram.uniforms.at("GrassTexture"), 2);
		glUniform1f(groundProgram.uniforms.at("GrassTextureScale"), 24.0f);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, appData.sandTexture);
		glUniform1i(groundProgram.uniforms.at("SandTexture"), 3);
		glUniform1f(groundProgram.uniforms.at("SandTextureScale"), 36.0f);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, appData.noiseNormalTexture);
		glUniform1i(groundProgram.uniforms.at("NoiseNormalTexture"), 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, appData.causticTexture);
		glUniform1i(groundProgram.uniforms.at("CausticTexture"), 5);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_1D, appData.subsurfaceScatteringTexture);
		glUniform1i(groundProgram.uniforms.at("SubSurfaceScatteringTexture"), 6);

		glActiveTexture(GL_TEXTURE0);
		
		groundMesh.render();
	}
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	
	//
	// render water
	//
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, appData.waterFramebuffer.id);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(appData.waterSimulationProgram.id);
	{	
		glUniform2f(appData.waterSimulationProgram.uniforms.at("FramebufferSize"), appData.framebufferSize.x, appData.framebufferSize.y);
		appData.waterSimulationProgram.worldMatrix(waterWorldMatrix);
		appData.waterSimulationProgram.normalMatrix(waterNormalMatrix);
		appData.waterSimulationProgram.viewMatrix(viewMatrix);
		glUniformMatrix4fv(appData.waterSimulationProgram.uniforms.at("TopViewMatrix"), 1, GL_FALSE, glm::value_ptr(appData.topViewMatrix));
		glUniformMatrix4fv(appData.waterSimulationProgram.uniforms.at("TopProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.topProjectionMatrix));

		glUniform1f(appData.waterSimulationProgram.uniforms.at("DeltaTime"), deltaTime);
		glUniform1f(appData.waterSimulationProgram.uniforms.at("Time"), (float)glfwGetTime());		

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, appData.backgroundFramebuffer.colorTexture);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("BackgroundColorTexture"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, appData.backgroundFramebuffer.depthTexture);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("BackgroundDepthTexture"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, appData.noiseTexture);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("NoiseTexture"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, appData.noiseNormalTexture);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("NoiseNormalTexture"), 3);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, appData.skyCubemap);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("SkyCubemap"), 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_1D, appData.subsurfaceScatteringTexture);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("SubSurfaceScatteringTexture"), 5);

		glActiveTexture(GL_TEXTURE6);
		// Empty

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, appData.topFramebuffer.depthTexture);
		glUniform1i(appData.waterSimulationProgram.uniforms.at("TopViewDepthTexture"), 7);

		glActiveTexture(GL_TEXTURE0);

		glDisable(GL_CULL_FACE);
		waterMesh.render();
		glEnable(GL_CULL_FACE);
	}
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);


	//
	//	combine Framebuffer
	//
	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUseProgram(combineProgram.id);
	{
		glClearColor(0.5f, 0.5f, 0.2f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, appData.backgroundFramebuffer.colorTexture);
		glUniform1i(combineProgram.uniforms.at("BackgroundTexture"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, appData.backgroundFramebuffer.depthTexture);
		glUniform1i(combineProgram.uniforms.at("BackgroundDepthTexture"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, appData.waterFramebuffer.colorTexture);
		glUniform1i(combineProgram.uniforms.at("WaterTexture"), 2);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, appData.waterFramebuffer.depthTexture);
		glUniform1i(combineProgram.uniforms.at("WaterDepthTexture"), 3);

		renderUnitQuad();
	}
	glUseProgram(0);


	if (appData.renderMode != RenderMode::Normal)
	{
		//
		//	render debug
		//
		glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUseProgram(textureProgram.id);
		{
			glClearColor(0.5f, 0.5f, 0.2f, 1.0f);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			switch (appData.renderMode)
			{
			case RenderMode::Background:
				glBindTexture(GL_TEXTURE_2D, appData.backgroundFramebuffer.colorTexture);
				break;
			case RenderMode::WaterMap:
				glBindTexture(GL_TEXTURE_2D, appData.waterMapFramebuffer.colorTexture);
				break;
			case RenderMode::Water:
				glBindTexture(GL_TEXTURE_2D, appData.waterFramebuffer.colorTexture);
				break;
			case RenderMode::TopView:
				glBindTexture(GL_TEXTURE_2D, appData.topFramebuffer.colorTexture);
				break;
			default:
				glBindTexture(GL_TEXTURE_2D, appData.waterFramebuffer.colorTexture);
			}

			glUniform1i(textureProgram.uniforms.at("Texture"), 0);

			renderUnitQuad();
		}
		glUseProgram(0);
	}

	if (appData.normals)
	{
		glUseProgram(normalsProgram.id);
		{
			glUniformMatrix4fv(normalsProgram.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));

			glUniformMatrix4fv(normalsProgram.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(waterWorldMatrix));
			glUniformMatrix3fv(normalsProgram.uniforms.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(waterNormalMatrix));
			glUniform4f(normalsProgram.uniforms.at("NormalColor"), 0, 0, 1, 1);
			waterMesh.render();

			glUniformMatrix4fv(normalsProgram.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(groundWorldMatrix));
			glUniformMatrix3fv(normalsProgram.uniforms.at("NormalMatrix"), 1, GL_FALSE, glm::value_ptr(groundNormalMatrix));
			glUniform4f(normalsProgram.uniforms.at("NormalColor"), 0, 1, 0, 1);
			groundMesh.render();
		}
		glUseProgram(0);
	}
}

class FpsCounter
{
public:
	int frame()
	{
		double currentTime = glfwGetTime();
		m_frames++;
		if (currentTime - m_lastTime >= 1.0)
		{
			m_lastFps = m_frames;
			m_frames = 0;
			m_lastTime += 1.0;
		}
		return m_lastFps;
	}

private:
	int m_frames = 0;
	int m_lastFps = 0;
	double m_lastTime = glfwGetTime();
};

GLuint createCubemap()
{
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	auto imagePositiveX = importImage("content/Watersimulation/sky_posx.jpg");
	auto imageNegativeX = importImage("content/Watersimulation/sky_negx.jpg");
	auto imagePositiveY = importImage("content/Watersimulation/sky_posy.jpg");
	auto imageNegativeY = importImage("content/Watersimulation/sky_negy.jpg");
	auto imagePositiveZ = importImage("content/Watersimulation/sky_posz.jpg");
	auto imageNegativeZ = importImage("content/Watersimulation/sky_negz.jpg");

	auto size = 2048;

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePositiveX->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageNegativeX->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePositiveY->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageNegativeY->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagePositiveZ->getPixels());
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageNegativeZ->getPixels());

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return id;
}

void update() {
	float cameraSpeed = 0.01f;

	if (appData.moveDirection[Direction::Front]) appData.camera = glm::translate(appData.camera, glm::vec3{ 0, 0, -1 } * cameraSpeed);
	if (appData.moveDirection[Direction::Back])  appData.camera = glm::translate(appData.camera, glm::vec3{ 0, 0, +1 } * cameraSpeed);
	if (appData.moveDirection[Direction::Right]) appData.camera = glm::translate(appData.camera, glm::vec3{ +1, 0, 0 } * cameraSpeed);
	if (appData.moveDirection[Direction::Left])  appData.camera = glm::translate(appData.camera, glm::vec3{ -1, 0, 0 } * cameraSpeed);
	if (appData.moveDirection[Direction::Up])	 appData.camera = glm::translate(appData.camera, glm::vec3{ 0, +1, 0 } * cameraSpeed);
	if (appData.moveDirection[Direction::Down])  appData.camera = glm::translate(appData.camera, glm::vec3{ 0, -1, 0 } * cameraSpeed);

	if (appData.manualCamera)
	{
		auto middle = glm::vec2{ appData.framebufferSize.x / 2.0f, appData.framebufferSize.y / 2.0f };
		auto offset = middle - appData.mousePosition;

		const auto rotationSpeed = 0.0001f;

		if (appData.rightMouseButtonPressed)
		{
			auto toVec3 = [](glm::vec4 v){ return glm::vec3{ v.x / v.w, v.y / v.w, v.z / v.w }; };

			auto normalMatrix = glm::transpose(glm::inverse(appData.camera));
			auto yAxisInWorld = glm::inverse(normalMatrix) * glm::vec4{ 0, 1, 0, 1 };
			auto yAxisInWorldWDiv = toVec3(yAxisInWorld);
			
			auto xAxis = glm::vec3{ 1, 0, 0 };

			appData.camera = glm::rotate(appData.camera, float(offset.y * rotationSpeed), xAxis);
			appData.camera = glm::rotate(appData.camera, float(offset.x * rotationSpeed), yAxisInWorldWDiv);
			
		}

		glfwSetCursorPos(appData.window, middle.x, middle.y);
		appData.mousePosition = middle;
	}
}

void simulateWater(const Mesh& waterMesh, const Mesh& terrain, const ProgramData& waterSimulationProgram, const float deltaTime, const float time)
{
	glUseProgram(waterSimulationProgram.id);

	auto vertexDimension = waterMesh.dimension() + 1;

	glUniform1i(0, vertexDimension);
	glBindMultiTextureEXT(GL_TEXTURE0, GL_TEXTURE_2D, appData.noiseTexture);
	glUniform1i(1, 0);
	glUniform1f(2, deltaTime);
	glUniform1f(3, time);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, waterMesh.positionBuffer1());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, waterMesh.positionBuffer2());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, waterMesh.normalBuffer());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, terrain.positionBuffer1());

	glDispatchCompute(vertexDimension, vertexDimension, 1);

	glUseProgram(0);
}

int main(int argc, char* argv[]) {
	appData.window = createContext();

	appData.initialize();

	initialize();

	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(appData.window, &framebufferWidth, &framebufferHeight);
	appData.projectionMatrix = glm::perspectiveFov(45.0f, float(framebufferWidth), float(framebufferHeight), 0.001f, 100.0f);
	appData.framebufferSize = glm::vec2{ framebufferWidth, framebufferHeight };

	// camera
	appData.camera = IDENTITY;
	appData.topViewMatrix = glm::lookAt(glm::vec3{ 0.5f, 0.5f, 0.5f }, glm::vec3{ 0.5f, 0.0f, 0.5f }, glm::vec3{ 1, 0, 0 });
	appData.topProjectionMatrix = glm::ortho(-0.5, 0.5, -0.5, 0.5);

	// set up light
	appData.lightPos = glm::vec3{ -1, 1, -1 };
	appData.lightProjectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.01f, 5.0f);
	appData.lightViewMatrix = glm::lookAt(appData.lightPos, glm::vec3{ 0 }, glm::vec3{ 0, 1, 0 });

	// textures
	appData.debugTexture1 = createDebugTexture(512, 512);
	appData.grassTexture = importTexture("content/Watersimulation/grassTexture.jpg");
	appData.sandTexture = importTexture("content/Watersimulation/sandTexture.jpg");
	appData.noiseTexture = importTexture("content/Watersimulation/noiseTexture.png");
	appData.noiseNormalTexture = importTexture("content/Watersimulation/noiseNormalTexture.jpg");
	appData.causticTexture = importTexture("content/Watersimulation/causticTexture.png");
	appData.subsurfaceScatteringTexture = createSubsurfaceScatteringTexture();

	// cubemap
	appData.skyCubemap = createCubemap();

	// create shader programs
	auto waterSimulationProgram = createWaterSimulationProgram();
	auto combineProgramData = createCombineProgram();
	auto normalsProgramData = createNormalsProgram();
	glUseProgram(normalsProgramData.id);
	glUniformMatrix4fv(normalsProgramData.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.projectionMatrix));
	glUniform1f(normalsProgramData.uniforms.at("NormalLength"), 0.01f);	
	glUseProgram(0);
	auto textureProgramData = createTextureProgram();
	glUseProgram(textureProgramData.id);
	glUniformMatrix4fv(textureProgramData.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(textureProgramData.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUniformMatrix4fv(textureProgramData.uniforms.at("WorldMatrix"), 1, GL_FALSE, glm::value_ptr(IDENTITY));
	glUseProgram(0);

	glUseProgram(appData.waterSimulationProgram.id);
	appData.waterSimulationProgram.projectionMatrix(appData.projectionMatrix);	
	glUseProgram(0);

	auto simpleWaterProgramData = createSimpleWaterProgram();
	glUseProgram(simpleWaterProgramData.id);
	glUniformMatrix4fv(simpleWaterProgramData.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.lightProjectionMatrix));
	glUniformMatrix4fv(simpleWaterProgramData.uniforms.at("ViewMatrix"), 1, GL_FALSE, glm::value_ptr(appData.lightViewMatrix));
	glUseProgram(0);
	auto groundProgramData = createGroundProgram();
	glUseProgram(groundProgramData.id);
	glUniformMatrix4fv(groundProgramData.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.projectionMatrix));
	glUseProgram(0);
	auto skyProgramData = createSkyProgram();
	glUseProgram(skyProgramData.id);
	glUniformMatrix4fv(skyProgramData.uniforms.at("ProjectionMatrix"), 1, GL_FALSE, glm::value_ptr(appData.projectionMatrix));
	glUseProgram(0);

	appData.waterMapSize = glm::ivec2{ 1024 };
	appData.topViewSize = glm::ivec2{ 1024 };

	// create framebuffer
	appData.backgroundFramebuffer = createGeneralFramebuffer(framebufferWidth, framebufferHeight);
	appData.waterFramebuffer = createGeneralFramebuffer(framebufferWidth, framebufferHeight);
	appData.waterMapFramebuffer = createGeneralFramebuffer(appData.waterMapSize.x, appData.waterMapSize.y);
	appData.topFramebuffer = createGeneralFramebuffer(appData.topViewSize.x, appData.topViewSize.y);

	int terrainSize = 200;

	auto groundHeightFunction = [](glm::vec2 coordinate) {
		float COORDINATE_STRETCH = 5.0f;
		float EFUNCTION_STRETCH = 10.0f;
		float EFUNCTION_WEIGHT = -0.7f;
		float NOISE_WEIGHT = 0.015;
		float NOISE_STRETCH = 6.0f;
		float HEIGHT = 0.6f;

		auto correctedCoordinate = (glm::vec2{ 1.0 } - coordinate) * COORDINATE_STRETCH - glm::vec2{ 0.0f };
		auto efunction = (1.0f / sqrt(2.0f * M_PI)) * exp(-(1.0f / EFUNCTION_STRETCH) * correctedCoordinate.x * correctedCoordinate.x);

		return static_cast<float>(HEIGHT * (0.1 + EFUNCTION_WEIGHT * efunction + NOISE_WEIGHT * glm::simplex(coordinate * NOISE_STRETCH)));
	};
	Terrain terrain(terrainSize, terrainSize, groundHeightFunction);
	Mesh groundMesh(terrain);

	Mesh waterMesh(terrainSize, [](glm::vec2 coordinate) { return 0; });

	FpsCounter fpsCounter;

	glClearColor(0, 0.5f, 0.5f, 1);
	glClearDepth(1);
	glEnable(GL_DEPTH_TEST);

	float lastFrameTime = 0;
	
	while (!glfwWindowShouldClose(appData.window)) {
		glfwSetWindowTitle(appData.window, std::to_string(fpsCounter.frame()).c_str());

		update();

		auto time = (float)glfwGetTime();
		auto timeDelta = time - lastFrameTime;
		lastFrameTime = time;
		simulateWater(waterMesh, groundMesh, waterSimulationProgram, timeDelta, time);

		// render the scene
		render(timeDelta, appData.projectionMatrix, waterMesh, groundMesh, simpleWaterProgramData, groundProgramData, textureProgramData, combineProgramData, normalsProgramData, skyProgramData);

		waterMesh.swapBuffer();

		//glfwSwapInterval(0);
		glfwSwapBuffers(appData.window);
		glfwPollEvents();
	}

	glfwTerminate();

	std::cin.get();

	return 0;
}