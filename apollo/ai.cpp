#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>

#include "ai.h"
#include "debug_draw.h"
#include "game.h"
#include "game_object.h"
#include "physics.h"
#include "profiler.h"
#include "tweakables.h"
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
	case GameObjectType::AlienWallHugger:
		return std::make_unique<AIModelAlienWallHugger>(gameObject);
	};
	return nullptr;
}




void UpdateRandomVelocity(GameObject& alien, RigidBody& rigidBody, const Time& /*time*/, float lookAheadTime)
{
	auto& collisionObject = GetCollisionObject(alien.objectId);
	bool validMoveTarget = false;
	const float maxAlienSpeed = 40.0f;
	do
	{
		Vector2 newDirection = GetRandomVectorOnCircle();
		rigidBody.velocity = maxAlienSpeed * newDirection;
		Vector2 futurePosition = rigidBody.position + rigidBody.velocity * lookAheadTime;
		validMoveTarget = !BoundingBoxCollidesWithWorldEdge(futurePosition, newDirection, collisionObject.boundingBoxDimensions);
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

	const float maxAlienSpeed = 40.0f;
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
	offspring.reserve(20);
}


void AIModelAlienMothership::Update(const Time& /*time*/)
{
	PROFILER_TIMER_FUNCTION();

	const int numberOfLaunchesPerWave = 20;

	if (currentMode == LaunchingMode::Waiting)
	{
		// remove all dead offspring from the list
		auto isDead = [] (ObjectId objectId) {
			return !GetGameObject(objectId).isAlive;
		};
		offspring.erase(remove_if(begin(offspring), end(offspring), isDead), end(offspring));

		if (offspring.size() < 0.5f * numberOfLaunchesPerWave)
			currentMode = LaunchingMode::Launching;
	}

	if (currentMode == LaunchingMode::Launching)
	{
		offspring.push_back(LaunchOffspring(alien));
		if (offspring.size() >= numberOfLaunchesPerWave)
		{
			currentMode = LaunchingMode::Waiting;
		}
	}
	// launch offspring
}

ObjectId AIModelAlienMothership::LaunchOffspring(const GameObject& parent)
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

	return child.objectId;
}


AIModelAlienOffspring::AIModelAlienOffspring(GameObject& alien)
	: alien(alien)
{
}

TWEAKABLE(float, cohesionMagnitude, "Alien.Offspring.CohesionMagnitude", 1000.0f, 0.0f, 1000.0f);
TWEAKABLE(float, cohesionRadius, "Alien.Offspring.CohesionRadius", 50.0f, 0.0f, 1000.0f);
TWEAKABLE(float, alignmentMagnitude, "Alien.Offspring.AlignmentMagnitude", 10.0f, 0.0f, 1000.0f);
TWEAKABLE(float, alignmentRadius, "Alien.Offspring.AlignmentRadius", 50.0f, 0.0f, 1000.0f);
TWEAKABLE(float, separationMagnitude, "Alien.Offspring.SeparationMagnitude", 500.0f, 0.0f, 1000.0f);
TWEAKABLE(float, separationRadius, "Alien.Offspring.SeparationRadius", 30.0f, 0.0f, 1000.0f);

