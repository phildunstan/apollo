#pragma once

#include "math_helpers.h"
#include "rendering.h"

void DebugDrawInit();
void DebugDrawShutdown();
void DebugDrawClear();
void DebugDrawLine(const Vector2& begin, const Vector2& end, Color color);
void DebugDrawBox(const glm::mat4& transform, float w, float h, Color color);
void DebugDrawBox2d(const Vector2& min, const Vector2& max, Color color);
void DebugDrawRender(const glm::mat4& projectionMatrix);
