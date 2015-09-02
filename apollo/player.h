#pragma once

#include "math_helpers.h"

struct GameObject;
struct Time;

extern float playerMovementSpeed;
extern float playerRotationSpeed;
extern float playerFireRate;


struct PlayerInput
{
	Vector2 movement { 0.0f, 0.0f };
	Vector2 facing { 0.0f, 0.0f };
	bool firing { false };
};

void ApplyPlayerInput(const Time& time, const PlayerInput& playerInput);


extern int playerScore;

void IncrementPlayerScore(const GameObject& gameObject);
