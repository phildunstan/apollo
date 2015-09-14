#pragma once

#include <memory>
#include <tuple>
#include <vector>
#include <cassert>

#include "game.h"
#include "math_helpers.h"
#include "game_object.h"
#include "physics.h"


extern Vector2 minWorld;
extern Vector2 maxWorld;

// World data
extern GameObject player;
extern std::vector<GameObject> bullets;
extern std::vector<GameObject> aliens;

GameObject& GetGameObject(ObjectId objectId);

void InitWorld();
void UpdateWorld(const Time& time);
bool IsGameOver();

GameObject& CreateBullet(const Vector2& position, const Vector2& velocity, CollisionLayer collisionLayer, CollisionLayer collisionMask);
void FirePlayerBullet();

template <GameObjectType GameObjectTypeT>
GameObject GameObject::CreateGameObject()
{
	GameObject object;
	object.objectId = GetNextObjectId(GameObjectTypeT);
	return object;
}

void CreateWall(const Vector2& startPosition, const Vector2& endPosition);

std::vector<ObjectId> GetAliensInCircle(const Vector2& center, float radius);

float GetPositionAlongWallCoordFromPositionAndFacing(const Vector2& position, const Vector2& facing);
std::tuple<Vector2, Vector2> GetPositionAndFacingFromWallCoord(float p, const Vector2& collisionBoxDimensions);
