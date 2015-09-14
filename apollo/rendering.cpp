#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>

#include "GL/glew.h"
#include "SDL.h"

#include "imgui.h"
#include "imgui_impl_sdl.h"

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
#include "debug_draw.h"
#include "math_helpers.h"
#include "physics.h"
#include "player.h"
#include "profiler.h"
#include "sprite.h"
#include "tweakables.h"
#include "game_object.h"
#include "world.h"

using namespace std;

TWEAKABLE(bool, renderBoundingBoxes, "Physics.RenderBoundingBoxes", false, false, true);
TWEAKABLE(bool, renderDeadObjects, "Physics.RenderDeadObjects", false, false, true);

unique_ptr<struct FONScontext, decltype(&glfonsDelete)> fontStash { 0, glfonsDelete };
int fontNormal;

GLProgram spriteShaderProgram;

struct RenderModel
{
	explicit RenderModel(const Sprite& sprite)
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
	fontNormal = fonsAddFont(fontStash.get(), "sans", "Hack-Regular.ttf");

	spriteShaderProgram = LoadShaders("sprite_vs.glsl", "sprite_fs.glsl");
	if (spriteShaderProgram == 0)
		return false;

	renderModels.reserve(10);
	for (const auto& metaData : gameObjectMetaDatas)
	{
		CreateRenderModel(metaData.type, metaData.spriteFilename);
	}

	return CheckOpenGLErrors();
}



