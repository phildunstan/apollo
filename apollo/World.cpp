#include "World.h"

#include <algorithm>
#include <unordered_set>

#include "glm/gtx/rotate_vector.hpp"

#include "math_helpers.h"

using namespace std;

GameObject player { GameObject::CreateGameObject<GameObjectType::Player>() };
vector<GameObject> bullets;
vector<GameObject> aliens;
vector<RigidBody> rigidBodies;
vector<CollisionObject> collisionObjects;

const Vector2 minWorld { -320.0f, -240.0f };
const Vector2 maxWorld { 320.0f, 240.0f };

ObjectId GetNextObjectId()
{
	static ObjectId objectId = 1;
	return objectId++;
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


RigidBody& GetRigidBody(ObjectId objectId)
{
	// we can use a binary search if we can guarantee that elements are only added to the RigidBody vectors
	// in increasing ObjectId order, and that the vectors are never reordered.
	auto rigidBodyIter = lower_bound(begin(rigidBodies), end(rigidBodies), objectId, [] (const auto& rigidBody, auto objectId) { return rigidBody.objectId < objectId; });
	assert((rigidBodyIter != end(rigidBodies)) && (rigidBodyIter->objectId == objectId));
	return *rigidBodyIter;
}

CollisionObject& GetCollisionObject(ObjectId objectId)
{
	// we can use a binary search if we can guarantee that elements are only added to the RigidBody vectors
	// in increasing ObjectId order, and that the vectors are never reordered.
	auto collisionObjectIter = lower_bound(begin(collisionObjects), end(collisionObjects), objectId, [] (const auto& collisionObjects, auto objectId) { return collisionObjects.objectId < objectId; });
	assert((collisionObjectIter != end(collisionObjects)) && (collisionObjectIter->objectId == objectId));
	return *collisionObjectIter;
}


vector<Vector2> GatherObjectVertices(const CollisionObject& object)
{
	Vector2 yAxis = object.facing;
	Vector2 xAxis = PerpendicularVector2D(yAxis);
	Vector2 halfDimensions = 0.5f * object.aabbDimensions;
	vector<Vector2> vertices {
		object.position - halfDimensions.x * xAxis - halfDimensions.y * yAxis,
		object.position + halfDimensions.x * xAxis - halfDimensions.y * yAxis,
		object.position - halfDimensions.x * xAxis + halfDimensions.y * yAxis,
		object.position + halfDimensions.x * xAxis + halfDimensions.y * yAxis };
	return vertices;
}


bool CollisionObjectsCollide(const CollisionObject& objectA, const CollisionObject& objectB)
{
	// this check really isn't symmetric as we don't check A against B and B against A at the calling site
	if (((objectA.layerMask & objectB.layer) == CollisionLayer::None) && ((objectB.layerMask & objectA.layer) == CollisionLayer::None))
	{
		return false;
	}

	// use the Separating Axis Theorem to check for collision

	// collect the vertices for objects A and B
	vector<Vector2> objectAVertices = GatherObjectVertices(objectA);
	vector<Vector2> objectBVertices = GatherObjectVertices(objectB);

	// collect the normal vectors for each edge in objects A and B
	vector<Vector2> edgeNormals;
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

bool AABBContains(const Vector2& aabbMin, const Vector2& aabbMax, const Vector2& point)
{
	return (point.x >= aabbMin.x) && (point.x <= aabbMax.x) && (point.y >= aabbMin.y) && (point.y <= aabbMax.y);
}

bool CollisionObjectCollidesWithWorldEdge(const CollisionObject& object)
{
	vector<Vector2> objectVertices = GatherObjectVertices(object);
	for (const auto& vertex : objectVertices)
	{
		if (!AABBContains(minWorld, maxWorld, vertex))
			return true;
	}
	return false;
}



RigidBody& AddRigidBody(ObjectId objectId, const Vector2& position, const Vector2& facing)
{
	rigidBodies.push_back(RigidBody { objectId, position, facing });
	return rigidBodies.back();
}

CollisionObject& AddCollisionObject(ObjectId objectId, const Vector2& aabbDimensions)
{
	collisionObjects.push_back(CollisionObject { objectId, aabbDimensions });
	CollisionObject& collisionObject = collisionObjects.back();
	const RigidBody& rigidBody = GetRigidBody(objectId);
	collisionObject.position = rigidBody.position;
	collisionObject.facing = rigidBody.facing;
	return collisionObject;
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

void CreatePlayerGameObject()
{
	assert(player.objectId != 0);
	AddRigidBody(player.objectId, Vector2 { 50.0f, 20.0f }, Vector2 { 1.0f, 0.0f });
	auto& collisionObject = AddCollisionObject(player.objectId, Vector2 { 32.0f, 32.0f });
	collisionObject.layer = CollisionLayer::Player;
	collisionObject.layerMask = CollisionLayer::Alien;
}

void CreateAlienGameObject()
{
	float random = GetRandomFloat01();
	if (random < 0.5f)
		aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienShy>());
	else if (random < 0.75f)
		aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienChase>());
	else
		aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienRandom>());
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

void KillObject(GameObject& gameObject)
{
	gameObject.isAlive = false;
	auto& collisionObject = GetCollisionObject(gameObject.objectId);
	collisionObject.layer = CollisionLayer::PendingDestruction;
	collisionObject.layerMask = CollisionLayer::None;
}

void UpdateRigidBodies(const Time& time)
{
	// physics dynamic update
	float deltaTime = time.deltaTime;
	for_each(begin(rigidBodies), end(rigidBodies), [deltaTime] (RigidBody& rigidBody)
	{
		rigidBody.position += rigidBody.velocity * deltaTime;
		rigidBody.facing = glm::normalize(glm::rotate(rigidBody.facing, rigidBody.angularVelocity * deltaTime));
	});
}

// return all of the objects that have been in a collision
vector<ObjectId> UpdateCollision(const Time& /*time*/)
{
	// update collision objects from rigid bodies
	for_each(begin(collisionObjects), end(collisionObjects), [] (CollisionObject& collisionObject)
	{
		const auto& rigidBody = GetRigidBody(collisionObject.objectId);
		collisionObject.position = rigidBody.position;
		collisionObject.facing = rigidBody.facing;
	});

	// collision tests between every collision object
	vector<ObjectId> collidingObjects;
	collisionObjects.reserve(collisionObjects.size());

	for (int i = 0; i < collisionObjects.size(); ++i)
	{
		auto& collisionObjectI = collisionObjects[i];
		if (collisionObjectI.layer == CollisionLayer::PendingDestruction)
			continue;
		if (CollisionObjectCollidesWithWorldEdge(collisionObjectI))
		{
			collidingObjects.push_back(collisionObjectI.objectId);
		}
		for (int j = i + 1; j < collisionObjects.size(); ++j)
		{
			auto& collisionObjectJ = collisionObjects[j];
			if (collisionObjectJ.layer == CollisionLayer::PendingDestruction)
				continue;
			if (CollisionObjectsCollide(collisionObjectI, collisionObjectJ))
			{
				collidingObjects.push_back(collisionObjectI.objectId);
				collidingObjects.push_back(collisionObjectJ.objectId);
			}
		}
	}

	sort(begin(collidingObjects), end(collidingObjects));
	unique(begin(collidingObjects), end(collidingObjects));

	return collidingObjects;
}


void UpdateAI(const Time& time)
{
	for_each(begin(aliens), end(aliens), [&time] (GameObject& alien) {
		alien.aiModel->Update(time, alien);
	});
}


void UpdateWorld(const Time& time)
{
	UpdateRigidBodies(time);
	auto collidingObjects = UpdateCollision(time);

	// resolve objects that have collided
	player.isAlive = !binary_search(begin(collidingObjects), end(collidingObjects), player.objectId);
	for (auto& enemy : aliens)
		if (binary_search(begin(collidingObjects), end(collidingObjects), enemy.objectId))
			KillObject(enemy);
	for (auto& bullet : bullets)
		if (binary_search(begin(collidingObjects), end(collidingObjects), bullet.objectId))
			KillObject(bullet);

	// update the AI
	UpdateAI(time);
}


void FirePlayerBullet()
{
	const auto& playerRB = GetRigidBody(player.objectId);
	bullets.push_back(GameObject::CreateGameObject<GameObjectType::Bullet>());
	auto& rigidBody = AddRigidBody(bullets.back().objectId, playerRB.position + playerRB.facing * 8.0f, playerRB.facing);
	const float bulletSpeed = 1200.0f;
	rigidBody.velocity = bulletSpeed * rigidBody.facing;
	//printf("Fire Bullet %llu at %f, %f with velocity %f, %f\n", rigidBody.objectId, rigidBody.position.x, rigidBody.position.y, rigidBody.velocity.x, rigidBody.velocity.y);

	auto& collisionObject = AddCollisionObject(bullets.back().objectId, Vector2 { 2.0f, 12.0f });
	collisionObject.layer = CollisionLayer::PlayerBullet;
	collisionObject.layerMask = CollisionLayer::Alien;
}


