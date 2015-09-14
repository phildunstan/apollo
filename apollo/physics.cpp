#include <algorithm>

#include "glm/gtx/rotate_vector.hpp"

#include "physics.h"
#include "world.h"
#include "math_helpers.h"
#include "profiler.h"

using namespace std;

vector<RigidBody> rigidBodies;
vector<CollisionObject> collisionObjects;

const int MAX_RIGID_BODIES = 1000;
const int MAX_COLLISION_OBJECTS = 1000;

void InitPhysics()
{
	rigidBodies.reserve(MAX_RIGID_BODIES);
	collisionObjects.reserve(MAX_COLLISION_OBJECTS);
}


RigidBody& GetRigidBody(ObjectId objectId)
{
	// we can use a binary search if we can guarantee that elements are never reordered.
	auto rigidBodyIter = lower_bound(begin(rigidBodies), end(rigidBodies), objectId, [] (const auto& rigidBody, auto objectId) { return GetIndex(rigidBody.objectId) < GetIndex(objectId); });
	assert((rigidBodyIter != end(rigidBodies)) && (GetIndex(rigidBodyIter->objectId) == GetIndex(objectId)));
	return *rigidBodyIter;
}

CollisionObject& GetCollisionObject(ObjectId objectId)
{
	// we can use a binary search if we can guarantee that elements are never reordered.
	auto collisionObjectIter = lower_bound(begin(collisionObjects), end(collisionObjects), objectId, [] (const auto& collisionObjects, auto objectId) { return GetIndex(collisionObjects.objectId) < GetIndex(objectId); });
	assert((collisionObjectIter != end(collisionObjects)) && (GetIndex(collisionObjectIter->objectId) == GetIndex(objectId)));
	return *collisionObjectIter;
}


vector<Vector2> GatherBoundingBoxVertices(const Vector2& position, const Vector2& facing, const Vector2& dimensions)
{
	Vector2 yAxis = facing;
	Vector2 xAxis = PerpendicularRightVector2D(yAxis);
	Vector2 halfDimensions = 0.5f * dimensions;
	vector<Vector2> vertices {
		position - halfDimensions.x * xAxis - halfDimensions.y * yAxis,
		position + halfDimensions.x * xAxis - halfDimensions.y * yAxis,
		position - halfDimensions.x * xAxis + halfDimensions.y * yAxis,
		position + halfDimensions.x * xAxis + halfDimensions.y * yAxis };
	return vertices;
}


vector<Vector2> GatherObjectVertices(const CollisionObject& object)
{
	return GatherBoundingBoxVertices(object.position, object.facing, object.boundingBoxDimensions);
}


