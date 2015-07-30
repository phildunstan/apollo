#pragma once

#include <cmath>
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;

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
constexpr T clamp(T x, T minValue, T maxValue)
{
	return std::min(maxValue, std::max(x, minValue));
}


inline bool IsSimilar(float x, float y, float epsilon = 1e-5f)
{
	return std::abs(x - y) <= epsilon;
}

template <typename VectorT>
inline bool IsUnitLength(const VectorT& v, float epsilon = 1e-5f)
{
	return IsSimilar(glm::length(v), 1.0f, epsilon);
}

inline Matrix4x4 CalculateObjectTransform(const Vector3& position, const Vector3& facing)
{
	auto y = facing;
	auto z = glm::vec3(0.0f, 0.0f, 1.0f);
	auto x = glm::cross(y, z);
	auto transform = Matrix4x4(glm::vec4(x, 0.0f), glm::vec4(y, 0.0f), glm::vec4(y, 0.0f), glm::vec4(position, 1.0f));
	return transform;
}


inline Vector3 PerpendicularVector2D(const Vector3& v)
{
	return Vector3{ v.y, -v.x, v.z };
}



