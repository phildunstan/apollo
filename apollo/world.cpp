#include "world.h"

#include <iterator>
#include <algorithm>
#include <tuple>
#include <unordered_set>

#include "physics.h"
#include "player.h"
#include "profiler.h"
#include "math_helpers.h"
#include "ai.h"

using namespace std;

Vector2 minWorld { -640.0f, -360.0f };
Vector2 maxWorld { 640.0f, 360.0f };

GameObject player { GameObject::CreateGameObject<GameObjectType::Player>() };
vector<GameObject> bullets;
vector<GameObject> aliens;


void CreatePlayerGameObject()
{
	assert(player.objectId);
	const auto& metaData = GetGameObjectMetaData(GameObjectType::Player);
	Vector2 position { 50.0f, 20.0f };
	Vector2 facing { 1.0f, 0.0f };
	AddRigidBody(player.objectId, position, facing);
	auto& collisionObject = AddCollisionObject(player.objectId, metaData.boundingBoxDimensions);
	collisionObject.layer = CollisionLayer::Player;
	collisionObject.layerMask = CollisionLayer::Alien;
}


GameObject& GetGameObject(ObjectId objectId)
{
	if (player.objectId == objectId)
		return player;
	// we can use a binary search if we can guarantee that elements are never reordered.
	auto enemyIter = lower_bound(begin(aliens), end(aliens), objectId, [] (const auto& enemy, auto objectId) { return GetIndex(enemy.objectId) < GetIndex(objectId); });
	if ((enemyIter != end(aliens)) && (enemyIter->objectId == objectId))
		return *enemyIter;
	auto bulletIter = lower_bound(begin(bullets), end(bullets), objectId, [] (const auto& bullet, auto objectId) { return GetIndex(bullet.objectId) < GetIndex(objectId); });
	assert((bulletIter != end(bullets)) && (bulletIter->objectId == objectId));
	return *bulletIter;
}


tuple<Vector2, Vector2> findRandomPlaceToSpawnAlien()
{
	Vector2 position { 0.0f, 0.0f };
	bool haveGoodPosition = false;
	do
	{
		position = GetRandomVectorInBox(minWorld + Vector2(40, 40), maxWorld - Vector2(40, 40));
		haveGoodPosition = glm::distance(GetRigidBody(player.objectId).position, position) > 100.0f;
		haveGoodPosition = haveGoodPosition &&
			find_if(begin(collisionObjects), end(collisionObjects), [&position] (const CollisionObject& existingObject)
			{
				return glm::distance(existingObject.position, position) < 50.0f;
			}) == end(collisionObjects);
	} while (!haveGoodPosition);

	Vector2 facing = GetRandomVectorOnCircle();

	return make_tuple(position, facing);
}

void CreateAlienPhysics(ObjectId objectId, const Vector2& position, const Vector2& facing)
{
	const auto& metaData = GetGameObjectMetaData(GetType(objectId));
	AddRigidBody(objectId, position, facing);
	auto& collisionObject = AddCollisionObject(objectId, metaData.boundingBoxDimensions);
	collisionObject.layer = CollisionLayer::Alien;
	collisionObject.layerMask = CollisionLayer::Player | CollisionLayer::PlayerBullet;
}

