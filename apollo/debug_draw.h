#pragma once

#include "math_helpers.h"

// using Color = uint32_t;
enum class Color : uint32_t
{
	White = 0xffffffff,
	Black = 0x000000ff,
	LightGray = 0xbfbfbfff,
	Gray = 0x7f7f7fff,
	DarkGray = 0x3f3f3fff,
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
void DebugDrawLine(const Vector2& begin, const Vector2& end, Color color);
void DebugDrawBox(const glm::mat4& transform, float w, float h, Color color);
void DebugDrawRender(const glm::mat4& projectionMatrix);
