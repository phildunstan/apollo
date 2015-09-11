#pragma once

#include <chrono>
#include <vector>

#include "ETWProviders/etwprof.h"

#define PROFILER_BEGIN_FRAME() ProfilerBeginFrame();

#define PROFILER_TIMER_FUNCTION() ProfilerTimer timer##__COUNTER__(__FUNCTION__, __FILE__, __LINE__)

#define PROFILER_TIMER_BEGIN(ID) ProfilerTimer timer##ID(#ID, __FILE__, __LINE__)
#define PROFILER_TIMER_END(ID) timer##ID.end()

using ProfilerTimeUnit = std::chrono::time_point<std::chrono::high_resolution_clock>;
using ProfilerDurationUnit = std::chrono::nanoseconds;


struct ProfilerDataPoint
{
	ProfilerDataPoint(const char* id_)
		: id(id_)
		, filename("")
		, duration()
		, line(0)
		, hitCount(0)
	{
	}

	ProfilerDataPoint(const char* id_, const char* filename_, int line_, ProfilerDurationUnit duration_, int hitCount_)
		: id(id_)
		, filename(filename_)
		, duration(duration_)
		, line(line_)
		, hitCount(hitCount_)
	{
	}

	const char* id;
	const char* filename;
	ProfilerDurationUnit duration;
	int line;
	int hitCount;
};

extern std::vector<ProfilerDataPoint> profileData;


void ProfilerInit();
void ProfilerShutdown();

inline void ProfilerAdd(const char* id, const char* filename, int line, ProfilerDurationUnit duration, int hitCount)
{
	profileData.emplace_back(id, filename, line, duration, hitCount);
}

void ProfilerBeginFrame();


using ProfilerFrameStatistics = std::vector<ProfilerDataPoint>;
const std::vector<ProfilerFrameStatistics>& ProfilerGetAccumulatedStatistics();


struct ProfilerTimer
{
	ProfilerTimer(const char* id_, const char* filename_, int line_)
		: id { id_ }
		, filename { filename_ }
		, line { line_ }
		, startTime { std::chrono::high_resolution_clock::now() }
		, etwTimestamp { ETWBegin(id) }
	{
	}

	~ProfilerTimer()
	{
		end();
	}

	void end()
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now - startTime;
		ProfilerAdd(id, filename, line, duration, 1);
		ETWEnd(id, etwTimestamp);
	}

	const char* id;
	const char* filename;
	int line;
	ProfilerTimeUnit startTime;
	int64 etwTimestamp;
};