bool CollisionObjectsCollide(const CollisionObject& objectA, const CollisionObject& objectB)
{
	PROFILER_TIMER_FUNCTION();

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
	edgeNormals.push_back(PerpendicularRightVector2D(objectA.facing));
	assert(IsUnitLength(objectB.facing));
	edgeNormals.push_back(objectB.facing);
	edgeNormals.push_back(PerpendicularRightVector2D(objectB.facing));

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


bool BoundingBoxCollidesWithWorldEdge(const Vector2& position, const Vector2& facing, const Vector2& dimensions)
{
	vector<Vector2> objectVertices = GatherBoundingBoxVertices(position, facing, dimensions);
	for (const auto& vertex : objectVertices)
	{
		if (!AABBContains(minWorld, maxWorld, vertex))
			return true;
	}
	return false;
}


bool CollisionObjectCollidesWithWorldEdge(const CollisionObject& object)
{
	PROFILER_TIMER_FUNCTION();

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
	assert(rigidBodies.size() < MAX_RIGID_BODIES);
	rigidBodies.push_back(RigidBody { objectId, position, facing });
	return rigidBodies.back();
}

CollisionObject& AddCollisionObject(ObjectId objectId, const Vector2& boundingBoxDimensions)
{
	assert(collisionObjects.size() < MAX_COLLISION_OBJECTS);
	collisionObjects.push_back(CollisionObject { objectId, boundingBoxDimensions });
	CollisionObject& collisionObject = collisionObjects.back();
	const RigidBody& rigidBody = GetRigidBody(objectId);
	collisionObject.position = rigidBody.position;
	collisionObject.facing = rigidBody.facing;
	return collisionObject;
}

void UpdateRigidBodies(const Time& time)
{
	PROFILER_TIMER_FUNCTION();

	// physics dynamic update
	float deltaTime = time.deltaTime;
	for_each(begin(rigidBodies), end(rigidBodies), [deltaTime] (RigidBody& rigidBody)
	{
		rigidBody.position += rigidBody.velocity * deltaTime;
		rigidBody.facing = glm::normalize(glm::rotate(rigidBody.facing, rigidBody.angularVelocity * deltaTime));
	});
}

void EnsurePlayerIsInsideWorldBounds()
{
	auto& playerRB = GetRigidBody(player.objectId);
	const auto& playerCollision = GetCollisionObject(player.objectId);
	vector<Vector2> objectVertices = GatherBoundingBoxVertices(playerRB.position, playerRB.facing, playerCollision.boundingBoxDimensions);
	Vector2 deltaRequired { 0.0f, 0.0f };
	for (const auto& vertex : objectVertices)
	{
		if (vertex.x < minWorld.x)
		{
			deltaRequired.x = max(deltaRequired.x, minWorld.x - vertex.x);
		}
		else if (vertex.x > maxWorld.x)
		{
			deltaRequired.x = min(deltaRequired.x, maxWorld.x - vertex.x);
		}
		if (vertex.y < minWorld.y)
		{
			deltaRequired.y = max(deltaRequired.y, minWorld.y - vertex.y);
		}
		else if (vertex.y > maxWorld.y)
		{
			deltaRequired.y = min(deltaRequired.y, maxWorld.y - vertex.y);
		}
	}
	if (deltaRequired != Vector2 { 0.0f, 0.0f })
	{
		playerRB.position = playerRB.position + deltaRequired;
	}
}

// return all of the objects that have been in a collision
void UpdateCollision(const Time& /*time*/, vector<pair<ObjectId, ObjectId>>& collidingPairs, vector<ObjectId>& collidingWithWorld)
{
	PROFILER_TIMER_FUNCTION();

	// update collision objects from rigid bodies
	for_each(begin(collisionObjects), end(collisionObjects), [] (CollisionObject& collisionObject)
	{
		const auto& rigidBody = GetRigidBody(collisionObject.objectId);
		collisionObject.position = rigidBody.position;
		collisionObject.facing = rigidBody.facing;
	});

	// collision tests between every collision object
	collidingWithWorld.clear();
	collidingWithWorld.reserve(collisionObjects.size());
	collidingPairs.clear();
	collidingPairs.reserve(collisionObjects.size());

	for (int i = 0; i < collisionObjects.size(); ++i)
	{
		auto& collisionObjectI = collisionObjects[i];
		if (collisionObjectI.layer == CollisionLayer::PendingDestruction)
			continue;

		if (CollisionObjectCollidesWithWorldEdge(collisionObjectI))
		{
			collidingWithWorld.push_back(collisionObjectI.objectId);
		}

		for (int j = i + 1; j < collisionObjects.size(); ++j)
		{
			auto& collisionObjectJ = collisionObjects[j];
			if (collisionObjectJ.layer == CollisionLayer::PendingDestruction)
				continue;
			if (CollisionObjectsCollide(collisionObjectI, collisionObjectJ))
			{
				collidingPairs.push_back(make_pair(collisionObjectI.objectId, collisionObjectJ.objectId));
			}
		}
	}

	//sort(begin(collidingPairs), end(collidingPairs));
	//unique(begin(collidingPairs), end(collidingPairs));
}
