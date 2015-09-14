#include <algorithm>
#include "SDL.h"

#include "player.h"
#include "game_object.h"
#include "physics.h"
#include "tweakables.h"
#include "world.h"


TWEAKABLE(float, playerMovementSpeed, "Player.MovementSpeed", 200.0f, 0.0f, 1000.0f);
TWEAKABLE(float, playerRotationSpeed, "Player.RotationSpeed", 0.2f, 0.0f, 1.0f);
TWEAKABLE(float, playerFireRate, "Player.FireRate", 4.0f, 0.0f, 10.0f); // shots per second


int playerScore { 0 };
float playerTimeOfLastShot { 0.0f };

void ApplyPlayerInput(const Time& time, const PlayerInput& playerInput)
{
	auto& playerRB = GetRigidBody(player.objectId);
	playerRB.velocity = playerInput.movement * playerMovementSpeed;

	if (glm::length(playerInput.facing) > 0.5f)
	{
		auto playerHeading = atan2f(-playerRB.facing.x, playerRB.facing.y);
		auto normalizedInput = glm::normalize(playerInput.facing);
		auto desiredHeading = atan2f(-normalizedInput.x, normalizedInput.y);
		auto delta = desiredHeading - playerHeading;
		if (delta < -PI)
		{
			delta += TWO_PI;
		}
		else if (delta > PI)
		{
			delta -= TWO_PI;
		}
		delta = clamp(delta, -playerRotationSpeed, playerRotationSpeed);
		auto newHeading = playerHeading + delta;

		playerRB.facing = Vector2(-sin(newHeading), cos(newHeading));
	}

	//// stop the player movement if they are colliding with the edge of the screen
	//const auto& collisionObject = GetCollisionObject(player.objectId);
	//Vector2 futurePosition = playerRB.position + playerRB.velocity * time.deltaTime;
	//if (BoundingBoxCollidesWithWorldEdge(futurePosition, playerRB.facing, collisionObject.boundingBoxDimensions))
	//{
	//	playerRB.velocity = Vector2 { 0.0f, 0.0f };
	//}

	if (playerInput.firing && ((time.elapsedTime - playerTimeOfLastShot) > 1.0f / playerFireRate))
	{
		FirePlayerBullet();
		playerTimeOfLastShot = time.elapsedTime;
	}
}



void IncrementPlayerScore(const GameObject& gameObject)
{
	switch (gameObject.type)
	{
	case GameObjectType::Player:
		break;
	case GameObjectType::Bullet:
		break;
	case GameObjectType::AlienRandom:
		playerScore += 10;
		break;
	case GameObjectType::AlienChase:
		playerScore += 10;
		break;
	case GameObjectType::AlienShy:
		playerScore += 10;
		break;
	case GameObjectType::AlienMothership:
		playerScore += 25;
		break;
	case GameObjectType::AlienOffspring:
		playerScore += 1;
		break;
	case GameObjectType::AlienWallHugger:
		playerScore += 10;
		break;
	}
}
