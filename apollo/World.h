#pragma once

#include <vector>
#include <cassert>

#include "math_helpers.h"

using ObjectId = uint64_t;
ObjectId GetNextObjectId();

struct GameObject
{
	GameObject()
		: objectId { GetNextObjectId() }
		, isAlive { true }
	{
	}

	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	ObjectId objectId;
	bool isAlive;
};

GameObject& GetGameObject(ObjectId objectId);


struct RigidBody
{
	RigidBody(ObjectId _objectId, const Vector2& _position, const Vector2& _facing)
		: objectId { _objectId }
		, position { _position }
		, facing { _facing }
	{
		assert(IsUnitLength(facing));
	}

	RigidBody(RigidBody&&) = default;
	RigidBody& operator=(RigidBody&&) = default;

	RigidBody(const RigidBody&) = delete;
	RigidBody& operator=(const RigidBody&) = delete;

	ObjectId objectId;
	Vector2 position;
	Vector2 facing;
	Vector2 velocity { 0.0f, 0.0f };
	float angularVelocity { 0.0f };
};

RigidBody& GetRigidBody(ObjectId objectId);


struct CollisionObject
{
	CollisionObject(ObjectId _objectId, const Vector2& _aabbDimensions)
		: objectId(_objectId)
		, aabbDimensions(_aabbDimensions)
	{
	}

	CollisionObject(CollisionObject&&) = default;
	CollisionObject& operator=(CollisionObject&&) = default;

	CollisionObject(const CollisionObject&) = delete;
	CollisionObject& operator=(const CollisionObject&) = delete;

	ObjectId objectId;
	Vector2 aabbDimensions;
	Vector2 position { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 1.0f };
};


// World data
extern GameObject player;
extern std::vector<GameObject> bullets;
extern std::vector<GameObject> aliens;
extern std::vector<RigidBody> rigidBodies;
extern std::vector<CollisionObject> collisionObjects;

void InitWorld();
void UpdateWorld(float deltaTime);

void FirePlayerBullet();

