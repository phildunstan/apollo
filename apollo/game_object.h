#pragma once

#include <memory>
#include <vector>

#include "game.h"
#include "math_helpers.h"



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
	{
	}

	const GameObject& operator=(const GameObject& other)
	{
		objectId = other.objectId;
		type = other.type;
		isAlive = other.isAlive;
		return *this;
	}

	ObjectId objectId { };
	GameObjectType type { GameObjectType::Player };
	bool isAlive { true };
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
