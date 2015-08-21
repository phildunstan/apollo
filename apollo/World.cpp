#include "world.h"

#include <iterator>
#include <algorithm>
#include <unordered_set>

#include "physics.h"
#include "player.h"
#include "math_helpers.h"
#include "ai.h"

using namespace std;

const Vector2 minWorld { -320.0f, -240.0f };
const Vector2 maxWorld { 320.0f, 240.0f };

GameObject player { GameObject::CreateGameObject<GameObjectType::Player>() };
vector<GameObject> bullets;
vector<GameObject> aliens;

ObjectId GetNextObjectId()
{
	static ObjectId objectId = 1;
	return objectId++;
}


void CreatePlayerGameObject()
{
	assert(player.objectId != 0);
	AddRigidBody(player.objectId, Vector2 { 50.0f, 20.0f }, Vector2 { 1.0f, 0.0f });
	auto& collisionObject = AddCollisionObject(player.objectId, Vector2 { 32.0f, 32.0f });
	collisionObject.layer = CollisionLayer::Player;
	collisionObject.layerMask = CollisionLayer::Alien;
}


GameObject& GetGameObject(ObjectId objectId)
{
	if (player.objectId == objectId)
		return player;
	// we can use a binary search if we can guarantee that elements are only added to the RigidBody vectors
	// in increasing ObjectId order, and that the vectors are never reordered.
	auto enemyIter = lower_bound(begin(aliens), end(aliens), objectId, [] (const auto& enemy, auto objectId) { return enemy.objectId < objectId; });
	if ((enemyIter != end(aliens)) && (enemyIter->objectId == objectId))
		return *enemyIter;
	auto bulletIter = lower_bound(begin(bullets), end(bullets), objectId, [] (const auto& bullet, auto objectId) { return bullet.objectId < objectId; });
	assert((bulletIter != end(bullets)) && (bulletIter->objectId == objectId));
	return *bulletIter;
}


Vector2 findGoodPlaceToSpawnAlien()
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

	return position;
}


void CreateAlienGameObject()
{
	float random = GetRandomFloat01();
	if (random < 0.5f)
	{
		aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienShy>());
		aliens.back().aiModel = std::make_unique<AIModelAlienShy>();
	}
	else if (random < 0.75f)
	{
		aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienChase>());
		aliens.back().aiModel = std::make_unique<AIModelAlienChase>();
	}
	else
	{
		aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienRandom>());
		aliens.back().aiModel = std::make_unique<AIModelAlienRandom>();
	}

	Vector2 position = findGoodPlaceToSpawnAlien();
	Vector2 facing = GetRandomVectorOnCircle();
	float spinSpeed = GetRandomFloat01();
	auto& rigidBody = AddRigidBody(aliens.back().objectId, position, facing);
	rigidBody.angularVelocity = 20.0f * (spinSpeed - 0.5f);
	auto& collisionObject = AddCollisionObject(aliens.back().objectId, Vector2 { 32.0f, 32.0f });
	collisionObject.layer = CollisionLayer::Alien;
	collisionObject.layerMask = CollisionLayer::Player | CollisionLayer::PlayerBullet;
}


void InitWorld()
{
	CreatePlayerGameObject();

	bullets.reserve(100);
	aliens.reserve(100);

	// create a bunch of aliens to shoot
	const int numAliens = 10;
	for (int i = 0; i < numAliens; ++i)
	{
		CreateAlienGameObject();
	}
}


void KillGameObject(GameObject& gameObject)
{
	IncrementPlayerScore(gameObject);
	gameObject.isAlive = false;
	auto& collisionObject = GetCollisionObject(gameObject.objectId);
	collisionObject.layer = CollisionLayer::PendingDestruction;
	collisionObject.layerMask = CollisionLayer::None;
}


void UpdateAI(const Time& time)
{
	for_each(begin(aliens), end(aliens), [&time] (GameObject& alien) {
		if (alien.isAlive)
		{
			alien.aiModel->Update(time, alien);
		}
	});
}


void UpdateWorld(const Time& time)
{
	UpdateRigidBodies(time);

	EnsurePlayerIsInsideWorldBounds();

	auto collidingObjects = UpdateCollision(time);

	// resolve objects that have collided
	player.isAlive = !binary_search(begin(collidingObjects), end(collidingObjects), player.objectId);
	for (auto& enemy : aliens)
		if (binary_search(begin(collidingObjects), end(collidingObjects), enemy.objectId))
			KillGameObject(enemy);
	for (auto& bullet : bullets)
		if (binary_search(begin(collidingObjects), end(collidingObjects), bullet.objectId))
			KillGameObject(bullet);

	// update the AI
	UpdateAI(time);
}


bool IsGameOver()
{
	return !any_of(begin(aliens), end(aliens), [] (const GameObject& alien) { return alien.isAlive; });
}


GameObject& CreateBullet(const Vector2& position, const Vector2& velocity, CollisionLayer collisionLayer, CollisionLayer collisionMask)
{
	bullets.push_back(GameObject::CreateGameObject<GameObjectType::Bullet>());
	auto& rigidBody = AddRigidBody(bullets.back().objectId, position, glm::normalize(velocity));
	rigidBody.velocity = velocity;
	//printf("Fire Bullet %llu at %f, %f with velocity %f, %f\n", rigidBody.objectId, rigidBody.bulletPosition.x, rigidBody.bulletPosition.y, rigidBody.velocity.x, rigidBody.velocity.y);

	auto& collisionObject = AddCollisionObject(bullets.back().objectId, Vector2 { 2.0f, 12.0f });
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

