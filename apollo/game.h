#pragma once

using ObjectId = uint64_t;

struct Time
{
	float elapsedTime { 0.0f };
	float deltaTime { 0.0f };
};

inline bool operator==(const Time& lhs, const Time& rhs)
{
	return (lhs.elapsedTime == rhs.elapsedTime) && (lhs.deltaTime == rhs.deltaTime);
}
