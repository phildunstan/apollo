
#include <algorithm>
#include "tweakables.h"

using namespace std;

Tweakables::AutoRegister::~AutoRegister()
{
	Tweakables::GetInstance().Deregister(m_variablePtr);
}


bool Tweakables::Register(const char* name, float& variable, float minValue, float maxValue)
{
	if (m_floats.find(name) != m_floats.end())
	{
		char msg[128];
		snprintf(msg, 128, "Tweakable variable already exists with name %s", name);
		fprintf(stderr, msg);
		return false;
	}

	m_floats[name] = { name, &variable, minValue, maxValue };
	return true;
}

float Tweakables::GetValue(const char* name)
{
	const auto iter = m_floats.find(name);
	if (iter == end(m_floats))
	{
		char msg[128];
		snprintf(msg, 128, "Tweakable %s was not found in the float variables", name);
		fprintf(stderr, msg);
	}
	return *iter->second.variablePtr;
}

void Tweakables::SetValue(const char* name, float value)
{
	auto iter = m_floats.find(name);
	if (iter == end(m_floats))
	{
		char msg[128];
		snprintf(msg, 128, "Tweakable %s was not found in the float variables", name);
		fprintf(stderr, msg);
	}
	*iter->second.variablePtr = value;
}


void Tweakables::Deregister(void* variablePtr)
{
	auto ptrMatches = [variablePtr] (const auto& pair) {
		return pair.second.variablePtr == variablePtr;
	};
	auto floatIter = find_if(begin(m_floats), end(m_floats), ptrMatches);
	if (floatIter != end(m_floats))
	{
		m_floats.erase(floatIter);
	}
}
