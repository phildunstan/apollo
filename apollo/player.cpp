#include "player.h"
#include "game_object.h"

int playerScore { 0 };


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
