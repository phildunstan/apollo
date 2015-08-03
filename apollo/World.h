#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "game.h"
#include "math_helpers.h"
#include "game_object.h"


extern const Vector2 minWorld;
extern const Vector2 maxWorld;

// World data
extern GameObject player;
extern std::vector<GameObject> bullets;
extern std::vector<GameObject> aliens;

GameObject& GetGameObject(ObjectId objectId);

void InitWorld();
void UpdateWorld(const Time& time);
bool IsGameOver();



void FirePlayerBullet();



#include "ai.h"

template <GameObjectType GameObjectTypeT>
GameObject GameObject::CreateGameObject()
{
	GameObject object;
	object.objectId = GetNextObjectId();
	object.type = GameObjectTypeT;
	object.aiModel = std::make_unique<AIModel<GameObjectTypeT>>();
	return object;
}


