#include "ai.h"
#include "game.h"
#include "game_object.h"
#include "physics.h"
#include "world.h"


void AIModelAlienRandom::Update(const Time & time, GameObject & alien)
{
	auto& rigidBody = GetRigidBody(alien.objectId);

	const float maxTimeBetweenMovementChanges = 1.0f;
	if (time.elapsedTime - timeOfLastMovementChange > maxTimeBetweenMovementChanges)
	{
		auto& collisionObject = GetCollisionObject(alien.objectId);
		bool validMoveTarget = false;
		do
		{
			Vector2 newDirection = GetRandomVectorOnCircle();
			rigidBody.velocity = alienSpeed * newDirection;
			Vector2 futurePosition = rigidBody.position + rigidBody.velocity * maxTimeBetweenMovementChanges;
			validMoveTarget = !BoundingBoxCollidesWithWorldEdge(futurePosition, newDirection, collisionObject.aabbDimensions);
		} while (!validMoveTarget);
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

void AIModelAlienChase::Update(const Time & time, GameObject & alien)
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

void AIModelAlienShy::Update(const Time & time, GameObject & alien)
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
