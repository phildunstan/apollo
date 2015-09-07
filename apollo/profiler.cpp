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


const vector<ProfilerDataPoint>& ProfilerGetAccumulatedStatistics()
{
	static vector<ProfilerDataPoint> statistics;
	static unordered_map<DataPointKey, size_t> index;
	statistics.clear();
	index.clear();

	for (const auto& dataPoint : profileData)
	{
		auto key = DataPointKey { dataPoint.filename, dataPoint.line };
		auto indexEntryIter = index.find(key);
		if (indexEntryIter == index.end())
		{
			index[key] = statistics.size();
			statistics.push_back(dataPoint);
		}
		else
		{
			auto& accumulatedStatistics = statistics[indexEntryIter->second];
			assert(accumulatedStatistics.filename == dataPoint.filename);
			assert(accumulatedStatistics.line == dataPoint.line);
			assert(accumulatedStatistics.id == dataPoint.id);
			accumulatedStatistics.duration += dataPoint.duration;
			accumulatedStatistics.hitCount += dataPoint.hitCount;
		}
	}

	return statistics;
}
