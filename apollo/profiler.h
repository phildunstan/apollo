#pragma once

#include <chrono>
#include <vector>

#define PROFILER_TIMER() ProfilerTimer timer##__COUNTER__(__FUNCTION__, __FILE__, __LINE__)

using ProfilerTimeUnit = std::chrono::time_point<std::chrono::high_resolution_clock>;
using ProfilerDurationUnit = std::chrono::nanoseconds;


struct ProfilerDataPoint
{
	const char* id;
	const char* filename;
	ProfilerDurationUnit duration;
	int line;
	int hitCount;
};

extern std::vector<ProfilerDataPoint> profileData;


void ProfilerInit();

inline void ProfilerAdd(const char* id, const char* filename, int line, ProfilerDurationUnit duration, int hitCount)
{
	profileData.push_back({ id, filename, line, duration, hitCount });
}

void ProfilerReset();


const std::vector<ProfilerDataPoint>& ProfilerGetAccumulatedStatistics();


struct ProfilerTimer
{
	ProfilerTimer(const char* id_, const char* filename_, int line_)
		: id(id_)
		, filename(filename_)
		, line(line_)
		, startTime(std::chrono::high_resolution_clock::now())
	{
	}

	~ProfilerTimer()
	{
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now - startTime;
		ProfilerAdd(id, filename, line, duration, 1);
	}

	const char* id;
	const char* filename;
	int line;
	ProfilerTimeUnit startTime;
};
