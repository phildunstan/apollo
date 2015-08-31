
#include <algorithm>
#include "tweakables.h"

using namespace std;


Tweakables::AutoRegister::~AutoRegister()
{
	Tweakables::GetInstance().Deregister(m_variablePtr);
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


namespace
{
	template <typename TweakableType, typename MapType>
	TweakableType GetValueFromMap(const char* name, const MapType& map)
	{
		const auto iter = map.find(name);
		assert(iter != end(map));
		return *iter->second.variablePtr;
	}

	template <typename TweakableType, typename MapType>
	void SetValueInMap(const char* name, TweakableType value, MapType& map)
	{
		auto iter = map.find(name);
		assert(iter != end(map));
		*iter->second.variablePtr = value;
	}
}


int Tweakables::GetInt(const char* name)
{
	return GetValueFromMap<int>(name, m_ints);
}

void Tweakables::SetInt(const char* name, int value)
{
	SetValueInMap(name, value, m_ints);
}


float Tweakables::GetFloat(const char* name)
{
	return GetValueFromMap<float>(name, m_floats);
}

void Tweakables::SetFloat(const char* name, float value)
{
	SetValueInMap(name, value, m_floats);
}


Vector2 Tweakables::GetVector2(const char* name)
{
	return GetValueFromMap<Vector2>(name, m_vector2s);
}

void Tweakables::SetVector2(const char* name, Vector2 value)
{
	SetValueInMap(name, value, m_vector2s);
}


Vector3 Tweakables::GetVector3(const char* name)
{
	return GetValueFromMap<Vector3>(name, m_vector3s);
}

void Tweakables::SetVector3(const char* name, Vector3 value)
{
	SetValueInMap(name, value, m_vector3s);
}


Color Tweakables::GetColor(const char* name)
{
	return GetValueFromMap<Color>(name, m_colors);
}

void Tweakables::SetColor(const char* name, Color value)
{
	SetValueInMap(name, value, m_colors);
}
