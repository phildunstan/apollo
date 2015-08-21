#include <vector>

#include "ai.h"
#include "game.h"
#include "game_object.h"
#include "physics.h"
#include "world.h"

using namespace std;

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


void AIModelAlienRandom::Update(const Time & time, GameObject & alien)
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

void AIModelAlienShy::Update(const Time & time, GameObject & alien)
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


void AIModelAlienChase::Update(const Time & time, GameObject& alien)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	UpdateChaseVelocity(alien, rigidBody, time);
}