void AIModelAlienOffspring::Update(const Time& time)
{
	auto& rigidBody = GetRigidBody(alien.objectId);

	// flocking parameters

	Vector2 totalCohesionForce { 0.0f, 0.0f };
	Vector2 totalAlignmentForce { 0.0f, 0.0f };
	Vector2 totalSeparationForce { 0.0f, 0.0f };
	float numberOfOffspringNeighbors { 0.0f };
	float numberOfAllNeighbors { 0.0f };

	auto nearbyAliens = GetAliensInCircle(rigidBody.position, 50.0f);
	for (ObjectId nearbyAlienId : nearbyAliens)
	{
		if (nearbyAlienId == alien.objectId)
			continue;

		++numberOfAllNeighbors;

		const GameObject& nearbyAlien = GetGameObject(nearbyAlienId);
		const auto& nearbyAlienRB = GetRigidBody(nearbyAlienId);
		float nearbyAlienDistance = glm::distance(nearbyAlienRB.position, rigidBody.position);
		if (nearbyAlien.type == alien.type)
		{
			++numberOfOffspringNeighbors;

			// add flocking forces
			if ((nearbyAlienDistance < cohesionRadius) && (nearbyAlienDistance > 0.0f))
			{
				totalCohesionForce = cohesionMagnitude * (nearbyAlienDistance / cohesionRadius) * glm::normalize(nearbyAlienRB.position - rigidBody.position);
			}
			if ((nearbyAlienDistance < alignmentRadius) && (glm::length(nearbyAlienRB.velocity) > 0.0f))
			{
				Vector2 alignmentForce = alignmentMagnitude * /*(1.0f - nearbyAlienDistance / alignmentRadius) * */glm::normalize(nearbyAlienRB.velocity);
				totalAlignmentForce += alignmentForce;
			}
		}

		if ((nearbyAlienDistance < separationRadius) && (nearbyAlienDistance > 0.0f))
		{
			Vector2 separationForce = separationMagnitude * (1.0f - nearbyAlienDistance / separationRadius) * glm::normalize(rigidBody.position - nearbyAlienRB.position);
			totalSeparationForce += separationForce;
		}
	}

	Vector2 forces { 0.0f, 0.0f };
	if (numberOfOffspringNeighbors > 0.0f)
	{
		totalCohesionForce = totalCohesionForce / numberOfOffspringNeighbors;
		//DebugDrawLine(rigidBody.position, rigidBody.position + totalCohesionForce * 0.1f, Color::Green);
		forces += totalCohesionForce;
		totalAlignmentForce = totalAlignmentForce / numberOfOffspringNeighbors;
		//DebugDrawLine(rigidBody.position, rigidBody.position + totalAlignmentForce * 1.0f, Color::Yellow);
		forces += totalAlignmentForce;
	}
	if (numberOfAllNeighbors > 0.0f)
	{
		totalSeparationForce = totalSeparationForce / numberOfAllNeighbors;
		//DebugDrawLine(rigidBody.position, rigidBody.position + totalSeparationForce * 1.0f, Color::Orange);
		forces += totalSeparationForce;
	}
	
	// repulsion for world edges
	const float wallRepulsionRadius = 100.0f;
	const float wallRepulsionMagnitude = 300.0f;
	if (rigidBody.position.x - minWorld.x < wallRepulsionRadius)
	{
		float distanceFactor = sqr(1.0f - (rigidBody.position.x - minWorld.x) / wallRepulsionRadius);
		Vector2 repulsionForce = wallRepulsionMagnitude * distanceFactor * Vector2 { 1.0f, 0.0f };
		//DebugDrawLine(rigidBody.position, rigidBody.position + repulsionForce * 1.0f, Color::White);
		forces += repulsionForce;
	}
	else if (maxWorld.x - rigidBody.position.x < wallRepulsionRadius)
	{
		float distanceFactor = sqr(1.0f - (maxWorld.x - rigidBody.position.x) / wallRepulsionRadius);
		Vector2 repulsionForce = wallRepulsionMagnitude * distanceFactor * Vector2 { -1.0f, 0.0f };
		//DebugDrawLine(rigidBody.position, rigidBody.position + repulsionForce * 1.0f, Color::White);
		forces += repulsionForce;
	}
	if (rigidBody.position.y - minWorld.y < wallRepulsionRadius)
	{
		float distanceFactor = sqr(1.0f - (rigidBody.position.y - minWorld.y) / wallRepulsionRadius);
		Vector2 repulsionForce = wallRepulsionMagnitude * distanceFactor * Vector2 { 0.0f, 1.0f };
		//DebugDrawLine(rigidBody.position, rigidBody.position + repulsionForce * 1.0f, Color::White);
		forces += repulsionForce;
	}
	else if (maxWorld.y - rigidBody.position.y < wallRepulsionRadius)
	{
		float distanceFactor = sqr(1.0f - (maxWorld.y - rigidBody.position.y) / wallRepulsionRadius);
		Vector2 repulsionForce = wallRepulsionMagnitude * distanceFactor * Vector2 { 0.0f, -1.0f };
		//DebugDrawLine(rigidBody.position, rigidBody.position + repulsionForce * 1.0f, Color::White);
		forces += repulsionForce;
	}

	// additional force attracting the flock to the player
	const float playerAttraction = 500.0f;
	const auto& playerRigidBody = GetRigidBody(player.objectId);
	float playerDistance = glm::distance(playerRigidBody.position, rigidBody.position);
	if (playerDistance > 0.0f)
	{
		float worldSize = glm::distance(minWorld, maxWorld);
		float distanceFactor = (playerDistance / worldSize) * (playerDistance / worldSize);
		Vector2 playerAttractionForce = playerAttraction * distanceFactor * glm::normalize(playerRigidBody.position - rigidBody.position);
		//DebugDrawLine(rigidBody.position, rigidBody.position + playerAttractionForce * 1.0f, Color::White);
		forces += playerAttractionForce;
	}
 
	const float maxAlienSpeed = 100.0f;
	const float velocityDampening = 0.0f;
	const float mass = 1.0f;
	rigidBody.velocity = (1.0f - velocityDampening * time.deltaTime) * rigidBody.velocity + (forces / mass) * time.deltaTime;
	float speed = glm::length(rigidBody.velocity);
	if (speed > 0.0f)
	{
		rigidBody.facing = glm::normalize(rigidBody.velocity);
	}
	if (speed > maxAlienSpeed)
	{
		rigidBody.velocity = (maxAlienSpeed / speed) * rigidBody.velocity;
	}
}


