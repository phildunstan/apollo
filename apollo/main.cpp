
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



GLProgram playerShaderProgram;
struct Sprite
{
	GLuint texture = 0;
	GLuint vertexBuffer = 0;
	GLuint vertexCount = 0;
	GLuint indexBuffer = 0;
	GLuint indexCount = 0;
};


Sprite CreateSprite(const std::string& spriteFilename)
{
	Sprite sprite;

	unique_ptr<uint8_t[]> pixels;
	int w = 0;
	int h = 0;
	tie(pixels, w, h) = std::async(LoadSDLTexture, spriteFilename.c_str()).get();
	glGenTextures(1, &sprite.texture);
	glBindTexture(GL_TEXTURE_2D, sprite.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenBuffers(1, &sprite.vertexBuffer);
	if (sprite.vertexBuffer == 0)
	{
		printf("Unable to allocate sprite vertex buffer: %s\n", glewGetErrorString(glGetError()));
		return Sprite {};
	}
	float minX = -w / 2.0f;
	float maxX = w / 2.0f;
	float minY = -h / 2.0f;
	float maxY = h / 2.0f;
	position_uv_vertex vertices[] =
	{ { { minX, minY, 0.0f }, { 0.0f, 1.0f } },
	  { { maxX, minY, 0.0f }, { 1.0f, 1.0f } },
	  { { maxX, maxY, 0.0f }, { 1.0f, 0.0f } },
	  { { minX, maxY, 0.0f }, { 0.0f, 0.0f } } };
	sprite.vertexCount = sizeof(vertices) / sizeof(position_uv_vertex);
	glBindBuffer(GL_ARRAY_BUFFER, sprite.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sprite.vertexCount * sizeof(position_uv_vertex), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenBuffers(1, &sprite.indexBuffer);
	if (sprite.indexBuffer == 0)
	{
		printf("Unable to allocate sprite index buffer: %s\n", glewGetErrorString(glGetError()));
		return Sprite {};
	}
	GLushort indices[] = { 0, 1, 3, 3, 1, 2 };
	sprite.indexCount = sizeof(indices) / sizeof(GLushort);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sprite.indexCount * sizeof(GLushort), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CheckOpenGLErrors();

	return sprite;
}


void DrawSprite(const Sprite& sprite, const glm::mat4& modelviewMatrix, const glm::mat4& projectionMatrix)
{
	glBindBuffer(GL_ARRAY_BUFFER, sprite.vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sprite.indexBuffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(position_uv_vertex), (void*)offsetof(position_uv_vertex, uv));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sprite.texture);
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
	glUniformMatrix4fv(modelviewUniform, 1, GL_FALSE, glm::value_ptr(modelviewMatrix));

	auto projectionUniform = glGetUniformLocation(playerShaderProgram, "u_projection");
	if (projectionUniform == -1)
	{
		printf("Unable to find uniform u_projection in player shader.\n");
		CheckOpenGLErrors();
	}
	glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glDrawElements(GL_TRIANGLES, sprite.indexCount, GL_UNSIGNED_SHORT, 0);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CheckOpenGLErrors();
}



float playerInputSensitivity = 2.0f;
struct PlayerInput
{
	Vector2 movement { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
	bool firing { false };
};
PlayerInput playerInput;


struct RigidBody
{
	Vector3 position { 0.0f, 0.0f, 0.0f };
	Vector3 facing { 0.0f, 1.0f, 0.0f };
};
RigidBody player;
vector<RigidBody> bullets;

Sprite playerSprite;
Sprite bulletSprite;





bool LoadResources(SDL_Renderer& renderer)
{
	playerShaderProgram = LoadShaders("sprite_vs.glsl", "sprite_fs.glsl");
	if (playerShaderProgram == 0)
		return false;

	playerSprite = CreateSprite("apollo.png");
	bulletSprite = CreateSprite("bullet.png");

	return CheckOpenGLErrors();
}


void InitWorld()
{
	bullets.reserve(100);
}



void Fire()
{
	//if (bullets.size() == bullets.capacity())
	//{
	//	// don't allow the vector to reallocate
	//	// reuse the oldest bullet
	//}
	//else
	{
		RigidBody bullet = player;
		bullets.push_back(bullet);
	}
}


void UpdateWorld()
{
	player.position += Vector3(playerInput.movement, 0.0f) * playerInputSensitivity;
	if (playerInput.facing.length() > 0.5f)
	{
		auto normalizedInput = glm::normalize(playerInput.facing);
		player.facing = Vector3(normalizedInput, 0.0f);
	}

	if (playerInput.firing)
	{
		Fire();
	}

	for (auto& bullet : bullets)
	{
		const float frameTime = 1.0f / 60.0f;
		const float bulletSpeed = 1200.0f * frameTime;
		bullet.position += bulletSpeed * bullet.facing;
	}
}


void RenderWorld(SDL_Renderer& renderer)
{
	glClearColor(0, 0, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(playerShaderProgram);

	auto projectionMatrix = glm::ortho(-320.0f, 320.0f, -240.0f, 240.0f);

	for (const auto& bullet : bullets)
	{
		auto modelviewMatrix = glm::mat4();
		modelviewMatrix = glm::translate(modelviewMatrix, bullet.position);
		modelviewMatrix = glm::rotate(modelviewMatrix, atan2f(-bullet.facing.x, bullet.facing.y), glm::vec3(0.0f, 0.0f, 1.0f));

		DrawSprite(bulletSprite, modelviewMatrix, projectionMatrix);
	}

	{
		auto modelviewMatrix = glm::mat4();
		modelviewMatrix = glm::translate(modelviewMatrix, player.position);
		modelviewMatrix = glm::rotate(modelviewMatrix, atan2f(-player.facing.x, player.facing.y), glm::vec3(0.0f, 0.0f, 1.0f));

		DrawSprite(playerSprite, modelviewMatrix, projectionMatrix);
	}

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
		auto joystickMovementX = SDL_JoystickGetAxis(joystick.get(), 0) / 32768.0f;
		auto joystickMovementY = -SDL_JoystickGetAxis(joystick.get(), 1) / 32768.0f;
		playerInput.movement = Vector2(joystickMovementX, joystickMovementY);

		auto joystickFacingX = SDL_JoystickGetAxis(joystick.get(), 2) / 32768.0f;
		auto joystickFacingY = -SDL_JoystickGetAxis(joystick.get(), 3) / 32768.0f;
		playerInput.facing = Vector2(joystickFacingX, joystickFacingY);

		auto joystickRightTrigger = SDL_JoystickGetAxis(joystick.get(), 5) / 32768.0f;
		playerInput.firing = (joystickRightTrigger > 0.8f);

		UpdateWorld();

		RenderWorld(*renderer);
		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}