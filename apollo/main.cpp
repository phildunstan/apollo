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

#include "imgui.h"
#include "imgui_impl_sdl.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "game.h"
#include "profiler.h"
#include "scope_exit.h"
#include "gl_helpers.h"
#include "math_helpers.h"
#include "sprite.h"
#include "debug_draw.h"
#include "physics.h"
#include "player.h"
#include "rendering.h"
#include "tweakables.h"
#include "world.h"
#include "recording.h"

using namespace std;
using namespace std::chrono;


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

int main(int /*argc*/, char** /*argv*/)
{
	ProfilerInit();
	auto profilerQuiter = make_scope_exit(ProfilerShutdown);

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

	// use an OpenGL ES 3.0 context
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	int windowWidth = 1280;
	int windowHeight = 720;
	int windowLeft = 2560 - windowWidth -10; // hardcode a window position for the live stream
	int windowRight = 1080 - windowHeight - 10;
	auto window = unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(SDL_CreateWindow("apollo", windowLeft, windowRight, windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI), SDL_DestroyWindow);
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

	ImGui_ImplSdl_Init(window.get());
	auto imguiSDLCleanup = make_scope_exit(ImGui_ImplSdl_Shutdown);

	CreateGameObjectMetaData();

	if (!LoadResources())
	{
		return 1;
	}

	DebugDrawInit();
	auto debugDrawCleanup = make_scope_exit([] () { DebugDrawShutdown(); });

	bool renderDebugUI = false;
	bool renderProfilerUI = true;
	ProfilerRenderingMode renderProfilerMode = ProfilerRenderingMode::FrameTotals;

	InitPhysics();
	InitWorld();

	auto startTime = high_resolution_clock::now();
	auto lastTime = startTime;

	enum class GameUpdateMode { Paused, Play, Replay };
	GameUpdateMode updateMode = GameUpdateMode::Play;
	int currentSnapshotIndex = 0;

	for (;;)
	{
		PROFILER_BEGIN_FRAME();
		PROFILER_TIMER_BEGIN(main_loop);

		ImGui_ImplSdl_NewFrame(window.get());

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				return 0;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_BACKQUOTE:
					renderDebugUI = !renderDebugUI;
					break;
				case SDLK_F11:
					renderProfilerUI = !renderProfilerUI;
					if (renderProfilerUI)
					{
						// invert the rendering mode every time the profiler is enabled
						renderProfilerMode = (renderProfilerMode == ProfilerRenderingMode::FrameThreads) ? ProfilerRenderingMode::FrameTotals : ProfilerRenderingMode::FrameThreads;
					}
					break;
				case SDLK_LEFT:
					currentSnapshotIndex = std::max(currentSnapshotIndex - 1, 0);
					updateMode = GameUpdateMode::Replay;
					break;
				case SDLK_RIGHT:
					if (currentSnapshotIndex == GetSnapshotCount() - 1)
					{
						updateMode = GameUpdateMode::Play;
					}
					else
					{
						currentSnapshotIndex = std::min(currentSnapshotIndex + 1, GetSnapshotCount());
						updateMode = GameUpdateMode::Replay;
					}
					break;
				}
				break;
			}
			ImGui_ImplSdl_ProcessEvent(&event);
		}


		Time time;
		uint64_t frameSeed { 0 };

		switch (updateMode)
		{
		case GameUpdateMode::Paused:
			break;

		case GameUpdateMode::Play:
			{
				frameSeed = GetRandomUint64();
				SeedRandom(frameSeed);

				auto currentTime = high_resolution_clock::now();
				time.elapsedTime = duration_cast<duration<float>>(currentTime - startTime).count();
				time.deltaTime = std::min(duration_cast<duration<float>>(currentTime - lastTime).count(), 0.1f); // cap deltaTime to 0.1s
				lastTime = currentTime;

				if (joystick)
				{
					ReadPlayerInputFromJoystick(*joystick);
				}
			}
			break;

		case GameUpdateMode::Replay:
			ReplaySnapshot(currentSnapshotIndex, time, frameSeed, playerInput);
			SeedRandom(frameSeed);
			break;
		};

		if (updateMode != GameUpdateMode::Paused)
		{
			ApplyPlayerInput(time, playerInput);
			UpdateWorld(time);
		}

		RenderWorld(time, windowWidth, windowHeight);
		RenderUI(time, windowWidth, windowHeight);
		PROFILER_TIMER_END(main_loop);

		DebugDrawRender(time, windowWidth, windowHeight);

		if (updateMode == GameUpdateMode::Play)
		{
			currentSnapshotIndex = CreateSnapshot(time, frameSeed, playerInput);
		}
		ValidateSnapshot(currentSnapshotIndex, time, frameSeed);

		if (renderProfilerUI)
		{
			RenderProfiler(time, windowWidth, windowHeight, renderProfilerMode);
		}

		if (renderDebugUI)
		{
			RenderDebugUI(time, windowWidth, windowHeight);
		}
		ImGui::Render();

		CheckOpenGLErrors();

		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}