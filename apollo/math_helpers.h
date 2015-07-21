#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

using Vector2 = glm::vec2;
using Vector3 = glm::vec3;

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