#pragma once

#include "game.h"

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
	Orange = 0xff7f00ff,
	Brown = 0xa52a2a,
};


bool LoadResources();
void RenderWorld(const Time& time, int windowWidth, int windowHeight);
void RenderUI(const Time& time, int windowWidth, int windowHeight);
void RenderDebugUI(const Time& time, int windowWidth, int windowHeight);

enum class ProfilerRenderingMode { FrameTotals, FrameThreads };
void RenderProfiler(const Time& time, int windowWidth, int windowHeight, ProfilerRenderingMode renderingMode);



