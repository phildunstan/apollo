#include <cassert>
#include <unordered_map>

#include "profiler.h"

using namespace std;

vector<ProfilerDataPoint> profileData;

void ProfilerInit()
{
	profileData.reserve(10000);
}

void ProfilerReset()
{
	profileData.clear();
}

struct DataPointKey
{
	const char* filename;
	int line;
};

bool operator==(const DataPointKey& lhs, const DataPointKey& rhs)
{
	return (lhs.filename == rhs.filename) && (lhs.line == rhs.line);
}

template <>
struct hash<DataPointKey>
{
	size_t operator()(const DataPointKey& key) const
	{
		return (3 * hash<const char*>()(key.filename)) ^ hash<int>()(key.line);
	}
};


void ProfilerAppendCurrentFrameStatistics(std::vector<ProfilerFrameStatistics>& accumulatedStatistics)
{
	accumulatedStatistics.emplace_back();
	ProfilerFrameStatistics& currentFrameStatistics = accumulatedStatistics.back();
	currentFrameStatistics.clear();

	static unordered_map<DataPointKey, size_t> index;
	index.clear();

	// reserve the 0th statistics for the total frame time
	currentFrameStatistics.emplace_back("total frame time");

	for (const auto& dataPoint : profileData)
	{
		auto key = DataPointKey { dataPoint.filename, dataPoint.line };
		auto indexEntryIter = index.find(key);
		if (indexEntryIter == index.end())
		{
			index[key] = currentFrameStatistics.size();
			currentFrameStatistics.push_back(dataPoint);

		}
		else
		{
			auto& blockStastistics = currentFrameStatistics[indexEntryIter->second];
			assert(blockStastistics.filename == dataPoint.filename);
			assert(blockStastistics.line == dataPoint.line);
			assert(blockStastistics.id == dataPoint.id);
			blockStastistics.duration += dataPoint.duration;
			blockStastistics.hitCount += dataPoint.hitCount;
		}

		if (strcmp(dataPoint.id, "main_loop") == 0)
		{
			currentFrameStatistics[0].duration = currentFrameStatistics[index[key]].duration;
			currentFrameStatistics[0].hitCount = 1;
		}
	}
}


const std::vector<ProfilerFrameStatistics>& ProfilerGetAccumulatedStatistics()
{
	static vector<ProfilerFrameStatistics> statistics;
	ProfilerAppendCurrentFrameStatistics(statistics);
	return statistics;
}