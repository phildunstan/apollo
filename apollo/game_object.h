#pragma once

#include <memory>

using ObjectId = uint64_t;
ObjectId GetNextObjectId();

enum class GameObjectType : int { Player, Bullet, AlienRandom, AlienChase, AlienShy };

class AIModelBase;

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

	std::unique_ptr<AIModelBase> aiModel;

private:
	GameObject() {}
};