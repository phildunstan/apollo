#include <random>

#include "math_helpers.h"

using namespace std;

static mt19937_64 randomEngine;
static uniform_real_distribution<float> randomFloat01(0.0f, 1.0f);

void SeedRandom(uint64_t seed)
{
	randomEngine.seed(seed);
}

float GetRandomFloat01()
{
	return randomFloat01(randomEngine);
}

Vector2 GetRandomVectorOnCircle()
{
	float angle = randomFloat01(randomEngine) * TWO_PI;
	return Vector2 { cos(angle), sin(angle) };
}

Vector2 GetRandomVectorInBox(const Vector2& min, const Vector2& max)
{
	float x = randomFloat01(randomEngine) * (max.x - min.x) + min.x;
	float y = randomFloat01(randomEngine) * (max.y - min.y) + min.y;
	return Vector2(x, y);
}
