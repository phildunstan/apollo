#pragma once

#include <chrono>
#include <vector>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // Defines macros used by TraceLoggingProvider.h
#undef min
#undef max
#include <TraceLoggingProvider.h>  // The native TraceLogging API
#include <TraceLoggingActivity.h>

// Forward-declare the ProfilerTraceLoggingProvider variable that you will use for tracing in this component
TRACELOGGING_DECLARE_PROVIDER(ProfilerTraceLoggingProvider);

#define PROFILER_BEGIN_FRAME() ProfilerBeginFrame()

#define PROFILER_TIMER_FUNCTION() ProfilerTimer timer##__COUNTER__(__FUNCTION__, __FILE__, __LINE__); \
	TraceLoggingFunction(ProfilerTraceLoggingProvider)

#define PROFILER_TIMER_BEGIN(ID) ProfilerTimer timer##ID(#ID, __FILE__, __LINE__); \
	TraceLoggingActivity<ProfilerTraceLoggingProvider> traceLoggingActivity; \
	TraceLoggingWriteStart(traceLoggingActivity, #ID)

#define PROFILER_TIMER_END(ID) timer##ID.end(); \
	TraceLoggingWriteStop(traceLoggingActivity, #ID)


using ProfilerTimeUnit = std::chrono::time_point<std::chrono::high_resolution_clock>;
using ProfilerDurationUnit = std::chrono::nanoseconds;

struct ProfileEvent
{
	enum class Type { Begin, End};

	ProfileEvent(Type type_, const char* id_, const char* filename_, int line_)
		: time(std::chrono::high_resolution_clock::now())
		, id(id_)
		, filename(filename_)
		, type(type_)
		, line(line_)
		, threadId(std::this_thread::get_id())
	{
	}

	ProfilerTimeUnit time;
	const char* id;
	const char* filename;
	Type type : 1;
	int line : 31;
	std::thread::id threadId;
};
static_assert(sizeof(ProfileEvent) == 32, "sizeof(ProfileEvent) == 32");

extern std::vector<ProfileEvent> profileEvents;

inline void ProfilerAddBeginEvent(const char* id, const char* filename, int line)
{
	profileEvents.emplace_back(ProfileEvent::Type::Begin, id, filename, line);
}

inline void ProfilerAddEndEvent(const char* id, const char* filename, int line)
{
	profileEvents.emplace_back(ProfileEvent::Type::End, id, filename, line);
}


struct ProfilerDataPoint
{
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
static_assert(sizeof(ProfilerDataPoint) == 32, "sizeof(ProfilerDataPoint) == 32");

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
	{
		ProfilerAddBeginEvent(id, filename, line);
	}

	~ProfilerTimer()
	{
		end();
	}

	void end()
	{
		ProfilerAddEndEvent(id, filename, line);
		auto now = std::chrono::high_resolution_clock::now();
		auto duration = now - startTime;
		ProfilerAdd(id, filename, line, duration, 1);
	}

	const char* id;
	const char* filename;
	int line;
	ProfilerTimeUnit startTime;
};
