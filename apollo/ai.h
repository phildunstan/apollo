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
	AIModel(GameObject& gameObject);
	virtual ~AIModel() = default;
	virtual std::unique_ptr<AIModel> Clone() const = 0;
	virtual void Update(const Time& time) = 0;

	AIModel& operator=(const AIModel&) = delete;

	ObjectId objectId;

protected:
	AIModel(const AIModel&) = default;
};


extern std::vector<std::unique_ptr<AIModel>> aiModels;


void CreateAI(GameObject& gameObject);

AIModel& GetAIModel(ObjectId objectId);
