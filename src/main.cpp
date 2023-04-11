#include "shader.h"

struct Timer
{
	double start, end;

	void begin_frame()
	{
		start = glfwGetTime();
	}
	double end_frame()
	{
		end = glfwGetTime();
		double elapsed = end - start;
		start = end;
		return elapsed;
	}
};

int main()
{
	Window   window = {};
	Mouse    mouse  = {};
	Keyboard keys   = {};

	init_window(&window, 1920, 1080);
	init_keyboard(&keys);

	Shader water_shader = {};
	load(&water_shader, "content/shaders/water.vert", "content/shaders/water.frag");

	Shader combine_shader = {};
	load(&combine_shader, "content/shaders/combine.vert", "content/shaders/combine.frag");

	Shader ground_shader = {};
	load(&ground_shader, "content/shaders/ground.vert", "content/shaders/ground.frag");

	Shader sky_shader = {};
	load(&sky_shader, "content/shaders/sky.vert", "content/shaders/sky.frag");

	Shader simple_water_shader = {};
	load(&simple_water_shader, "content/shaders/simplewater.vert", "content/shaders/simplewater.frag");

	Compute_Shader water_sim_comp = {};
	load(&water_sim_comp, "content/shaders/watersimulation.comp");

	Camera camera = { {0, .25, 0} };

	struct {
		GLuint AB;
		GLuint VAO;
	} quad;

	Vertex unit_quad[] {
		{ { -1, -1, 0 }, { 0, 1, 0 }, { 0, 0 } },
		{ {  1, -1, 0 }, { 0, 1, 0 }, { 1, 0 } },
		{ {  1,  1, 0 }, { 0, 1, 0 }, { 1, 1 } },

		{ { -1, -1, 0 }, { 0, 1, 0 }, { 0, 0 } },
		{ {  1,  1, 0 }, { 0, 1, 0 }, { 1, 1 } },
		{ { -1,  1, 0 }, { 0, 1, 0 }, { 0, 1 } },
	};

	// Array Buffer
	glGenBuffers(1, &quad.AB);
	glBindBuffer(GL_ARRAY_BUFFER, quad.AB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(unit_quad), unit_quad, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Vertex Array Object
	glGenVertexArrays(1, &quad.VAO);
	setAttribPointer(quad.VAO, Attrib::Position, quad.AB, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	setAttribPointer(quad.VAO, Attrib::TexCoord, quad.AB, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, texCoord));

	int framebuf_width, framebuf_height;
	glfwGetFramebufferSize(window.instance, &framebuf_width, &framebuf_height);
	mat4 proj = glm::perspectiveFov(45.0f, float(framebuf_width), float(framebuf_height), .001f, 100.f);
	vec2 framebufferSize = vec2{ framebuf_width, framebuf_height };

	ivec2 waterMapSize = ivec2{ 1024 };
	ivec2 topViewSize  = ivec2{ 1024 };

	// create framebuffers
	Framebuffer backgroundFramebuffer = make_framebuffer(framebuf_width, framebuf_height);
	Framebuffer waterFramebuffer      = make_framebuffer(framebuf_width, framebuf_height);
	Framebuffer waterMapFramebuffer   = make_framebuffer(waterMapSize.x, waterMapSize.y);
	Framebuffer topFramebuffer        = make_framebuffer(topViewSize.x, topViewSize.y);

	// textures
	GLuint noise_normal_tex = import_texture("content/textures/noise_normal.jpg");
	GLuint caustic_tex = import_texture("content/textures/caustic.png");
	GLuint debug_tex   = import_texture("content/textures/ground.png");
	GLuint noise_tex   = import_texture("content/textures/noise.png");
	GLuint subsurf_tex = create_subsurf_texture();
	GLuint skyCubemap  = createCubemap();

	vec3 light_pos  = { 0, 1, 0 };
	mat4 light_proj = glm::ortho(-1.0, 1.0, -1.0, 1.0);
	mat4 light_view = glm::lookAt(light_pos, { 0, -1, 0 }, { 0, 0, 1 });

	int terrainSize = 200;
	Mesh ground = {}; init(ground, terrainSize);
	Mesh water  = {}; init(water , terrainSize, true);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	Timer timer = {};
	timer.begin_frame();

	float dt = 1.f / 60;

	while (!glfwWindowShouldClose(window.instance))
	{
		update_window(window);
		update_mouse(&mouse, window);
		update_keyboard(&keys, window);

		if (keys.W.is_pressed) camera_update_pos(&camera, DIR_FORWARD , 1/80.f);
		if (keys.S.is_pressed) camera_update_pos(&camera, DIR_BACKWARD, 1/80.f);
		if (keys.A.is_pressed) camera_update_pos(&camera, DIR_LEFT    , 1/80.f);
		if (keys.D.is_pressed) camera_update_pos(&camera, DIR_RIGHT   , 1/80.f);

		camera_update_dir(&camera, mouse.dx, mouse.dy, dt);

		if (keys.ESC.is_pressed) break;

		static float water_timer = 0; water_timer += dt;

		{ // water simulation
			int dimension = water.mesh_size + 1;
			
			glUseProgram(water_sim_comp.id);

			glUniform1i(0, dimension  );
			glUniform1f(1, dt         );
			glUniform1f(2, water_timer);
			glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, noise_tex);

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, water.positions[0] );
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, water.positions[1] );
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, water.normals      );
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ground.positions[0]);

			glDispatchCompute(dimension, dimension, 1);
		}

		// ----- RENDER FUNCTION ---- //

		mat4 view = lookAt(camera.position, camera.position + camera.front, camera.up);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Render water-map
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, waterMapFramebuffer.id);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			bind(simple_water_shader);
			{
				set_vec4(simple_water_shader, "world_position", vec4(0));
				set_mat4(simple_water_shader, "proj_view"     , light_proj * light_view);

				glViewport(0, 0, waterMapSize.x, waterMapSize.y);
				render(water);
				glViewport(0, 0, int(framebufferSize.x), int(framebufferSize.y));
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		// Render top-view
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, topFramebuffer.id);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			bind(simple_water_shader);
			{
				set_vec4(simple_water_shader, "world_position", vec4(0));
				set_mat4(simple_water_shader, "proj_view", light_proj * light_view);

				glViewport(0, 0, topViewSize.x, topViewSize.y);
				render(ground);
				glViewport(0, 0, int(framebufferSize.x), int(framebufferSize.y));
			}

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		// render skybox
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, backgroundFramebuffer.id);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			bind(sky_shader);
			{
				set_mat4(sky_shader, "inverse_view_proj", glm::inverse(proj * view));

				glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, skyCubemap);

				glDisable(GL_DEPTH_TEST);
				glBindVertexArray(quad.VAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
				glEnable(GL_DEPTH_TEST);
			}

			// Render ground
			bind(ground_shader);
			{
				set_vec4  (ground_shader, "world_position"     , vec4(0) );
				set_mat4  (ground_shader, "proj_view"          , proj * view);
				set_mat3  (ground_shader, "NormalMatrix"       , mat3(1) );
				set_mat4  (ground_shader, "WaterMapProjectionMatrix" , light_proj);
				set_mat4  (ground_shader, "WaterMapViewMatrix" , light_view );
				set_float (ground_shader, "TextureScale"       , 24.0f      );
				set_float (ground_shader, "Time"               , water_timer);

				bind_texture(waterMapFramebuffer.depth, 0);
				bind_texture(waterMapFramebuffer.color, 1);
				bind_texture(debug_tex                , 2);
				bind_texture(noise_normal_tex         , 3);
				bind_texture(caustic_tex              , 4);
				glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_1D, subsurf_tex);

				render(ground);
			}
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		// Render water
		bind(water_shader);
		{
			set_vec4 (water_shader, "world_position"      , vec4(0)        );
			set_mat4 (water_shader, "ViewMatrix"          , view           );
			set_mat4 (water_shader, "ProjectionMatrix"    , proj           );
			set_mat3 (water_shader, "NormalMatrix"        , mat3(1.f)      );
			set_mat4 (water_shader, "TopViewMatrix"       , light_view     );
			set_mat4 (water_shader, "TopProjectionMatrix" , light_proj     );
			set_vec2 (water_shader, "FramebufferSize"     , framebufferSize);
			set_float(water_shader, "DeltaTime"           , dt             );
			set_float(water_shader, "Time"                , glfwGetTime()  );

			bind_texture(backgroundFramebuffer.color, 0);
			bind_texture(backgroundFramebuffer.depth, 1);
			bind_texture(topFramebuffer.depth , 2);
			bind_texture(noise_tex            , 3);
			bind_texture(noise_normal_tex     , 4);
			bind_texture(subsurf_tex          , 5);
			glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_CUBE_MAP, skyCubemap);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, waterFramebuffer.id);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glDisable(GL_CULL_FACE);
			render(water);
			glEnable(GL_CULL_FACE);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		}

		// Combine framebuffer
		bind(combine_shader);
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			bind_texture(backgroundFramebuffer.color, 0);
			bind_texture(backgroundFramebuffer.depth, 1);
			bind_texture(waterFramebuffer.color     , 2);
			bind_texture(waterFramebuffer.depth     , 3);

			glDisable(GL_DEPTH_TEST);

			glBindVertexArray(quad.VAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);
		}

		char title[16] = {};
		snprintf(title, 16, "%04f", 1.f / timer.end_frame());
		glfwSetWindowTitle(window.instance, title);
	}

	glfwTerminate();
	return 0;
}