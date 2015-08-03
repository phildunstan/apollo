#include <algorithm>

#include "glm/gtx/rotate_vector.hpp"

#include "physics.h"
#include "world.h"
#include "math_helpers.h"

using namespace std;

vector<RigidBody> rigidBodies;
vector<CollisionObject> collisionObjects;

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
