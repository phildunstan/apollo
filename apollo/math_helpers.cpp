#include <random>

#include "math_helpers.h"

using namespace std;

static mt19937_64 randomEngine;
static uniform_real_distribution<float> randomFloat01 { 0.0f, 1.0f };
static uniform_int_distribution<uint64_t> randomUint64;

Matrix4x4 CalculateObjectTransform(const Vector3& position, const Vector3& facing)
{
	auto y = facing;
	auto z = glm::vec3(0.0f, 0.0f, 1.0f);
	auto x = glm::cross(y, z);
	auto transform = Matrix4x4(glm::vec4(x, 0.0f), glm::vec4(y, 0.0f), glm::vec4(y, 0.0f), glm::vec4(position, 1.0f));
	return transform;
}

Matrix4x4 CalculateObjectTransform(const Vector2& position, const Vector2& facing)
{
	auto y = glm::vec3(facing, 0.0f);
	auto z = glm::vec3(0.0f, 0.0f, 1.0f);
	auto x = glm::cross(y, z);
	auto transform = Matrix4x4(glm::vec4(x, 0.0f), glm::vec4(y, 0.0f), glm::vec4(y, 0.0f), glm::vec4(position, 0.0f, 1.0f));
	return transform;
}

void SeedRandom(uint64_t seed)
{
	randomEngine.seed(seed);
}

float GetRandomFloat01()
{
	return randomFloat01(randomEngine);
}

uint64_t GetRandomUint64()
{
	return randomUint64(randomEngine);
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