void RenderWorld(const Time& /*time*/, int windowWidth, int windowHeight)
{
	PROFILER_TIMER_FUNCTION();

	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0, 0, 0.2f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(spriteShaderProgram);

	auto projectionMatrix = glm::ortho(-windowWidth / 2.0f, windowWidth / 2.0f, -windowHeight / 2.0f, windowHeight / 2.0f, -10.0f, 10.0f);

	// draw the walls
	DebugDrawLine(Vector2 { minWorld.x, minWorld.y }, Vector2 { minWorld.x, maxWorld.y }, Color::White);
	DebugDrawLine(Vector2 { minWorld.x, minWorld.y }, Vector2 { maxWorld.x, minWorld.y }, Color::White);
	DebugDrawLine(Vector2 { maxWorld.x, maxWorld.y }, Vector2 { minWorld.x, maxWorld.y }, Color::White);
	DebugDrawLine(Vector2 { maxWorld.x, maxWorld.y }, Vector2 { maxWorld.x, minWorld.y }, Color::White);

	// draw the bullets
	for_each(begin(bullets), end(bullets), [&projectionMatrix] (const GameObject& bullet)
	{
		if (bullet.isAlive)
		{
			auto& bulletRB = GetRigidBody(bullet.objectId);
			const auto& renderModel = GetRenderModel(GetType(bullet.objectId));
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
			const auto& renderModel = GetRenderModel(GetType(enemy.objectId));
			auto modelviewMatrix = CreateSpriteModelviewMatrix(renderModel.sprite, enemyRB.position, enemyRB.facing);
			DrawSprite(renderModel.sprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
		}
	});

	// draw the player
	{
		auto& playerRB = GetRigidBody(player.objectId);
		const auto& renderModel = GetRenderModel(GetType(player.objectId));
		auto modelviewMatrix = CreateSpriteModelviewMatrix(renderModel.sprite, playerRB.position, playerRB.facing);
		DrawSprite(renderModel.sprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	}

	// draw the collision world
	if (renderBoundingBoxes || renderDeadObjects)
	{
		for_each(begin(collisionObjects), end(collisionObjects), [] (const auto& collisionObject)
		{
			auto& gameObject = GetGameObject(collisionObject.objectId);
			auto transform = CalculateObjectTransform(collisionObject.position, collisionObject.facing);
			float w = collisionObject.boundingBoxDimensions.x;
			float h = collisionObject.boundingBoxDimensions.y;
			auto color = gameObject.isAlive ? Color::White : Color::Gray;
			if ((renderBoundingBoxes && gameObject.isAlive) || (renderDeadObjects && !gameObject.isAlive))
			{
				DebugDrawBox(transform, w, h, color);
			}
		});
	}

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

	fonsSetFont(fontStash.get(), fontNormal);
	fonsSetSize(fontStash.get(), 24.0f);
	fonsSetColor(fontStash.get(), glfonsRGBA(255, 255, 255, 255));
	float dx = -100.0f, dy = height;
	for (const auto& line : credits)
	{
		fonsDrawText(fontStash.get(), dx, dy, line.c_str(), nullptr);
		dy += 20.0f;
	}
}



void RenderUI(const Time& time, int windowWidth, int windowHeight)
{
	PROFILER_TIMER_FUNCTION();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-windowWidth / 2.0f, windowWidth / 2.0f, windowHeight / 2.0f, -windowHeight / 2.0f, -10.0f, 10.0f);

	float dx = -windowWidth / 2.0f;
	float dy = -windowHeight / 2.0f + 20.0f;
	fonsSetFont(fontStash.get(), fontNormal);
	fonsSetSize(fontStash.get(), 24.0f);
	fonsSetColor(fontStash.get(), glfonsRGBA(255, 255, 255, 255));
	fonsDrawText(fontStash.get(), dx, dy, "Apollo", nullptr);


	dx = windowWidth / 2.0f - 160.0f;
	fonsSetFont(fontStash.get(), fontNormal);
	fonsSetSize(fontStash.get(), 24.0f);
	fonsSetColor(fontStash.get(), glfonsRGBA(255, 255, 255, 255));
	char scoreText[24];
	snprintf(scoreText, 24, "Score: %04d", playerScore);
	fonsDrawText(fontStash.get(), dx, dy, scoreText, nullptr);

	if (IsGameOver())
	{
		RollCredits(time);
	}

	CheckOpenGLErrors();
}


void RenderDebugUI(const Time& /*time*/, int /*windowWidth*/, int /*windowHeight*/)
{
	PROFILER_TIMER_FUNCTION();

	const auto& tweakables = Tweakables::GetInstance().GetTweakables();
	for (const auto& tweakable : tweakables)
	{
		const char* name = tweakable.name;
		//while (const char* dot = strchr(name, '.'))
		//{
		//	std::string headerName(name, dot);
		//	ImGui::CollapsingHeader(headerName.c_str());
		//	name = dot + 1;
		//}

		switch (tweakable.type)
		{
		case Tweakables::Tweakable::Type::Bool:
			ImGui::Checkbox(name, tweakable.value.b);
			break;
		case Tweakables::Tweakable::Type::Int:
			ImGui::SliderInt(name, tweakable.value.i, tweakable.min.i, tweakable.max.i);
			break;
		case Tweakables::Tweakable::Type::Float:
			ImGui::SliderFloat(name, tweakable.value.f, tweakable.min.f, tweakable.max.f);
			break;
		case Tweakables::Tweakable::Type::Vector2:
		{
			char modifiedName[64];
			snprintf(modifiedName, 64, "%s_x", name);
			ImGui::SliderFloat(modifiedName, &tweakable.value.vec2->x, tweakable.min.vec2.x, tweakable.max.vec2.x);
			snprintf(modifiedName, 64, "%s_y", name);
			ImGui::SliderFloat(modifiedName, &tweakable.value.vec2->y, tweakable.min.vec2.y, tweakable.max.vec2.y);
			break;
		}
		case Tweakables::Tweakable::Type::Vector3:
		{
			char modifiedName[64];
			snprintf(modifiedName, 64, "%s_x", name);
			ImGui::SliderFloat(modifiedName, &tweakable.value.vec3->x, tweakable.min.vec3.x, tweakable.max.vec3.x);
			snprintf(modifiedName, 64, "%s_y", name);
			ImGui::SliderFloat(modifiedName, &tweakable.value.vec3->y, tweakable.min.vec3.y, tweakable.max.vec3.y);
			snprintf(modifiedName, 64, "%s_z", name);
			ImGui::SliderFloat(modifiedName, &tweakable.value.vec3->z, tweakable.min.vec3.z, tweakable.max.vec3.z);
			break;
		}
		case Tweakables::Tweakable::Type::Color:
			ImGui::ColorEdit4(name, reinterpret_cast<float*>(tweakable.value.color));
			break;
		default:
			break;
		}
	}
}



void RenderProfiler(const Time& /*time*/, int windowWidth, int windowHeight)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, windowWidth, windowHeight, 0, -10.0f, 10.0f);

	const auto& accumulatedStatistics = ProfilerGetAccumulatedStatistics();

	const auto& lastFrameStatistics = accumulatedStatistics.back();
	float dx = 0;
	float dy = 50.0f;
	for (const auto& dataPoint : lastFrameStatistics)
	{
		if (dataPoint.hitCount > 0)
		{
			fonsSetFont(fontStash.get(), fontNormal);
			fonsSetSize(fontStash.get(), 24.0f);
			fonsSetColor(fontStash.get(), glfonsRGBA(255, 255, 255, 255));
			char text[255];
			_snprintf_s(text, 255, "%-40s %9lldus %8d %9lldus", dataPoint.id, dataPoint.duration.count() / 1000, dataPoint.hitCount, dataPoint.duration.count() / (dataPoint.hitCount * 1000));
			fonsDrawText(fontStash.get(), dx, dy, text, nullptr);
			dy += 20.0f;
		}
	}

	// 200px high = 1/30s
	float graphHeight = 200.0f;
	float performanceGraphScale = graphHeight * 1e-6f / 30.0f; // scale in pixels per ns

	DebugDrawLine(Vector2 { -windowWidth / 2.0f, graphHeight - windowHeight / 2.0f }, Vector2 { windowWidth / 2.0f, graphHeight - windowHeight / 2.0f }, Color::Red);

	float graphBarWidth = 20.0f;
	int graphHistorySize = windowWidth / static_cast<int>(graphBarWidth);
	int numBars = min(graphHistorySize, static_cast<int>(accumulatedStatistics.size()));
	for (size_t i = 0; i < numBars; ++i)
	{
		const auto& frameStatistics = accumulatedStatistics[accumulatedStatistics.size() - numBars + i];
		float frameTime = static_cast<float>(frameStatistics[0].duration.count());
		float x = i * graphBarWidth - windowWidth / 2.0f;
		float y = -windowHeight / 2.0f;
		float w = graphBarWidth;
		float h = performanceGraphScale * frameTime;
		DebugDrawBox2d(Vector2 { x, y }, Vector2 { x + w, y + h }, Color::Cyan);
	}
}