#include <memory>
#include <iostream>
#include <fstream>

#include "GL/glew.h"
#include "SDL.h"
#include "SDL_image.h"

#pragma warning(push, 3)
#pragma warning(disable: 4996)
#define FONTSTASH_IMPLEMENTATION    // Expands implementation
#include "fontstash.h"
#define GLFONTSTASH_IMPLEMENTATION  // Expands implementation
#include "glfontstash.h"
#pragma warning(pop)

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "game.h"
#include "gl_helpers.h"
#include "sdl_helpers.h"
#include "math_helpers.h"
#include "physics.h"
#include "Sprite.h"
#include "game_object.h"
#include "world.h"
#include "debug_draw.h"

using namespace std;

unique_ptr<struct FONScontext, decltype(&glfonsDelete)> fontStash { 0, glfonsDelete };
int fontNormal;

GLProgram spriteShaderProgram;

struct RenderModel
{
	RenderModel(const Sprite& sprite)
		: sprite(sprite)
	{
	}

	Sprite sprite;
};

static vector<unique_ptr<RenderModel>> renderModels; // set an arbitrary initial size to prevent initial allocations

void CreateRenderModel(GameObjectType gameObjectType, const char* spriteName)
{
	auto renderModel = make_unique<RenderModel>(CreateSprite(spriteName));

	if (renderModels.size() <= static_cast<int>(gameObjectType))
		renderModels.resize(static_cast<int>(gameObjectType) + 1);
	renderModels[static_cast<int>(gameObjectType)] = move(renderModel);
}


const RenderModel& GetRenderModel(GameObjectType gameObjectType)
{
	assert(static_cast<int>(gameObjectType) < renderModels.size());
	assert(renderModels[static_cast<int>(gameObjectType)]);
	return *renderModels[static_cast<int>(gameObjectType)];
}


bool LoadResources()
{
	fontStash = unique_ptr<struct FONScontext, decltype(&glfonsDelete)> { glfonsCreate(512, 512, FONS_ZERO_TOPLEFT), glfonsDelete };
	fontNormal = fonsAddFont(fontStash.get(), "sans", "Bluehigh.ttf");

	spriteShaderProgram = LoadShaders("sprite_vs.glsl", "sprite_fs.glsl");
	if (spriteShaderProgram == 0)
		return false;

	renderModels.reserve(10);
	CreateRenderModel(GameObjectType::Player, "apollo.png");
	CreateRenderModel(GameObjectType::Bullet, "bullet.png");
	CreateRenderModel(GameObjectType::AlienRandom, "enemy_random.png");
	CreateRenderModel(GameObjectType::AlienChase, "enemy_chase.png");
	CreateRenderModel(GameObjectType::AlienShy, "enemy_shy.png");

	return CheckOpenGLErrors();
}



void RenderWorld(const Time& /*time*/)
{
	glClearColor(0, 0, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(spriteShaderProgram);

	auto projectionMatrix = glm::ortho(-320.0f, 320.0f, -240.0f, 240.0f);

	// draw the bullets
	for_each(begin(bullets), end(bullets), [&projectionMatrix] (const GameObject& bullet)
	{
		if (bullet.isAlive)
		{
			auto& bulletRB = GetRigidBody(bullet.objectId);
			const auto& renderModel = GetRenderModel(bullet.type);
			auto modelviewMatrix = CreateSpriteModelviewMatrix(renderModel.sprite, bulletRB.position, bulletRB.facing);
			DrawSprite(renderModel.sprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
		}
	});

	// draw the aliens
	for_each(begin(aliens), end(aliens), [&projectionMatrix] (const GameObject& enemy)
	{
		if (enemy.isAlive)
		{
			auto& enemyRB = GetRigidBody(enemy.objectId);
			const auto& renderModel = GetRenderModel(enemy.type);
			auto modelviewMatrix = CreateSpriteModelviewMatrix(renderModel.sprite, enemyRB.position, enemyRB.facing);
			DrawSprite(renderModel.sprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
		}
	});

	// draw the player
	{
		auto& playerRB = GetRigidBody(player.objectId);
		const auto& renderModel = GetRenderModel(player.type);
		auto modelviewMatrix = CreateSpriteModelviewMatrix(renderModel.sprite, playerRB.position, playerRB.facing);
		DrawSprite(renderModel.sprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
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


vector<string> credits;

void LoadCredits()
{
	ifstream is { "credits.txt" };
	if (!is)
	{
		printf("Error opening credits.txt file.");
		return;
	}
	do
	{
		string line;
		getline(is, line);
		credits.push_back(line);
	} while (!is.eof());
	if (!is)
	{
		printf("Error reading credits.txt file.");
		return;
	}
}

void RollCredits(const Time& time)
{
	if (credits.size() == 0)
		LoadCredits();

	static float height = 250.0f;
	height += -20.0f * time.deltaTime;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-320.0f, 320.0f, 240.0f, -240.0f, -100.0f, 100.0f);
	fonsSetFont(fontStash.get(), fontNormal);
	fonsSetSize(fontStash.get(), 24.0f);
	fonsSetColor(fontStash.get(), glfonsRGBA(255, 255, 255, 255));
	float dx = -100.0f, dy = height;
	for (const auto& line : credits)
	{
		fonsDrawText(fontStash.get(), dx, dy, line.c_str(), NULL);
		dy += 20.0f;
	}
}



void RenderUI(const Time& time)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-320.0f, 320.0f, 240.0f, -240.0f, -100.0f, 100.0f);
	float dx = -320.0f, dy = -220.0f;
	fonsSetFont(fontStash.get(), fontNormal);
	fonsSetSize(fontStash.get(), 24.0f);
	fonsSetColor(fontStash.get(), glfonsRGBA(255, 255, 255, 255));
	fonsDrawText(fontStash.get(), dx, dy, "Apollo", NULL);

	if (IsGameOver())
	{
		RollCredits(time);
	}

	CheckOpenGLErrors();
}
