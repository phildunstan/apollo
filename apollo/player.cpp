#include "player.h"
#include "game_object.h"

int playerScore { 0 };


void IncrementPlayerScore(const GameObject& gameObject)
{
	if ((gameObject.type == GameObjectType::AlienChase) ||
		(gameObject.type == GameObjectType::AlienRandom) ||
		(gameObject.type == GameObjectType::AlienShy))
		++playerScore;
}
