#pragma once

#include <vector>

#include "game.h"
#include "math_helpers.h"

struct Time;
struct GameObject;



void CreateAI(ObjectId objectId);

void UpdateAI(const Time& time);




struct AIModelAlienRandom
{
	explicit AIModelAlienRandom(ObjectId objectId);
	void Update(const Time& time);

	ObjectId objectId;
	float timeOfLastMovementChange { 0.0f };
	float timeOfLastShot { 0.0f };
};


struct AIModelAlienShy
{
	explicit AIModelAlienShy(ObjectId objectId);
	void Update(const Time& time);

	ObjectId objectId;
	float timeOfLastMovementChange { 0.0f };
};


struct AIModelAlienChase
{
	explicit AIModelAlienChase(ObjectId objectId);
	void Update(const Time& time);

	ObjectId objectId;
};


struct AIModelAlienMothership
{
	explicit AIModelAlienMothership(ObjectId objectId);
	void Update(const Time& time);
	ObjectId LaunchOffspring();

	ObjectId objectId;
	enum class LaunchingMode { Waiting, Launching };
	LaunchingMode currentMode { LaunchingMode::Waiting };
	std::vector<ObjectId> offspring;
};


struct AIModelAlienOffspring
{
	explicit AIModelAlienOffspring(ObjectId objectId);
	void Update(const Time& time);

	ObjectId objectId;
};


struct AIModelAlienWallHugger
{
	explicit AIModelAlienWallHugger(ObjectId objectId);
	void Update(const Time& time);

	ObjectId objectId;
	enum class MovementMode { Stationary, SlideLeft, SlideRight, Crossing };
	MovementMode currentMovementMode { MovementMode::Stationary };
	Vector2 wallStartPosition { 0.0f, 0.0f };
};


extern std::vector<AIModelAlienRandom> randomAIs;
extern std::vector<AIModelAlienShy> shyAIs;
extern std::vector<AIModelAlienChase> chaseAIs;
extern std::vector<AIModelAlienMothership> mothershipAIs;
extern std::vector<AIModelAlienOffspring> offspringAIs;
extern std::vector<AIModelAlienWallHugger> wallHuggerAIs;
