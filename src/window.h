#pragma comment(lib, "winmm")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "external/GLEW/glew32s")
#pragma comment(lib, "external/GLFW/glfw3")

#define _CRT_SECURE_NO_WARNINGS // because printf is "too dangerous"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image.h"
#include "external/stb_image_write.h"

#define GLEW_STATIC
#include <external/GLEW\glew.h> // OpenGL functions
#include <external/GLFW\glfw3.h>// window & input

#include <Windows.h>
#include <fileapi.h>
#include <iostream>

#define out(val) std::cout << ' ' << val << '\n'
#define stop std::cin.get()
#define print printf
#define printvec(vec) printf("%f %f %f\n", vec.x, vec.y, vec.z)
#define Alloc(type, count) (type *)calloc(count, sizeof(type))

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#include "external/GLM/glm.hpp"
#include "external/GLM/gtc/noise.hpp"
#include "external/GLM/gtc/type_ptr.hpp"
#include "external/GLM/gtc/matrix_transform.hpp"
#include "external/GLM/gtx/transform.hpp"

#include <vector>

#define PI	  3.14159265359f
#define TWOPI 6.28318530718f

#define ToRadians(value) ( ((value) * PI) / 180.0f )

typedef unsigned int  uint;
typedef unsigned char byte;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;
using glm::ivec2;
using glm::ivec3;

#define BIT_NOISE_1 0xB5297A4D;
#define BIT_NOISE_2 0x68E31DA4;
#define BIT_NOISE_3 0x1B56C4E9;

uint random_uint()
{
	uint seed = __rdtsc();
	seed *= BIT_NOISE_1;
	seed *= seed; // helps avoid linearity
	seed ^= (seed >> 8);
	seed += BIT_NOISE_2;
	seed ^= (seed >> 8);
	seed *= BIT_NOISE_3;
	seed ^= (seed >> 8);
	return seed;
}
float noise_chance(uint n, uint seed = 0) // normalized
{
	n *= BIT_NOISE_1;
	n += seed;
	n ^= (n >> 8);
	n += BIT_NOISE_2;
	n ^= (n >> 8);
	n *= BIT_NOISE_3;
	n ^= (n >> 8);

	return (float)n / (float)UINT_MAX;
}
float lerp(float start, float end, float amount)
{
	return (start + amount * (end - start));
}
float perlin(float n)
{
	int x1 = (int)n;
	int x2 = x1 + 1;

	return lerp(noise_chance(x1), noise_chance(x2), n - (float)x1);
}
byte* read_text_file_into_memory(const char* path)
{
	DWORD BytesRead;
	HANDLE os_file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER size;
	GetFileSizeEx(os_file, &size);

	byte* memory = (byte*)calloc(size.QuadPart + 1, sizeof(byte));
	ReadFile(os_file, memory, size.QuadPart, &BytesRead, NULL);

	CloseHandle(os_file);

	return memory;
}

#define WINDOW_ERROR(str) out("WINDOW ERROR: " << str)

struct Window
{
	GLFWwindow* instance;
	uint screen_width, screen_height;
};

