#pragma once

#include <memory>
#include <vector>
#include "game.h"
#include "math_helpers.h"

struct Time;
struct GameObject;

class AIModel
{
public:
	AIModel() = default;
	virtual ~AIModel() = default;
	virtual std::unique_ptr<AIModel> Clone() const = 0;
	virtual void Update(const Time& time) = 0;

	AIModel& operator=(const AIModel&) = delete;

protected:
	AIModel(const AIModel&) = default;
};

std::unique_ptr<AIModel> CreateAI(GameObject& gameObject);


class AIModelAlienRandom : public AIModel
{
public:
	explicit AIModelAlienRandom(GameObject& alien);
	explicit AIModelAlienRandom(const AIModelAlienRandom& other) = default;
	std::unique_ptr<AIModel> Clone() const override;
	void Update(const Time& time) override;

private:
	GameObject& alien;
	float timeOfLastMovementChange { 0.0f };
	float timeOfLastShot { 0.0f };
};


class AIModelAlienShy : public AIModel
{
public:
	explicit AIModelAlienShy(GameObject& alien);
	explicit AIModelAlienShy(const AIModelAlienShy& other) = default;
	std::unique_ptr<AIModel> Clone() const override;
	void Update(const Time& time) override;

private:
	GameObject& alien;
	float timeOfLastMovementChange { 0.0f };
};


class AIModelAlienChase : public AIModel
{
public:
	explicit AIModelAlienChase(GameObject& alien);
	explicit AIModelAlienChase(const AIModelAlienChase& other) = default;
	std::unique_ptr<AIModel> Clone() const override;
	void Update(const Time& time) override;

private:
	GameObject& alien;
};


class AIModelAlienMothership : public AIModel
{
public:
	explicit AIModelAlienMothership(GameObject& alien);
	explicit AIModelAlienMothership(const AIModelAlienMothership& other) = default;
	std::unique_ptr<AIModel> Clone() const override;
	void Update(const Time& time) override;

private:
	static ObjectId LaunchOffspring(const GameObject& parent);

	GameObject& alien;
	enum class LaunchingMode { Waiting, Launching };
	LaunchingMode currentMode { LaunchingMode::Waiting };
	std::vector<ObjectId> offspring;
};


class AIModelAlienOffspring : public AIModel
{
public:
	explicit AIModelAlienOffspring(GameObject& alien);
	explicit AIModelAlienOffspring(const AIModelAlienOffspring& other) = default;
	std::unique_ptr<AIModel> Clone() const override;
	void Update(const Time& time) override;

private:
	GameObject& alien;
};


class AIModelAlienWallHugger : public AIModel
{
public:
	explicit AIModelAlienWallHugger(GameObject& alien);
	explicit AIModelAlienWallHugger(const AIModelAlienWallHugger& other) = default;
	std::unique_ptr<AIModel> Clone() const override;
	void Update(const Time& time) override;

private:

	enum class MovementMode { Stationary, SlideLeft, SlideRight, Crossing };
	MovementMode currentMovementMode { MovementMode::Stationary };

	GameObject& alien;

	Vector2 wallStartPosition { 0.0f, 0.0f };
};





