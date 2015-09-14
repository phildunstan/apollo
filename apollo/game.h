#pragma once

enum class GameObjectType : uint8_t
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


using ObjectId = uint32_t;

inline ObjectId CreateObjectId(GameObjectType type, uint32_t index) { return (static_cast<uint32_t>(type) << 24) | (index & 0xffffff); }
inline GameObjectType GetType(ObjectId objectId) { return static_cast<GameObjectType>(objectId >> 24); }
inline uint32_t GetIndex(ObjectId objectId) { return static_cast<uint32_t>(objectId & 0xffffff); }

ObjectId GetNextObjectId(GameObjectType type);


struct Time
{
	float elapsedTime { 0.0f };
	float deltaTime { 0.0f };
};

inline bool operator==(const Time& lhs, const Time& rhs)
{
	return (lhs.elapsedTime == rhs.elapsedTime) && (lhs.deltaTime == rhs.deltaTime);
}
