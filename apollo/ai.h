#pragma once

struct Time;
struct GameObject;

const float maxAlienSpeed = 40.0f;

class AIModel
{
public:
	AIModel() = default;
	virtual ~AIModel() = default;
	virtual void Update(const Time& time, GameObject& gameObject) = 0;

	AIModel(const AIModel&) = delete;
	AIModel& operator=(const AIModel&) = delete;
};


class AIModelAlienRandom : public AIModel
{
public:
	~AIModelAlienRandom() {}
	void Update(const Time& time, GameObject& alien) override;

	float timeOfLastMovementChange { 0.0f };
	float timeOfLastShot { 0.0f };
};


class AIModelAlienShy : public AIModel
{
public:
	void Update(const Time& time, GameObject& alien) override;

	float timeOfLastMovementChange { 0.0f };
};


class AIModelAlienChase : public AIModel
{
public:
	void Update(const Time& time, GameObject& alien) override;
};