void init_window(Window* window, uint screen_width, uint screen_height, const char* window_name = "window")
{
	window->screen_width = screen_width;
	window->screen_height = screen_height;

	if (!glfwInit()) { WINDOW_ERROR("GLFW ERROR : could not init glfw"); stop; return; }
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	window->instance = glfwCreateWindow(screen_width, screen_height, window_name, NULL, NULL);
	if (!window->instance) { glfwTerminate(); WINDOW_ERROR("no window instance"); stop; return; }

	glfwMakeContextCurrent(window->instance);
	glfwSwapInterval(1); // disable v-sync

	//Capture the cursor
	glfwSetInputMode(window->instance, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
void update_window(Window window)
{
	glfwPollEvents(); // should maybe switch the order of these 2
	glfwSwapBuffers(window.instance);
}
void shutdown_window()
{
	glfwTerminate();
}

struct Button
{
	char is_pressed;
	char was_pressed;
	uint id;
};

struct Mouse
{
	double raw_x, raw_y;   // pixel coordinates
	double norm_x, norm_y; // normalized screen coordinates
	double dx, dy;  // pos change since last frame in pixels
	double norm_dx, norm_dy;

	Button right_button, left_button;
};

void update_mouse(Mouse* mouse, Window window)
{
	static Mouse prev_mouse = {};

	glfwGetCursorPos(window.instance, &mouse->raw_x, &mouse->raw_y);

	mouse->dx = mouse->raw_x - prev_mouse.raw_x; // what do these mean again?
	mouse->dy = prev_mouse.raw_y - mouse->raw_y;

	mouse->norm_dx = mouse->dx / window.screen_width;
	mouse->norm_dy = mouse->dy / window.screen_height;

	prev_mouse.raw_x = mouse->raw_x;
	prev_mouse.raw_y = mouse->raw_y;

	mouse->norm_x = ((uint)mouse->raw_x % window.screen_width) / (double)window.screen_width;
	mouse->norm_y = ((uint)mouse->raw_y % window.screen_height) / (double)window.screen_height;

	mouse->norm_y = 1 - mouse->norm_y;
	mouse->norm_x = (mouse->norm_x * 2) - 1;
	mouse->norm_y = (mouse->norm_y * 2) - 1;

	bool key_down = (glfwGetMouseButton(window.instance, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

	if (key_down)
	{
		mouse->right_button.was_pressed = (mouse->right_button.is_pressed == true);
		mouse->right_button.is_pressed = true;
	}
	else
	{
		mouse->right_button.was_pressed = (mouse->right_button.is_pressed == true);
		mouse->right_button.is_pressed = false;
	}

	key_down = (glfwGetMouseButton(window.instance, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

	if (key_down)
	{
		mouse->left_button.was_pressed = (mouse->left_button.is_pressed == true);
		mouse->left_button.is_pressed = true;
	}
	else
	{
		mouse->left_button.was_pressed = (mouse->left_button.is_pressed == true);
		mouse->left_button.is_pressed = false;
	}
}

// for selecting game objecs
vec3 get_mouse_world_dir(Mouse mouse, mat4 proj_view)
{
	proj_view = glm::inverse(proj_view); //what is an unproject matrix?

	vec4 ray_near = vec4(mouse.norm_x, mouse.norm_y, -1, 1); // near plane is z = -1
	vec4 ray_far = vec4(mouse.norm_x, mouse.norm_y, 0, 1);

	// these are actually using inverse(proj_view)
	ray_near = proj_view * ray_near; ray_near /= ray_near.w;
	ray_far = proj_view * ray_far;  ray_far /= ray_far.w;

	return glm::normalize(ray_far - ray_near);
}

#define NUM_KEYBOARD_BUTTONS 34 // update when adding keyboard buttons
struct Keyboard
{
	union
	{
		Button buttons[NUM_KEYBOARD_BUTTONS];

		struct
		{
			Button A, B, C, D, E, F, G, H;
			Button I, J, K, L, M, N, O, P;
			Button Q, R, S, T, U, V, W, X;
			Button Y, Z;

			Button ESC, SPACE;
			Button SHIFT, CTRL;
			Button UP, DOWN, LEFT, RIGHT;
		};
	};
};

void init_keyboard(Keyboard* keyboard)
{
	keyboard->A = { false, false, GLFW_KEY_A };
	keyboard->B = { false, false, GLFW_KEY_B };
	keyboard->C = { false, false, GLFW_KEY_C };
	keyboard->D = { false, false, GLFW_KEY_D };
	keyboard->E = { false, false, GLFW_KEY_E };
	keyboard->F = { false, false, GLFW_KEY_F };
	keyboard->G = { false, false, GLFW_KEY_G };
	keyboard->H = { false, false, GLFW_KEY_H };
	keyboard->I = { false, false, GLFW_KEY_I };
	keyboard->J = { false, false, GLFW_KEY_J };
	keyboard->K = { false, false, GLFW_KEY_K };
	keyboard->L = { false, false, GLFW_KEY_L };
	keyboard->M = { false, false, GLFW_KEY_M };
	keyboard->N = { false, false, GLFW_KEY_N };
	keyboard->O = { false, false, GLFW_KEY_O };
	keyboard->P = { false, false, GLFW_KEY_P };
	keyboard->Q = { false, false, GLFW_KEY_Q };
	keyboard->R = { false, false, GLFW_KEY_R };
	keyboard->S = { false, false, GLFW_KEY_S };
	keyboard->T = { false, false, GLFW_KEY_T };
	keyboard->U = { false, false, GLFW_KEY_U };
	keyboard->V = { false, false, GLFW_KEY_V };
	keyboard->W = { false, false, GLFW_KEY_W };
	keyboard->X = { false, false, GLFW_KEY_X };
	keyboard->Y = { false, false, GLFW_KEY_Y };
	keyboard->Z = { false, false, GLFW_KEY_Z };

	keyboard->ESC = { false, false, GLFW_KEY_ESCAPE };
	keyboard->SPACE = { false, false, GLFW_KEY_SPACE };
	keyboard->SHIFT = { false, false, GLFW_KEY_LEFT_SHIFT };
	keyboard->CTRL = { false, false, GLFW_KEY_LEFT_CONTROL };

	keyboard->UP = { false, false, GLFW_KEY_UP };
	keyboard->DOWN = { false, false, GLFW_KEY_DOWN };
	keyboard->LEFT = { false, false, GLFW_KEY_LEFT };
	keyboard->RIGHT = { false, false, GLFW_KEY_RIGHT };
}
void update_keyboard(Keyboard* keyboard, Window window)
{
	for (uint i = 0; i < NUM_KEYBOARD_BUTTONS; ++i)
	{
		bool key_down = (glfwGetKey(window.instance, keyboard->buttons[i].id) == GLFW_PRESS);

		if (key_down)
		{
			keyboard->buttons[i].was_pressed = (keyboard->buttons[i].is_pressed == true);
			keyboard->buttons[i].is_pressed = true;
		}
		else
		{
			keyboard->buttons[i].was_pressed = (keyboard->buttons[i].is_pressed == true);
			keyboard->buttons[i].is_pressed = false;
		}
	}
}
