#include <vector>

#include "ai.h"
#include "game.h"
#include "game_object.h"
#include "physics.h"
#include "world.h"

using namespace std;

std::unique_ptr<AIModel> CreateAI(GameObject& gameObject)
{
	switch (gameObject.type)
	{
	case GameObjectType::Player:
		return nullptr;
	case GameObjectType::Bullet:
		return nullptr;
	case GameObjectType::AlienRandom:
		return std::make_unique<AIModelAlienRandom>(gameObject);
	case GameObjectType::AlienChase:
		return std::make_unique<AIModelAlienChase>(gameObject);
	case GameObjectType::AlienShy:
		return std::make_unique<AIModelAlienShy>(gameObject);
	case GameObjectType::AlienMothership:
		return std::make_unique<AIModelAlienMothership>(gameObject);
	case GameObjectType::AlienOffspring:
		return std::make_unique<AIModelAlienOffspring>(gameObject);
	};
	return nullptr;
}




void UpdateRandomVelocity(GameObject& alien, RigidBody& rigidBody, const Time& /*time*/, float lookAheadTime)
{
	auto& collisionObject = GetCollisionObject(alien.objectId);
	bool validMoveTarget = false;
	do
	{
		Vector2 newDirection = GetRandomVectorOnCircle();
		rigidBody.velocity = maxAlienSpeed * newDirection;
		Vector2 futurePosition = rigidBody.position + rigidBody.velocity * lookAheadTime;
		validMoveTarget = !BoundingBoxCollidesWithWorldEdge(futurePosition, newDirection, collisionObject.aabbDimensions);
	} while (!validMoveTarget);
}


void UpdateChaseVelocity(GameObject& alien, RigidBody& rigidBody, const Time& time)
{
	Vector2 forces { 0.0f, 0.0f };

	// the chase enemy is attracted by the player
	const float playerAttraction = 1000.0f;
	const auto& playerRigidBody = GetRigidBody(player.objectId);
	Vector2 playerPosition = playerRigidBody.position;
	forces += playerAttraction * glm::normalize(playerPosition - rigidBody.position);

	// the chase enemy is repelled by other nearby enemies
	auto nearbyAliens = GetAliensInCircle(rigidBody.position, 50.0f);
	for (ObjectId nearbyAlienId : nearbyAliens)
	{
		if (nearbyAlienId == alien.objectId)
			continue;

		const auto& nearbyAlienRB = GetRigidBody(nearbyAlienId);
		const float alienRepulsion = 1000.0f;
		forces += alienRepulsion * glm::normalize(rigidBody.position - nearbyAlienRB.position);
	}

	const float velocityDampening = 0.5f;
	const float mass = 1.0f;
	rigidBody.velocity = (1.0f - velocityDampening) * rigidBody.velocity + (forces / mass) * time.deltaTime;
	float speed = glm::length(rigidBody.velocity);
	if (speed > maxAlienSpeed)
	{
		rigidBody.velocity = (maxAlienSpeed / speed) * rigidBody.velocity;
	}
}


AIModelAlienRandom::AIModelAlienRandom(GameObject& alien)
	: alien(alien)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	rigidBody.angularVelocity = 20.0f * (GetRandomFloat01() - 0.5f);
}

void AIModelAlienRandom::Update(const Time & time)
{
	auto& rigidBody = GetRigidBody(alien.objectId);

	const float maxTimeBetweenMovementChanges = 1.0f;
	if (time.elapsedTime - timeOfLastMovementChange > maxTimeBetweenMovementChanges)
	{
		UpdateRandomVelocity(alien, rigidBody, time, maxTimeBetweenMovementChanges);
		timeOfLastMovementChange = time.elapsedTime;
	}

	if (time.elapsedTime - timeOfLastShot > 1.2f)
	{
		const auto& playerRigidBody = GetRigidBody(player.objectId);
		Vector2 directionTowardsPlayer = glm::normalize(playerRigidBody.position - rigidBody.position);
		Vector2 bulletPosition = rigidBody.position + 8.0f * directionTowardsPlayer;
		const float bulletSpeed = 100.0f;
		Vector2 bulletVelocity = directionTowardsPlayer * bulletSpeed;
		CreateBullet(bulletPosition, bulletVelocity, CollisionLayer::Alien, CollisionLayer::Player);
		timeOfLastShot = time.elapsedTime;
	}
}

