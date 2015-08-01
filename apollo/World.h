#pragma once

#include <vector>
#include <cassert>

#include "math_helpers.h"

struct Time
{
	float elapsedTime;
	float deltaTime;
};

using ObjectId = uint64_t;
ObjectId GetNextObjectId();

struct GameObject
{
	GameObject()
		: objectId { GetNextObjectId() }
	{
	}

	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	ObjectId objectId;
	bool isAlive { true };
	float timeOfLastShot { 0.0f };
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

enum class CollisionLayer : uint32_t { None = 0, Player = 1, PlayerBullet = 2, Alien = 4, All = 0xffff, PendingDestruction = 0x80000000 };

inline CollisionLayer operator&(CollisionLayer lhs, CollisionLayer rhs)
{
	return static_cast<CollisionLayer>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline CollisionLayer operator|(CollisionLayer lhs, CollisionLayer rhs)
{
	return static_cast<CollisionLayer>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

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

	CollisionLayer layer { CollisionLayer::None };
	CollisionLayer layerMask { CollisionLayer::All }; // all of the layers this collision object collides with
};


// World data
extern GameObject player;
extern std::vector<GameObject> bullets;
extern std::vector<GameObject> aliens;
extern std::vector<RigidBody> rigidBodies;
extern std::vector<CollisionObject> collisionObjects;

void InitWorld();
void UpdateWorld(const Time& time);

void FirePlayerBullet();

