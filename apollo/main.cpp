#include <algorithm>
#include <numeric>
#include <cstdio>
#include <cassert>
#include <memory>
#include <tuple>
#include <chrono>
#include <string>

#include "GL/glew.h"
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_main.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "game.h"
#include "scope_exit.h"
#include "gl_helpers.h"
#include "sdl_helpers.h"
#include "math_helpers.h"
#include "Sprite.h"
#include "debug_draw.h"
#include "World.h"
#include "rendering.h"

using namespace std;
using namespace std::chrono;


float playerMovementSpeed = 120.0f;
float playerRotationSpeed = 0.2f;
float playerFireRate = 4.0f; // shots per second
struct PlayerInput
{
	Vector2 movement { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
	bool firing { false };
};
PlayerInput playerInput;


void ReadPlayerInputFromJoystick(SDL_Joystick& joystick)
{
	auto joystickMovementX = SDL_JoystickGetAxis(&joystick, 0) / 32768.0f;
	auto joystickMovementY = -SDL_JoystickGetAxis(&joystick, 1) / 32768.0f;
	auto movementInput = Vector2(joystickMovementX, joystickMovementY);
	if (glm::length(movementInput) > 0.2f)
	{
		playerInput.movement = movementInput;
	}
	else
	{
		playerInput.movement = Vector2(0.0f, 0.0f);
	}

	auto joystickFacingX = SDL_JoystickGetAxis(&joystick, 2) / 32768.0f;
	auto joystickFacingY = -SDL_JoystickGetAxis(&joystick, 3) / 32768.0f;
	playerInput.facing = Vector2(joystickFacingX, joystickFacingY);

	auto joystickRightTrigger = SDL_JoystickGetAxis(&joystick, 5) / 32768.0f;
	playerInput.firing = (joystickRightTrigger > -0.5f);
}


void ApplyPlayerInput(const Time& time)
{
	auto& playerRB = GetRigidBody(player.objectId);
	playerRB.velocity = playerInput.movement * playerMovementSpeed;
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

	if (playerInput.firing && ((time.elapsedTime - player.timeOfLastShot) > 1.0f / playerFireRate))
	{
		FirePlayerBullet();
		player.timeOfLastShot = time.elapsedTime;
	}
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
	if (!joystick)
	{
		printf("No joystick attached.\n");
	}
		

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

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

	auto startTime = high_resolution_clock::now();
	auto lastTime = startTime;

	for (;;)
	{
		auto currentTime = high_resolution_clock::now();
		auto elapsedTime = duration_cast<duration<float>>(currentTime - startTime).count();
		auto deltaTime = duration_cast<duration<float>>(currentTime - lastTime).count();
		Time time { elapsedTime, deltaTime };
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

		if (joystick)
		{
			ReadPlayerInputFromJoystick(*joystick);
		}
		ApplyPlayerInput(time);

		UpdateWorld(time);

		RenderWorld(time);
		RenderUI(time);
		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}