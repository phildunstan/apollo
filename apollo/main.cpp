
#include <cstdio>
#include <cassert>
#include <memory>
#include <tuple>

#include "GL/glew.h"
#include "SDL.h"
#include "SDL_main.h"
#include "SDL_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "scope_exit.h"
#include "gl_helpers.h"
#include "sdl_helpers.h"

using namespace std;

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;

float playerInputSensitivity = 2.0f;
Vector2 playerInput;

Vector3 playerPosition;

GLuint playerSpriteTexture = 0;

GLShader playerVertexShader;
GLShader playerFragmentShader;
GLProgram playerShaderProgram;

GLuint playerVertexBuffer = 0;
GLuint playerNumberVertices = 0;
GLuint playerIndexBuffer = 0;
GLuint playerNumberIndices = 0;


bool LoadResources(SDL_Renderer& renderer)
{
	const char* vertexShaderSource =
		R"!!!(
#version 300 es
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texcoord;
out vec2 v_texCoord;
uniform mat4 u_modelview;
uniform mat4 u_projection;
void main()
{
	gl_Position = u_projection * u_modelview * a_position;
	v_texCoord = a_texcoord;
}
)!!!";
	playerVertexShader = CreateShader(GL_VERTEX_SHADER, vertexShaderSource);
	if (playerVertexShader == 0)
		return false;


	const char* fragmentShaderSource =
		R"!!!(
#version 300 es
precision mediump float;
in vec2 v_texCoord;
layout(location = 0) out vec4 fragColor;
uniform sampler2D s_texture;
void main()
{
	fragColor = texture(s_texture, v_texCoord);
}
)!!!";
	playerFragmentShader = CreateShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
	if (playerFragmentShader == 0)
		return false;

	playerShaderProgram = CreateProgram(playerVertexShader, playerFragmentShader);
	if (playerShaderProgram == 0)
		return false;


	glGenBuffers(1, &playerVertexBuffer);
	if (playerVertexBuffer == 0)
	{
		printf("Unable to allocate player vertex buffer: %s\n", glewGetErrorString(glGetError()));
		return false;
	}


	unique_ptr<uint8_t> pixels;
	int w = 0;
	int h = 0;
	tie(pixels, w, h) = std::async(LoadSDLTexture, "apollo.png").get();

	glGenTextures(1, &playerSpriteTexture);
	glBindTexture(GL_TEXTURE_2D, playerSpriteTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	float minX = -w / 2.0f;
	float maxX =  w / 2.0f;
	float minY = -h / 2.0f;
	float maxY =  h / 2.0f;
	position_uv_vertex playerSpriteVertices[] = { {{ minX, minY, 0.0f}, {0.0f, 1.0f}},
												  {{ maxX, minY, 0.0f}, {1.0f, 1.0f}},
												  {{ maxX, maxY, 0.0f}, {1.0f, 0.0f}},
												  {{ minX, maxY, 0.0f}, {0.0f, 0.0f}}};
	playerNumberVertices = sizeof(playerSpriteVertices) / sizeof(position_uv_vertex);
	glBindBuffer(GL_ARRAY_BUFFER, playerVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, playerNumberVertices * sizeof(position_uv_vertex), playerSpriteVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenBuffers(1, &playerIndexBuffer);
	if (playerIndexBuffer == 0)
	{
		printf("Unable to allocate player index buffer: %s\n", glewGetErrorString(glGetError()));
		return false;
	}

	GLushort playerSpriteIndices[] = { 0, 1, 3, 3, 1, 2 };
	playerNumberIndices = sizeof(playerSpriteIndices) / sizeof(GLushort);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, playerIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, playerNumberIndices * sizeof(GLushort), playerSpriteIndices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	return CheckOpenGLErrors();
}

void UpdateWorld()
{
	playerPosition += Vector3(playerInput, 0.0f) * playerInputSensitivity;
}


void RenderWorld(SDL_Renderer& renderer)
{
	glClearColor(0, 0, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(playerShaderProgram);

	glBindBuffer(GL_ARRAY_BUFFER, playerVertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, playerIndexBuffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, uv));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, playerSpriteTexture);
	auto texUniform = glGetUniformLocation(playerShaderProgram, "s_texture");
	if (texUniform == -1)
	{
		printf("Unable to find uniform s_texture in player shader.\n");
		CheckOpenGLErrors();
	}
	glUniform1i(texUniform, 0);

	auto modelviewUniform = glGetUniformLocation(playerShaderProgram, "u_modelview");
	if (modelviewUniform == -1)
	{
		printf("Unable to find uniform u_modelview in player shader.\n");
		CheckOpenGLErrors();
	}
	auto modelviewMatrix = glm::translate(glm::mat4(), playerPosition);
	glUniformMatrix4fv(modelviewUniform, 1, GL_FALSE, glm::value_ptr(modelviewMatrix));

	auto projectionUniform = glGetUniformLocation(playerShaderProgram, "u_projection");
	if (projectionUniform == -1)
	{
		printf("Unable to find uniform u_projection in player shader.\n");
		CheckOpenGLErrors();
	}
	auto projectionMatrix = glm::ortho(-320.0f, 320.0f, -240.0f, 240.0f);
	glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glDrawElements(GL_TRIANGLES, playerNumberIndices, GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CheckOpenGLErrors();
}


int main(int argc, char** argv)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
	{
		printf("SDL init failed: %s\n", SDL_GetError());
		return 1;
	}
	auto sdlQuiter = make_scope_exit(SDL_Quit);

	auto sdlImageInitFlags = IMG_INIT_PNG;
	if (IMG_Init(sdlImageInitFlags) != sdlImageInitFlags)
	{
		printf("Failed to init SDL_Image: %s\n", IMG_GetError());
		return 1;
	}
	auto sdlImageQuiter = make_scope_exit(IMG_Quit);

	auto joystick = unique_ptr<SDL_Joystick, void(__cdecl *)(SDL_Joystick*)>(SDL_JoystickOpen(0), SDL_JoystickClose);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	int windowWidth = 640;
	int windowHeight = 480;
	auto window = unique_ptr<SDL_Window, void (__cdecl*)(SDL_Window*)>(SDL_CreateWindow("apollo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI), &SDL_DestroyWindow);
	if (!window)
	{
		printf("Unable to create SDL window: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	auto renderer = unique_ptr<SDL_Renderer, void (__cdecl*)(SDL_Renderer*)>(SDL_CreateRenderer(window.get(), -1, 0), SDL_DestroyRenderer);

	auto glContext = SDL_GL_CreateContext(window.get());
	auto glContextDeleter = make_scope_exit([&glContext] () { SDL_GL_DeleteContext(glContext); });

	glewInit();

	glViewport(0, 0, windowWidth, windowHeight);

	if (!LoadResources(*renderer))
	{
		return 1;
	}

	while (true)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				return 0;
				break;
			}
		}

		if (!joystick)
		{
			printf("Unable to open SDL Joystick: %s\n", SDL_GetError());
		}
		auto joystickX = SDL_JoystickGetAxis(joystick.get(), 0) / 32768.0f;
		auto joystickY = -SDL_JoystickGetAxis(joystick.get(), 1) / 32768.0f;
		playerInput = Vector2(joystickX, joystickY);

		UpdateWorld();

		RenderWorld(*renderer);
		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}