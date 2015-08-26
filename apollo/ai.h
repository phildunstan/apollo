#pragma once

#include <memory>

struct Time;
struct GameObject;

class AIModel
{
public:
	AIModel() = default;
	virtual ~AIModel() = default;
	virtual void Update(const Time& time) = 0;

	AIModel(const AIModel&) = delete;
	AIModel& operator=(const AIModel&) = delete;
};

std::unique_ptr<AIModel> CreateAI(GameObject& gameObject);


class AIModelAlienRandom : public AIModel
{
public:
	AIModelAlienRandom(GameObject& alien);
	void Update(const Time& time) override;

private:
	GameObject& alien;
	float timeOfLastMovementChange { 0.0f };
	float timeOfLastShot { 0.0f };
};


class AIModelAlienShy : public AIModel
{
public:
	AIModelAlienShy(GameObject& alien);
	void Update(const Time& time) override;

private:
	GameObject& alien;
	float timeOfLastMovementChange { 0.0f };
};


class AIModelAlienChase : public AIModel
{
public:
	AIModelAlienChase(GameObject& alien);
	void Update(const Time& time) override;

private:
	GameObject& alien;
};


class AIModelAlienMothership : public AIModel
{
public:
	AIModelAlienMothership(GameObject& alien);
	void Update(const Time& time) override;

private:
	static void LaunchOffspring(const GameObject& parent);

	GameObject& alien;
	enum class LaunchingMode { Waiting, Launching };
	LaunchingMode currentMode { LaunchingMode::Waiting };
	float timeOfLastLaunch { 0.0f };
	float numberOfOffspringLaunchedThisWave = 0;
};


class AIModelAlienOffspring : public AIModel
{
public:
	AIModelAlienOffspring(GameObject& alien);
	void Update(const Time& time) override;

private:
	GameObject& alien;
};






