#pragma once

#include <memory>
#include <vector>
#include "ai.h"

ObjectId GetNextObjectId();

enum class GameObjectType : int
{
	Player,
	Bullet,
	AlienRandom,
	AlienChase,
	AlienShy,
	AlienMothership,
	AlienOffspring,
	AlienWallHugger,
	Count
};

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


struct GameObjectMetaData
{
	GameObjectType type;
	const char* spriteFilename;
	float mass;
	Vector2 boundingBoxDimensions;
};

extern std::vector<GameObjectMetaData> gameObjectMetaDatas;

GameObjectMetaData& GetGameObjectMetaData(GameObjectType type);
void CreateGameObjectMetaData();
