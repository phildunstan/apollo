#pragma once

#include <cmath>
#include <random>
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/epsilon.hpp"

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

using Matrix4x4 = glm::mat4;

constexpr float pi_internal()
{
	return 3.14159265358979323846264338327950288f;
}

const float PI = pi_internal();
const float TWO_PI = 2.0f * pi_internal();
const float PI_OVER_TWO = pi_internal() / 2.0f;
const float PI_OVER_FOUR = pi_internal() / 4.0f;

template <typename T>
constexpr T sqr(T x)
{
	return x * x;
}


template <typename T>
constexpr T clamp(T x, T minValue, T maxValue)
{
	return std::min(maxValue, std::max(x, minValue));
}


inline bool IsSimilar(float a, float b, float epsilon = 1e-4f)
{
	float sizeA = std::abs(a);
	float sizeB = std::abs(b);
	if (sizeA > sizeB)
		return glm::epsilonEqual(a, b, sizeA * epsilon);
	else
		return glm::epsilonEqual(a, b, sizeB * epsilon);
}

template <typename VectorT>
inline bool IsSimilar(const VectorT& a, const VectorT& b, float epsilon = 1e-4f)
{
	float sizeA = glm::length(a);
	float sizeB = glm::length(b);
	if (sizeA > sizeB)
		return glm::all(glm::epsilonEqual(a, b, sizeA * epsilon));
	else
		return glm::all(glm::epsilonEqual(a, b, sizeB * epsilon));
}

template <typename VectorT>
inline bool IsUnitLength(const VectorT& v, float epsilon = 1e-4f)
{
	return IsSimilar(glm::length(v), 1.0f, epsilon);
}

inline Vector2 PerpendicularRightVector2D(const Vector2& v)
{
	return Vector2 { v.y, -v.x};
}

Matrix4x4 CalculateObjectTransform(const Vector3& position, const Vector3& facing);
Matrix4x4 CalculateObjectTransform(const Vector2& position, const Vector2& facing);


void SeedRandom(uint64_t seed);
float GetRandomFloat01();
uint64_t GetRandomUint64();
Vector2 GetRandomVectorOnCircle();
Vector2 GetRandomVectorInBox(const Vector2& min, const Vector2& max);

