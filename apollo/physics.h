#pragma once

#include <utility>
#include <vector>

#include "game.h"
#include "math_helpers.h"
#include "game_object.h"

struct RigidBody
{
	RigidBody() = default;
	RigidBody(ObjectId _objectId, const Vector2& _position, const Vector2& _facing)
		: objectId { _objectId }
		, position { _position }
		, facing { _facing }
	{
		assert(IsUnitLength(facing));
	}

	RigidBody(RigidBody&&) = default;
	RigidBody(const RigidBody&) = default;
	RigidBody& operator=(RigidBody&&) = default;
	RigidBody& operator=(const RigidBody&) = default;

	ObjectId objectId { 0 };
	Vector2 position { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
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
	CollisionObject() = default;
	CollisionObject(ObjectId _objectId, const Vector2& _aabbDimensions)
		: objectId(_objectId)
		, boundingBoxDimensions(_aabbDimensions)
	{
	}

	CollisionObject(CollisionObject&&) = default;
	CollisionObject(const CollisionObject&) = default;
	CollisionObject& operator=(CollisionObject&&) = default;
	CollisionObject& operator=(const CollisionObject&) = default;

	ObjectId objectId { 0 };
	Vector2 boundingBoxDimensions { 0.0f, 0.0f };
	Vector2 position { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 1.0f };

	CollisionLayer layer { CollisionLayer::None };
	CollisionLayer layerMask { CollisionLayer::All }; // all of the layers this collision object collides with
};


extern std::vector<RigidBody> rigidBodies;
extern std::vector<CollisionObject> collisionObjects;


void InitPhysics();

RigidBody& AddRigidBody(ObjectId objectId, const Vector2& position, const Vector2& facing);
RigidBody& GetRigidBody(ObjectId objectId);
void UpdateRigidBodies(const Time& time);

void EnsurePlayerIsInsideWorldBounds();

bool BoundingBoxCollidesWithWorldEdge(const Vector2& position, const Vector2& facing, const Vector2& dimensions);
bool CollisionObjectCollidesWithWorldEdge(const CollisionObject& object);

CollisionObject& AddCollisionObject(ObjectId objectId, const Vector2& boundingBoxDimensions);
CollisionObject& GetCollisionObject(ObjectId objectId);
void UpdateCollision(const Time& time, std::vector<std::pair<ObjectId, ObjectId>>& collidingPairs, std::vector<ObjectId>& collidingWithWorld);


