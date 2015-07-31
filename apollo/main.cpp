#include <algorithm>
#include <cstdio>
#include <cassert>
#include <memory>
#include <tuple>
#include <chrono>
#include <unordered_set>

#include "GL/glew.h"
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_main.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "scope_exit.h"
#include "gl_helpers.h"
#include "sdl_helpers.h"
#include "math_helpers.h"
#include "Sprite.h"
#include "debug_draw.h"
#include "World.h"

using namespace std;
using namespace std::chrono;



float playerMovementSpeed = 2.0f;
float playerRotationSpeed = 0.2f;
struct PlayerInput
{
	Vector2 movement { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
	bool firing { false };
};
PlayerInput playerInput;


void ApplyPlayerInput()
{
	auto& playerRB = GetRigidBody(player.objectId);
	playerRB.position += playerInput.movement * playerMovementSpeed;
	if (glm::length(playerInput.facing) > 0.5f)
	{
		auto playerHeading = atan2f(-playerRB.facing.x, playerRB.facing.y);
		auto normalizedInput = glm::normalize(playerInput.facing);
		auto desiredHeading = atan2f(-normalizedInput.x, normalizedInput.y);
		auto delta = desiredHeading - playerHeading;
		if (delta < -PI)
		{
			delta += TWO_PI;
		}
		else if (delta > PI)
		{
			delta -= TWO_PI;
		}
		delta = clamp(delta, -playerRotationSpeed, playerRotationSpeed);
		auto newHeading = playerHeading + delta;

		playerRB.facing = Vector2(-sin(newHeading), cos(newHeading));
	}

	if (playerInput.firing)
	{
		FirePlayerBullet();
	}
}




GLProgram spriteShaderProgram;
Sprite playerSprite;
Sprite bulletSprite;
Sprite enemySprite;


bool LoadResources()
{
	spriteShaderProgram = LoadShaders("sprite_vs.glsl", "sprite_fs.glsl");
	if (spriteShaderProgram == 0)
		return false;

	playerSprite = CreateSprite("apollo.png");
	bulletSprite = CreateSprite("bullet.png");
	enemySprite = CreateSprite("enemy.png");

	return CheckOpenGLErrors();
}



void RenderWorld()
{
	glClearColor(0, 0, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(spriteShaderProgram);

	auto projectionMatrix = glm::ortho(-320.0f, 320.0f, -240.0f, 240.0f);

	// draw the bullets
	for_each(begin(bullets), end(bullets), [&projectionMatrix](const GameObject& bullet)
	{
		if (bullet.isAlive)
		{
			auto& bulletRB = GetRigidBody(bullet.objectId);
			auto modelviewMatrix = CreateSpriteModelviewMatrix(bulletSprite, bulletRB.position, bulletRB.facing);
			DrawSprite(bulletSprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
		}
	});

	// draw the aliens
	for_each(begin(aliens), end(aliens), [&projectionMatrix](const GameObject& enemy)
	{
		if (enemy.isAlive)
		{
			auto& enemyRB = GetRigidBody(enemy.objectId);
			auto modelviewMatrix = CreateSpriteModelviewMatrix(enemySprite, enemyRB.position, enemyRB.facing);
			DrawSprite(enemySprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
		}
	});

	// draw the player
	{
		auto& playerRB = GetRigidBody(player.objectId);
		auto modelviewMatrix = CreateSpriteModelviewMatrix(playerSprite, playerRB.position, playerRB.facing);
		DrawSprite(playerSprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	}

	// draw the collision world
	//for_each(begin(collisionObjects), end(collisionObjects), [](const auto& collisionObject)
	//{
	//	auto& gameObject = GetGameObject(collisionObject.objectId);
	//	auto transform = CalculateObjectTransform(collisionObject.position, collisionObject.facing);
	//	float w = collisionObject.aabbDimensions.x;
	//	float h = collisionObject.aabbDimensions.y;
	//	auto color = gameObject.isAlive ? Color::White : Color::Gray;
	//	DebugDrawBox(transform, w, h, color);
	//});

	DebugDrawRender(projectionMatrix);

	CheckOpenGLErrors();
}


int main(int /*argc*/, char** /*argv*/)
{
	SeedRandom(2);

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

	auto joystick = unique_ptr<SDL_Joystick, decltype(&SDL_JoystickClose)>(SDL_JoystickOpen(0), SDL_JoystickClose);

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
	auto window = unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("apollo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI), SDL_DestroyWindow);
	if (!window)
	{
		printf("Unable to create SDL window: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	auto renderer = unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>(SDL_CreateRenderer(window.get(), -1, 0), SDL_DestroyRenderer);

	auto glContext = SDL_GL_CreateContext(window.get());
	auto glContextDeleter = make_scope_exit([&glContext] () { SDL_GL_DeleteContext(glContext); });

	glewInit();

	glViewport(0, 0, windowWidth, windowHeight);

	if (!LoadResources())
	{
		return 1;
	}

	DebugDrawInit();
	auto debugDrawCleanup = make_scope_exit([] () { DebugDrawShutdown(); });

	InitWorld();

	auto lastTime = high_resolution_clock::now();

	for (;;)
	{
		auto currentTime = high_resolution_clock::now();
		auto elapsedTime = duration_cast<duration<float>>(currentTime - lastTime).count();
		lastTime = currentTime;

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
		auto movementInput = Vector2(joystickMovementX, joystickMovementY);
		if (glm::length(movementInput) > 0.2f)
		{
			playerInput.movement = movementInput;
		}
		else
		{
			playerInput.movement = Vector2(0.0f, 0.0f);
		}

		auto joystickFacingX = SDL_JoystickGetAxis(joystick.get(), 2) / 32768.0f;
		auto joystickFacingY = -SDL_JoystickGetAxis(joystick.get(), 3) / 32768.0f;
		playerInput.facing = Vector2(joystickFacingX, joystickFacingY);

		auto joystickRightTrigger = SDL_JoystickGetAxis(joystick.get(), 5) / 32768.0f;
		playerInput.firing = (joystickRightTrigger > -0.5f);

		ApplyPlayerInput();

		UpdateWorld(elapsedTime);

		RenderWorld();
		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}