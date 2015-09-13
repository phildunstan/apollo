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

	GameObject() = default;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	GameObject(const GameObject& other)
		: objectId { other.objectId }
		, type { other.type }
		, isAlive { other.isAlive }
		, timeOfLastShot { other.timeOfLastShot }
	{
	}

	const GameObject& operator=(const GameObject& other)
	{
		objectId = other.objectId;
		type = other.type;
		isAlive = other.isAlive;
		timeOfLastShot = other.timeOfLastShot;
		return *this;
	}

	ObjectId objectId { 0 };
	GameObjectType type { GameObjectType::Player };
	bool isAlive { true };
	float timeOfLastShot { 0.0f };
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