AIModelAlienWallHugger::AIModelAlienWallHugger(GameObject& alien)
	: alien(alien)
{
}

TWEAKABLE(float, wallHuggerSpeed, "Alien.WallHugger.Speed", 150.0f, 0.0f, 1000.0f);
TWEAKABLE(float, wallHuggerCrossingProbability, "Alien.WallHugger.CrossingProbability", 0.002f, 0.0f, 0.01f);

void AIModelAlienWallHugger::Update(const Time& /*time*/)
{
	auto& rigidBody = GetRigidBody(alien.objectId);
	Vector2 position = rigidBody.position;
	Vector2 facing = rigidBody.facing;
	const auto& collisionObject = GetCollisionObject(alien.objectId);

	if (currentMovementMode == MovementMode::Stationary)
	{
		currentMovementMode = (GetRandomFloat01() < 0.5f) ? MovementMode::SlideLeft : MovementMode::SlideRight;
	}

	if (currentMovementMode != MovementMode::Crossing)
	{
		float wallCoord = GetPositionAlongWallCoordFromPositionAndFacing(position, facing);
		tie(position, facing) = GetPositionAndFacingFromWallCoord(wallCoord, collisionObject.boundingBoxDimensions);

		if (currentMovementMode == MovementMode::SlideLeft)
			rigidBody.velocity = wallHuggerSpeed * Vector2 { -facing.y, facing.x };
		else // if (currentMovementMode == MovementMode::SlideRight)
			rigidBody.velocity = wallHuggerSpeed * Vector2 { facing.y, -facing.x };


		if (GetRandomFloat01() < wallHuggerCrossingProbability)
		{
			currentMovementMode = MovementMode::Crossing;
			wallStartPosition = rigidBody.position;
			rigidBody.velocity = wallHuggerSpeed * facing;
		}
	}

	if (currentMovementMode == MovementMode::Crossing)
	{
		if (((facing.x < 0) && (position.x < minWorld.x)) ||
			((facing.x > 0) && (position.x > maxWorld.x)) ||
			((facing.y < 0) && (position.y < minWorld.y)) ||
			((facing.y > 0) && (position.y > maxWorld.y)))
		{
			// when we hit the other side, reverse the facing so that GetPositionAlongWallCoordFromPositionAndFacing works
			facing = -facing;
			float wallCoord = GetPositionAlongWallCoordFromPositionAndFacing(position, facing);
			tie(position, facing) = GetPositionAndFacingFromWallCoord(wallCoord, collisionObject.boundingBoxDimensions);

			Vector2 wallFinishPosition = position;
			CreateWall(wallStartPosition, wallFinishPosition);

			currentMovementMode = MovementMode::Stationary;
			rigidBody.velocity = Vector2 { 0.0f, 0.0f };
		}
	}

	rigidBody.position = position;
	rigidBody.facing = facing;
}
