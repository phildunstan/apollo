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

#define PROFILER_TIMER_FUNCTION() ProfilerBlock timer##__COUNTER__(__FUNCTION__, __FILE__, __LINE__); \
	TraceLoggingFunction(ProfilerTraceLoggingProvider)

#define PROFILER_TIMER_BEGIN(ID) ProfilerBlock timer##ID(#ID, __FILE__, __LINE__); \
	TraceLoggingActivity<ProfilerTraceLoggingProvider> traceLoggingActivity; \
	TraceLoggingWriteStart(traceLoggingActivity, #ID)

#define PROFILER_TIMER_END(ID) timer##ID.end(); \
	TraceLoggingWriteStop(traceLoggingActivity, #ID)


using ProfilerTimeUnit = std::chrono::time_point<std::chrono::high_resolution_clock>;
using ProfilerDurationUnit = std::chrono::nanoseconds;


void ProfilerInit();
void ProfilerBeginFrame();
void ProfilerShutdown();


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


struct ProfilerBlockStatistics
{
	ProfilerBlockStatistics(const char* id_, const char* filename_, int line_, ProfilerDurationUnit duration_, int hitCount_)
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
static_assert(sizeof(ProfilerBlockStatistics) == 32, "sizeof(ProfilerDataPoint) == 32");

using ProfilerFrameStatistics = std::vector<ProfilerBlockStatistics>;
const std::vector<ProfilerFrameStatistics>& ProfilerGetAccumulatedStatistics();


struct ProfilerBlock
{
	ProfilerBlock(const char* id_, const char* filename_, int line_)
		: id { id_ }
		, filename { filename_ }
		, line { line_ }
	{
		ProfilerAddBeginEvent(id, filename, line);
	}

	~ProfilerBlock()
	{
		end();
	}

	void end()
	{
		ProfilerAddEndEvent(id, filename, line);
	}

	const char* const id;
	const char* const filename;
	const int line;
};
