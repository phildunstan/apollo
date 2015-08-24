#pragma once

#include <memory>
#include "ai.h"

using ObjectId = uint64_t;
ObjectId GetNextObjectId();

enum class GameObjectType : int { Player, Bullet, AlienRandom, AlienChase, AlienShy, AlienMothership, AlienOffspring };

struct GameObject
{
	template <GameObjectType GameObjectTypeT>
	static GameObject CreateGameObject();

	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;

	ObjectId objectId;
	GameObjectType type;
	bool isAlive { true };
	float timeOfLastShot { 0.0f };

	std::unique_ptr<AIModel> aiModel;

private:
	GameObject() {}
};