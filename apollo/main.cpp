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


struct GameObject
{
	GameObject()
		: objectId { getNextObjectId() }
		, isAlive { true }
	{
	}

	ObjectId objectId;
	bool isAlive;
};

GameObject player;
vector<GameObject> bullets;
vector<GameObject> enemies;

GameObject& GetGameObject(ObjectId objectId)
{
	if (player.objectId == objectId)
		return player;
	// we can use a binary search if we can guarantee that elements are only added to the RigidBody vectors
	// in increasing ObjectId order, and that the vectors are never reordered.
	auto enemyIter = lower_bound(begin(enemies), end(enemies), objectId, [](const auto& enemy, auto objectId) { return enemy.objectId < objectId; });
	if ((enemyIter != end(enemies)) && (enemyIter->objectId == objectId))
		return *enemyIter;
	auto bulletIter = lower_bound(begin(bullets), end(bullets), objectId, [](const auto& bullet, auto objectId) { return bullet.objectId < objectId; });
	assert((bulletIter != end(bullets)) && (bulletIter->objectId == objectId));
	return *bulletIter;
}


struct RigidBody
{
	RigidBody(ObjectId _objectId, const Vector3& _position, const Vector3& _facing)
		: objectId { _objectId }
		, position { _position }
		, facing { _facing }
	{
		assert(IsUnitLength(facing));
	}

	ObjectId objectId;
	Vector3 position;
	Vector3 facing;
};

vector<RigidBody> rigidBodies;

RigidBody& GetRigidBody(ObjectId objectId)
{
	// we can use a binary search if we can guarantee that elements are only added to the RigidBody vectors
	// in increasing ObjectId order, and that the vectors are never reordered.
	auto rigidBodyIter = lower_bound(begin(rigidBodies), end(rigidBodies), objectId, [] (const auto& rigidBody, auto objectId) { return rigidBody.objectId < objectId; });
	assert((rigidBodyIter != end(rigidBodies)) && (rigidBodyIter->objectId == objectId));
	return *rigidBodyIter;
}


struct CollisionObject
{
	CollisionObject(ObjectId _objectId, const Vector3& _aabbDimensions)
		: objectId(_objectId)
		, aabbDimensions(_aabbDimensions)
		, position({ 0.0f, 0.0f, 0.0f })
		, facing({ 0.0f, 1.0f, 0.0f })
	{
	}

	ObjectId objectId;
	Vector3 aabbDimensions;
	Vector3 position;
	Vector3 facing;
};

vector<CollisionObject> collisionObjects;

vector<Vector3> GatherObjectVertices(const CollisionObject& object)
{
	Vector3 yAxis = object.facing;
	Vector3 xAxis = PerpendicularVector2D(yAxis);
	Vector3 halfDimensions = 0.5f * object.aabbDimensions;
	vector<Vector3> vertices{
		object.position - halfDimensions.x * xAxis - halfDimensions.y * yAxis,
		object.position + halfDimensions.x * xAxis - halfDimensions.y * yAxis,
		object.position - halfDimensions.x * xAxis + halfDimensions.y * yAxis,
		object.position + halfDimensions.x * xAxis + halfDimensions.y * yAxis };
	return vertices;
}

