#include "World.h"

#include <algorithm>
#include <unordered_set>

#include "glm/gtx/rotate_vector.hpp"

#include "math_helpers.h"

using namespace std;

GameObject player;
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

void InitWorld()
{
	assert(player.objectId != 0);
	AddRigidBody(player.objectId, Vector2 { 50.0f, 20.0f }, Vector2 { 1.0f, 0.0f });
	AddCollisionObject(player.objectId, Vector2 { 32.0f, 32.0f });

	bullets.reserve(100);
	aliens.reserve(100);

	// create a bunch of aliens to shoot
	for (int i = 0; i < 10; ++i)
	{
		aliens.emplace_back();
		Vector2 position = findGoodPlaceToSpawnAlien();
		Vector2 facing = GetRandomVectorOnCircle();
		float spinSpeed = GetRandomFloat01();
		RigidBody& rigidBody = AddRigidBody(aliens.back().objectId, position, facing);
		rigidBody.angularVelocity = 2.0f * (spinSpeed - 0.5f);
		AddCollisionObject(aliens.back().objectId, Vector2 { 32.0f, 32.0f });
	}
}



void UpdateWorld(float deltaTime)
{
	for_each(begin(rigidBodies), end(rigidBodies), [deltaTime] (RigidBody& rigidBody)
	{
		rigidBody.position += rigidBody.velocity * deltaTime;
		rigidBody.facing = glm::rotate(rigidBody.facing, rigidBody.angularVelocity * deltaTime);
	});

	// update the collision world
	for_each(begin(collisionObjects), end(collisionObjects), [] (CollisionObject& collisionObject)
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
	for (auto& enemy : aliens)
		if (collidingObjects.find(enemy.objectId) != collidingObjects.end())
			enemy.isAlive = false;
	for (auto& bullet : bullets)
		if (collidingObjects.find(bullet.objectId) == collidingObjects.end())
			bullet.isAlive = false;
}


void FirePlayerBullet()
{
	const auto& playerRB = GetRigidBody(player.objectId);
	bullets.emplace_back();
	auto& rigidBody = AddRigidBody(bullets.back().objectId, playerRB.position + playerRB.facing * 8.0f, playerRB.facing);
	const float bulletSpeed = 1200.0f;
	rigidBody.velocity = bulletSpeed * rigidBody.facing;
	//printf("Fire Bullet %llu at %f, %f with velocity %f, %f\n", rigidBody.objectId, rigidBody.position.x, rigidBody.position.y, rigidBody.velocity.x, rigidBody.velocity.y);

	AddCollisionObject(bullets.back().objectId, Vector2 { 2.0f, 12.0f });
}


