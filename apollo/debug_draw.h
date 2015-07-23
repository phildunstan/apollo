#pragma once

#include "math_helpers.h"

// using Color = uint32_t;
enum class Color : uint32_t
{
	White = 0xffffffff,
	Black = 0x000000ff,
	Red = 0xff0000ff,
	Green = 0x00ff00ff,
	Blue = 0x0000ffff,
	Yellow = 0xffff00ff,
	Cyan = 0x00ffffff,
	Purple = 0xff00ffff,
};

void DebugDrawInit();
void DebugDrawShutdown();
void DebugDrawClear();
void DebugDrawLine(const Vector3& begin, const Vector3& end, Color color);
void DebugDrawBox(const glm::mat4& transform, float w, float h, Color color);
void DebugDrawRender(const glm::mat4& projectionMatrix);