bool CollisionObjectsCollide(const CollisionObject& objectA, const CollisionObject& objectB)
{
	// use the Separating Axis Theorem to check for collision

	// collect the vertices for objects A and B
	vector<Vector3> objectAVertices = GatherObjectVertices(objectA);
	vector<Vector3> objectBVertices = GatherObjectVertices(objectB);

	// collect the normal vectors for each edge in objects A and B
	vector<Vector3> edgeNormals;
	edgeNormals.reserve(4);
	assert(IsUnitLength(objectA.facing));
	edgeNormals.push_back(objectA.facing);
	edgeNormals.push_back(PerpendicularVector2D(objectA.facing));
	assert(IsUnitLength(objectB.facing));
	edgeNormals.push_back(objectB.facing);
	edgeNormals.push_back(PerpendicularVector2D(objectB.facing));

	for (const auto& edgeNormal : edgeNormals)
	{
		// find the intervals containing all of the vertices in objects A and B respectively projected edge normals
		// if the intervals don't overlap then there is no overlap between the objects
		float objectAMin = FLT_MAX;
		float objectAMax = -FLT_MAX;
		for_each(begin(objectAVertices), end(objectAVertices), [&] (const auto& vertex) {
			float p = glm::dot(vertex, edgeNormal);
			objectAMin = min(p, objectAMin);
			objectAMax = max(p, objectAMax);
		});

		float objectBMin = FLT_MAX;
		float objectBMax = -FLT_MAX;
		for_each(begin(objectBVertices), end(objectBVertices), [&] (const auto& vertex) {
			float p = glm::dot(vertex, edgeNormal);
			objectBMin = min(p, objectBMin);
			objectBMax = max(p, objectBMax);
		});

		if ((objectAMin > objectBMax) || (objectBMin > objectAMax))
			return false;
	}

	return true;
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


void AddRigidBody(ObjectId objectId, const Vector3& position, const Vector3& facing)
{
	rigidBodies.push_back(RigidBody { objectId, position, facing });
}

void AddCollisionObject(ObjectId objectId, const Vector3& aabbDimensions)
{
	collisionObjects.push_back(CollisionObject { objectId, aabbDimensions });
}


void InitWorld()
{
	assert(player.objectId != 0);
	AddRigidBody(player.objectId, Vector3{ 50.0f, 20.0f, 0.0f }, Vector3{ 1.0f, 0.0f, 0.0f });
	AddCollisionObject(player.objectId, Vector3 { 32.0f, 32.0f, 0.0f } );

	bullets.reserve(100);
	enemies.reserve(100);

	enemies.emplace_back();
	AddRigidBody(enemies.back().objectId, Vector3{ 100.0f, 100.0f, 0.0f }, glm::normalize(Vector3{ 1.0f, 1.0f, 0.0f }));
	AddCollisionObject(enemies.back().objectId, Vector3 { 32.0f, 32.0f, 0.0f });
}


void FireBullet()
{
	const auto& playerRB = GetRigidBody(player.objectId);		
	bullets.emplace_back();
	AddRigidBody(bullets.back().objectId, playerRB.position + playerRB.facing * 8.0f, playerRB.facing);
	AddCollisionObject(bullets.back().objectId, Vector3 { 2.0f, 12.0f, 0.0f });
}


void UpdateWorld(float deltaTime)
{
	auto& playerRB = GetRigidBody(player.objectId);
	playerRB.position += Vector3(playerInput.movement, 0.0f) * playerMovementSpeed;
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

		playerRB.facing = Vector3(-sin(newHeading), cos(newHeading), 0.0f);
	}

	if (playerInput.firing)
	{
		FireBullet();
	}

	for_each(begin(bullets), end(bullets), [deltaTime](auto& bullet)
	{
		const float bulletSpeed = 1200.0f * deltaTime;
		auto& bulletRB = GetRigidBody(bullet.objectId);
		bulletRB.position += bulletSpeed * bulletRB.facing;
	});

	// update the collision world
	for_each(begin(collisionObjects), end(collisionObjects), [] (auto& collisionObject)
	{
		const auto& rigidBody = GetRigidBody(collisionObject.objectId);
		collisionObject.position = rigidBody.position;
		collisionObject.facing = rigidBody.facing;
	});

	// collision tests between every collision object
	unordered_set<ObjectId> collidingObjects;

	for (int i = 0; i < collisionObjects.size(); ++i)
	{
		for (int j = i + 1; j < collisionObjects.size(); ++j)
		{
			if (CollisionObjectsCollide(collisionObjects[i], collisionObjects[j]))
			{
				collidingObjects.insert(collisionObjects[i].objectId);
				collidingObjects.insert(collisionObjects[j].objectId);
			}
		}
	}

	player.isAlive = (collidingObjects.find(player.objectId) == collidingObjects.end());
	for (auto& enemy : enemies)
		enemy.isAlive = (collidingObjects.find(enemy.objectId) == collidingObjects.end());
	for (auto& bullet : bullets)
		bullet.isAlive = (collidingObjects.find(bullet.objectId) == collidingObjects.end());
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
	for_each(begin(bullets), end(bullets), [&projectionMatrix](const auto& bullet)
	{
		auto& bulletRB = GetRigidBody(bullet.objectId);
		auto modelviewMatrix = CreateSpriteModelviewMatrix(bulletSprite, bulletRB.position, bulletRB.facing);
		DrawSprite(bulletSprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	});

	// draw the enemies
	for_each(begin(enemies), end(enemies), [&projectionMatrix](const auto& enemy)
	{
		auto& enemyRB = GetRigidBody(enemy.objectId);
		auto modelviewMatrix = CreateSpriteModelviewMatrix(enemySprite, enemyRB.position, enemyRB.facing);
		DrawSprite(enemySprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	});

	// draw the player
	{
		auto& playerRB = GetRigidBody(player.objectId);
		auto modelviewMatrix = CreateSpriteModelviewMatrix(playerSprite, playerRB.position, playerRB.facing);
		DrawSprite(playerSprite, spriteShaderProgram, modelviewMatrix, projectionMatrix);
	}

	// draw the collision world
	for_each(begin(collisionObjects), end(collisionObjects), [](const auto& collisionObject)
	{
		auto& gameObject = GetGameObject(collisionObject.objectId);
		auto transform = CalculateObjectTransform(collisionObject.position, collisionObject.facing);
		float w = collisionObject.aabbDimensions.x;
		float h = collisionObject.aabbDimensions.y;
		auto color = gameObject.isAlive ? Color::White : Color::Gray;
		DebugDrawBox(transform, w, h, color);
	});

	DebugDrawRender(projectionMatrix);

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

		UpdateWorld(elapsedTime);

		RenderWorld();
		SDL_GL_SwapWindow(window.get());
	}

	return 0;
}