#pragma once

#include "game_object.h"
#include "physics.h"

const float alienSpeed = 40.0f;

class AIModelBase
{
public:
	AIModelBase() = default;
	virtual ~AIModelBase() = default;
	virtual void Update(const Time& time, GameObject& gameObject) = 0;

	AIModelBase(const AIModelBase&) = delete;
	AIModelBase& operator=(const AIModelBase&) = delete;
};


template <GameObjectType GameObjectTypeT>
class AIModel : public AIModelBase
{
public:
	AIModel() {}
	void Update(const Time& /*time*/, GameObject& /*gameObject*/) override {}
};


template <>
class AIModel<GameObjectType::AlienRandom> : public AIModelBase
{
public:
	AIModel() {}
	void Update(const Time& time, GameObject& alien) override
	{
		if (time.elapsedTime - timeOfLastMovementChange > 1.0f)
		{
			auto& rigidBody = GetRigidBody(alien.objectId);
			rigidBody.velocity = alienSpeed * GetRandomVectorOnCircle();
			timeOfLastMovementChange = time.elapsedTime;
		}
	}

	float timeOfLastMovementChange { 0.0f };
};


template <>
class AIModel<GameObjectType::AlienChase> : public AIModelBase
{
public:
	AIModel() {}
	void Update(const Time& time, GameObject& alien) override
	{
		if (time.elapsedTime - timeOfLastMovementChange > 1.0f)
		{
			const auto& playerRigidBody = GetRigidBody(player.objectId);
			Vector2 playerPosition = playerRigidBody.position;

			auto& rigidBody = GetRigidBody(alien.objectId);
			rigidBody.velocity = alienSpeed * glm::normalize(playerPosition - rigidBody.position);
			timeOfLastMovementChange = time.elapsedTime;
		}
	}

	float timeOfLastMovementChange { 0.0f };
};


template <>
class AIModel<GameObjectType::AlienShy> : public AIModelBase
{
public:
	AIModel() {}
	void Update(const Time& time, GameObject& alien) override
	{
		if (time.elapsedTime - timeOfLastMovementChange > 1.0f)
		{
			auto& rigidBody = GetRigidBody(alien.objectId);
			const auto& playerRigidBody = GetRigidBody(player.objectId);
			Vector2 playerFacing = playerRigidBody.facing;
			Vector2 playerPosition = playerRigidBody.position;

			if (glm::dot(playerFacing, rigidBody.position - playerPosition) > 0)
			{
				rigidBody.velocity = alienSpeed * GetRandomVectorOnCircle();
			}
			else
			{
				rigidBody.velocity = alienSpeed * glm::normalize(playerPosition - rigidBody.position);
			}
			timeOfLastMovementChange = time.elapsedTime;
		}
	}

	float timeOfLastMovementChange { 0.0f };
};







