#include "game_object.h"


ObjectId GetNextObjectId(GameObjectType type)
{
	static uint32_t counter = 1;
	return CreateObjectId(type, counter++);
}

std::vector<GameObjectMetaData> gameObjectMetaDatas;

GameObjectMetaData& GetGameObjectMetaData(GameObjectType type)
{
	return gameObjectMetaDatas[static_cast<size_t>(type)];
}

void CreateGameObjectMetaData()
{
	gameObjectMetaDatas.resize(static_cast<size_t>(GameObjectType::Count));
	GetGameObjectMetaData(GameObjectType::Player) = GameObjectMetaData { GameObjectType::Player, "apollo.png", 1.0f, Vector2 { 32.0f, 32.0f } };
	GetGameObjectMetaData(GameObjectType::Bullet) = GameObjectMetaData { GameObjectType::Bullet, "bullet.png", 1.0f, Vector2 { 3.0f, 9.0f } };
	GetGameObjectMetaData(GameObjectType::AlienRandom) = GameObjectMetaData { GameObjectType::AlienRandom, "enemy_random.png", 1.0f, Vector2 { 32.0f, 32.0f } };
	GetGameObjectMetaData(GameObjectType::AlienChase) = GameObjectMetaData { GameObjectType::AlienChase, "enemy_chase.png", 1.0f, Vector2 { 32.0f, 32.0f } };
	GetGameObjectMetaData(GameObjectType::AlienShy) = GameObjectMetaData { GameObjectType::AlienShy, "enemy_shy.png", 1.0f, Vector2 { 32.0f, 32.0f } };
	GetGameObjectMetaData(GameObjectType::AlienMothership) = GameObjectMetaData { GameObjectType::AlienMothership, "enemy_mothership.png", 1.0f, Vector2 { 64.0f, 64.0f } };
	GetGameObjectMetaData(GameObjectType::AlienOffspring) = GameObjectMetaData { GameObjectType::AlienOffspring, "enemy_offspring.png", 1.0f, Vector2 { 8.0f, 8.0f } };
	GetGameObjectMetaData(GameObjectType::AlienWallHugger) = GameObjectMetaData { GameObjectType::AlienWallHugger, "enemy_wallhugger.png", 1.0f, Vector2 { 24.0f, 24.0f } };
}