template <GameObjectType AlienType>
GameObject& CreateAlien()
{
	aliens.push_back(GameObject::CreateGameObject<AlienType>());
	GameObject& alien = aliens.back();

	Vector2 position { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
	tie(position, facing) = findRandomPlaceToSpawnAlien();
	CreateAlienPhysics(alien.objectId, position, facing);

	CreateAI(alien.objectId);

	return alien;
}


float GetPositionAlongWallCoordFromPositionAndFacing(const Vector2& position, const Vector2& facing)
{
	float wallWidth = maxWorld.x - minWorld.x;
	float wallHeight = maxWorld.y - minWorld.y;
	float totalWallLength = 2 * wallWidth + 2 * wallHeight;

	if (IsSimilar(facing, Vector2 { 0, 1 }))
	{
		// position along bottom wall, p increasing to the right
		float p = position.x - minWorld.x;
		if (p < 0.0f)
			p += totalWallLength;
		return p;
	}

	if (IsSimilar(facing, Vector2 { -1, 0 }))
	{
		// position along right wall, p increasing upwards
		float p = wallWidth + position.y - minWorld.y;
		return p;
	}

	if (IsSimilar(facing, Vector2 { 0, -1 }))
	{
		// position along top wall, p increasing to the left
		float p = wallWidth + wallHeight + maxWorld.x - position.x;
		return p;
	}

	assert(IsSimilar(facing, Vector2 { 1, 0 }));
	{
		// position along left wall, p increasing downwards
		float p = wallWidth + wallHeight + wallWidth + maxWorld.y - position.y;
		if (p > totalWallLength)
			p -= totalWallLength;
		return p;
	}
}

tuple<Vector2, Vector2> GetPositionAndFacingFromWallCoord(float p, const Vector2& collisionBoxDimensions)
{
	float wallWidth = maxWorld.x - minWorld.x;
	float wallHeight = maxWorld.y - minWorld.y;

	if (p < wallWidth)
	{
		// position along bottom wall, p increasing to the right
		Vector2 position = Vector2 { minWorld.x + p,  minWorld.y + collisionBoxDimensions.y / 2.0f };
		Vector2 facing = Vector2 { 0.0f, 1.0f };
		return make_tuple(position, facing);
	}
	p -= wallWidth;

	if (p < wallHeight)
	{
		// position along right wall, p increasing upwards
		Vector2 position = Vector2 { maxWorld.x - collisionBoxDimensions.y / 2.0f,  minWorld.y + p };
		Vector2 facing = Vector2 { -1.0f, 0.0f };
		return make_tuple(position, facing);
	}
	p -= wallHeight;

	if (p < wallWidth)
	{
		// position along top wall, p increasing to the left
		Vector2 position = Vector2 { maxWorld.x - p,  maxWorld.y - collisionBoxDimensions.y / 2.0f };
		Vector2 facing = Vector2 { 0.0f, -1.0f };
		return make_tuple(position, facing);
	}
	p -= wallWidth;

	assert(p <= wallHeight + 1.0e-4f);
	p = min(p, wallHeight); // clamp the remainder to wallHeight to remove any floating point errors
	{
		// position along left wall, p increasing downwards
		Vector2 position = Vector2 { minWorld.x + collisionBoxDimensions.y / 2.0f,  maxWorld.y - p };
		Vector2 facing = Vector2 { 1.0f, 0.0f };
		return make_tuple(position, facing);
	}
}

tuple<Vector2, Vector2> findRandomPlaceAlongWallToSpawnWallHugger(const Vector2& collisionBoxDimensions)
{
	Vector2 position { 0.0f, 0.0f };
	Vector2 facing = { 0.0f, 1.0f };

	bool haveGoodPosition = false;
	do
	{
		// treat the 4 walls as a single line (bottom, right, top, left)
		// generate a random position along that line,
		// then map it back to the original walls.
		float wallWidth = maxWorld.x - minWorld.x;
		float wallHeight = maxWorld.y - minWorld.y;
		float wallLength = 2 * wallWidth + 2 * wallHeight;
		float p = GetRandomFloat01() * wallLength;

		tie(position, facing) = GetPositionAndFacingFromWallCoord(p, collisionBoxDimensions);

		haveGoodPosition = glm::distance(GetRigidBody(player.objectId).position, position) > 100.0f;
		haveGoodPosition = haveGoodPosition &&
			find_if(begin(collisionObjects), end(collisionObjects), [&position] (const CollisionObject& existingObject)
		{
			return glm::distance(existingObject.position, position) < 50.0f;
		}) == end(collisionObjects);
	} while (!haveGoodPosition);

	return make_tuple(position, facing);
}

template <>
GameObject& CreateAlien<GameObjectType::AlienWallHugger>()
{
	aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienWallHugger>());
	GameObject& alien = aliens.back();

	Vector2 position { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
	const auto& metaData = GetGameObjectMetaData(GetType(alien.objectId));
	Vector2 collisionBoundingBoxDimensions = metaData.boundingBoxDimensions;
	tie(position, facing) = findRandomPlaceAlongWallToSpawnWallHugger(collisionBoundingBoxDimensions);
	CreateAlienPhysics(alien.objectId, position, facing);
	CreateAI(alien.objectId);

	return alien;
}

void CreateRandomAlien()
{
	float random = GetRandomFloat01();
	if (random < 0.2f)
	{
		CreateAlien<GameObjectType::AlienShy>();
	}
	else if (random < 0.5f)
	{
		CreateAlien<GameObjectType::AlienChase>();
	}
	else if (random < 0.8f)
	{
		CreateAlien<GameObjectType::AlienRandom>();
	}
	else if (random < 0.9f)
	{
		CreateAlien<GameObjectType::AlienMothership>();
	}
	else
	{
		CreateAlien<GameObjectType::AlienWallHugger>();
	}
}


void InitWorld()
{
	CreatePlayerGameObject();

	bullets.reserve(1000);
	aliens.reserve(1000);

	// create a bunch of aliens to shoot
	const int numAliens = 30;
	for (int i = 0; i < numAliens; ++i)
	{
		CreateRandomAlien();
	}
}


void KillGameObject(ObjectId objectId)
{
	// player is invincible
	if (GetType(objectId) == GameObjectType::Player)
		return;

	GameObject& gameObject = GetGameObject(objectId);
	if (!gameObject.isAlive)
		return;
	IncrementPlayerScoreForKilling(objectId);
	gameObject.isAlive = false;

	auto& collisionObject = GetCollisionObject(objectId);
	collisionObject.layer = CollisionLayer::PendingDestruction;
	collisionObject.layerMask = CollisionLayer::None;
}



void UpdateWorld(const Time& time)
{
	PROFILER_TIMER_FUNCTION();

	static vector<pair<ObjectId, ObjectId>> collidingPairs;
	static vector<ObjectId> collidingWithWorld;

	UpdateRigidBodies(time);

	EnsurePlayerIsInsideWorldBounds();

	UpdateCollision(time, collidingPairs, collidingWithWorld);

	// resolve objects that have collided against the world
	for (const auto objectId : collidingWithWorld)
	{
		if ((GetType(objectId) != GameObjectType::Player) && (GetType(objectId) != GameObjectType::AlienWallHugger))
			KillGameObject(objectId);
	}

	// resolve game object - game object collision pairs
	for (const auto collidingPair : collidingPairs)
	{
		KillGameObject(collidingPair.first);
		KillGameObject(collidingPair.second);
	}

	// update the AI
	UpdateAI(time);
}


bool IsGameOver()
{
	return !any_of(begin(aliens), end(aliens), [] (const GameObject& alien) { return alien.isAlive; });
}


GameObject CreateBullet(const Vector2& position, const Vector2& velocity, CollisionLayer collisionLayer, CollisionLayer collisionMask)
{
	bullets.push_back(GameObject::CreateGameObject<GameObjectType::Bullet>());
	ObjectId objectId = bullets.back().objectId;

	auto& rigidBody = AddRigidBody(objectId, position, glm::normalize(velocity));
	rigidBody.velocity = velocity;
	//printf("Fire Bullet %llu at %f, %f with velocity %f, %f\n", rigidBody.objectId, rigidBody.bulletPosition.x, rigidBody.bulletPosition.y, rigidBody.velocity.x, rigidBody.velocity.y);

	auto& collisionObject = AddCollisionObject(objectId, Vector2 { 2.0f, 12.0f });
	collisionObject.layer = collisionLayer;
	collisionObject.layerMask = collisionMask;

	return bullets.back();
}

void FirePlayerBullet()
{
	const auto& playerRB = GetRigidBody(player.objectId);
	Vector2 fireOffset { 0.0f, 8.0f };
	Vector4 bulletPosition4 = CalculateObjectTransform(playerRB.position, playerRB.facing) * Vector4(fireOffset, 0.0f, 1.0f);
	Vector2 bulletPosition { bulletPosition4.x, bulletPosition4.y };
	const float bulletSpeed = 1200.0f;
	Vector2 bulletVelocity = bulletSpeed * playerRB.facing;
	CreateBullet(bulletPosition, bulletVelocity, CollisionLayer::PlayerBullet, CollisionLayer::Alien);
}


void CreateWall(const Vector2& startPosition, const Vector2& endPosition)
{
	RigidBody& playerRB = GetRigidBody(player.objectId);
	Vector2 playerPosition = playerRB.position;
	if (IsSimilar(startPosition.y, endPosition.y))
	{
		// horizontal wall
		if (startPosition.y < playerPosition.y)
		{
			minWorld.y = startPosition.y;
		}
		else
		{
			maxWorld.y = startPosition.y;
		}
	}
	else
	{
		//assert(IsSimilar(startPosition.x, endPosition.x));
		// vertical wall
		if (startPosition.x < playerPosition.x)
		{
			minWorld.x = startPosition.x;
		}
		else
		{
			maxWorld.x = startPosition.x;
		}
	}
}


vector<ObjectId> GetAliensInCircle(const Vector2& center, float radius)
{
	vector<ObjectId> nearby;
	nearby.reserve(aliens.size());

	for (auto& alien : aliens)
	{
		if (!alien.isAlive)
			continue;

		const auto& rigidBody = GetRigidBody(alien.objectId);
		float distance = glm::distance(rigidBody.position, center);
		if (distance <= radius)
		{
			nearby.push_back(alien.objectId);
		}
	}

	return nearby;
}

