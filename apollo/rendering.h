#pragma once

#include "game.h"

bool LoadResources();
void RenderWorld(const Time& time, int windowWidth, int windowHeight);
void RenderUI(const Time& time, int windowWidth, int windowHeight);
void RenderDebugUI(const Time& time, int windowWidth, int windowHeight);

