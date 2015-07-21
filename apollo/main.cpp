
#include <cstdio>
#include <cassert>
#include <memory>
#include <tuple>
#include <chrono>

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


using ObjectId = uint64_t;
ObjectId getNextObjectId()
{
	static ObjectId objectId = 1;
	return objectId++;
}


struct RigidBody
{
	RigidBody()
		: objectId { 0 }
		, position { 0.0f, 0.0f, 0.0f }
		, facing { 0.0f, 1.0f, 0.0f }
	{
	}

	RigidBody(const Vector3& position, const Vector3& facing)
		: objectId { getNextObjectId() }
		, position { position }
		, facing { facing }
	{
	}

	RigidBody(RigidBody&& other) noexcept
		: objectId { other.objectId }
		, position { other.position }
		, facing { other.facing }
	{
		other.objectId = 0;
	}

	const RigidBody& operator=(RigidBody&& other)
	{
		objectId = other.objectId;
		other.objectId = 0;
		position = other.position;
		facing = other.facing;
		return *this;
	}

	RigidBody(const RigidBody&) = delete;
	RigidBody& operator=(const RigidBody&) = delete;

	ObjectId objectId;
	Vector3 position;
	Vector3 facing;
};

RigidBody player;
vector<RigidBody> bullets;
vector<RigidBody> enemies;


struct CollisionObject
{
	Vector3 aabbMin;
	Vector3 aabbMax;
	ObjectId objectId;
};
vector<CollisionObject> collisionWorld;


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


void InitWorld()
{
	bullets.reserve(100);
	enemies.reserve(100);

	enemies.emplace_back(Vector3(0, 0, 0), Vector3(0, 1, 0));
}



void FireBullet()
{
	//if (bullets.size() == bullets.capacity())
	//{
	//	// don't allow the vector to reallocate
	//	// reuse the oldest enemy
	//}
	//else
	{
		bullets.emplace_back(player.position + player.facing * 16.0f, player.facing);
	}
}


void UpdateWorld(float deltaTime)
{
	player.position += Vector3(playerInput.movement, 0.0f) * playerMovementSpeed;
	if (glm::length(playerInput.facing) > 0.8f)
	{
		auto playerHeading = atan2f(-player.facing.x, player.facing.y);
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

		player.facing = Vector3(-sin(newHeading), cos(newHeading), 0.0f);
	}

	if (playerInput.firing)
	{
		FireBullet();
	}

	for (auto& bullet : bullets)
	{
		const float bulletSpeed = 1200.0f * deltaTime;
		bullet.position += bulletSpeed * bullet.facing;
	}
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

	for (const auto& bullet : bullets)
	{
		auto modelviewMatrix = glm::mat4();
		modelviewMatrix = glm::translate(modelviewMatrix, bullet.position);
		modelviewMatrix = glm::rotate(modelviewMatrix, atan2f(-bullet.facing.x, bullet.facing.y), glm::vec3(0.0f, 0.0f, 1.0f));

		DrawSprite(bulletSprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	}

	for (const auto& enemy : enemies)
	{
		auto modelviewMatrix = glm::mat4();
		modelviewMatrix = glm::translate(modelviewMatrix, enemy.position);
		modelviewMatrix = glm::rotate(modelviewMatrix, atan2f(-enemy.facing.x, enemy.facing.y), glm::vec3(0.0f, 0.0f, 1.0f));

		DrawSprite(enemySprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	}

	{
		auto modelviewMatrix = glm::mat4();
		modelviewMatrix = glm::translate(modelviewMatrix, player.position);
		modelviewMatrix = glm::rotate(modelviewMatrix, atan2f(-player.facing.x, player.facing.y), glm::vec3(0.0f, 0.0f, 1.0f));

		DrawSprite(playerSprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	}

	CheckOpenGLErrors();
}


int main(int /*argc*/, char** /*argv*/)
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

	if (!LoadResources())
	{
		return 1;
	}

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
		playerInput.movement = Vector2(joystickMovementX, joystickMovementY);

		auto joystickFacingX = SDL_JoystickGetAxis(joystick.get(), 2) / 32768.0f;
		auto joystickFacingY = -SDL_JoystickGetAxis(joystick.get(), 3) / 32768.0f;
		playerInput.facing = Vector2(joystickFacingX, joystickFacingY);

		auto joystickRightTrigger = SDL_JoystickGetAxis(joystick.get(), 5) / 32768.0f;
		playerInput.firing = (joystickRightTrigger > 0.8f);

		UpdateWorld(elapsedTime);

		RenderWorld();
		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}