AIModelAlienShy::AIModelAlienShy(GameObject& alien)
	: alien(alien)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	rigidBody.angularVelocity = 20.0f * (GetRandomFloat01() - 0.5f);
}

void AIModelAlienShy::Update(const Time & time)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	const float maxTimeBetweenMovementChanges = 1.0f;
	if (time.elapsedTime - timeOfLastMovementChange > maxTimeBetweenMovementChanges)
	{
		const auto& playerRigidBody = GetRigidBody(player.objectId);
		Vector2 playerFacing = playerRigidBody.facing;
		Vector2 playerPosition = playerRigidBody.position;

		// check if the player is facing towards this enemy
		if (glm::dot(playerFacing, rigidBody.position - playerPosition) > 0)
		{
			// if they are then choose a random direction to move in
			UpdateRandomVelocity(alien, rigidBody, time, maxTimeBetweenMovementChanges);
			timeOfLastMovementChange = time.elapsedTime;
		}
		else
		{
			UpdateChaseVelocity(alien, rigidBody, time);
		}
		timeOfLastMovementChange = time.elapsedTime;
	}
}


AIModelAlienChase::AIModelAlienChase(GameObject & alien)
	: alien(alien)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	rigidBody.angularVelocity = 20.0f * (GetRandomFloat01() - 0.5f);
}

void AIModelAlienChase::Update(const Time & time)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	UpdateChaseVelocity(alien, rigidBody, time);
}


AIModelAlienMothership::AIModelAlienMothership(GameObject& alien)
	: alien(alien)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	rigidBody.angularVelocity = (GetRandomFloat01() < 0.5f ? -1.0f : 1.0f) * 2.0f;
}

void AIModelAlienMothership::Update(const Time& time)
{
	const float timeBetweenWaves = 5.0f;
	const float timeBetweenLaunchesInWave = 0.02f;
	const int numberOfLaunchesPerWave = 10;

	float timeSinceLastLaunch = time.elapsedTime - timeOfLastLaunch;
	if ((currentMode == LaunchingMode::Waiting) && (timeSinceLastLaunch >= timeBetweenWaves))
	{
		currentMode = LaunchingMode::Launching;
	}

	if (currentMode == LaunchingMode::Launching)
	{
		if (timeSinceLastLaunch > timeBetweenLaunchesInWave)
		{
			LaunchOffspring(alien);
			++numberOfOffspringLaunchedThisWave;
			if (numberOfOffspringLaunchedThisWave >= numberOfLaunchesPerWave)
			{
				currentMode = LaunchingMode::Waiting;
				numberOfOffspringLaunchedThisWave = 0;
			}
			timeOfLastLaunch = time.elapsedTime;
		}
	}
	// launch offspring
}

void AIModelAlienMothership::LaunchOffspring(const GameObject& parent)
{
	aliens.push_back(GameObject::CreateGameObject<GameObjectType::AlienOffspring>());
	GameObject& child = aliens.back();

	const auto& parentRB = GetRigidBody(parent.objectId);
	Vector2 childHeading = GetRandomVectorOnCircle();
	Vector2 childPosition = parentRB.position + 16.0f * childHeading;
	Vector2 childFacing = childHeading;

	AddRigidBody(child.objectId, childPosition, childFacing);
	auto& collisionObject = AddCollisionObject(child.objectId, Vector2 { 16.0f, 16.0f });
	collisionObject.layer = CollisionLayer::Alien;
	collisionObject.layerMask = CollisionLayer::Player | CollisionLayer::PlayerBullet;

	child.aiModel = CreateAI(child);
}

AIModelAlienOffspring::AIModelAlienOffspring(GameObject& alien)
	: alien(alien)
{
}

void AIModelAlienOffspring::Update(const Time & /*time*/)
{
	auto& rigidBody = GetRigidBody(alien.objectId);

	const float speed = 40.0f;
	rigidBody.velocity = speed * rigidBody.facing;
}

