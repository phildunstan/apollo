
#include <algorithm>
#include "tweakables.h"

using namespace std;

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


bool Tweakables::GetBool(const char* name) const
{
	const auto* tweakable = GetTweakable(name);
	assert(tweakable);
	return *tweakable->value.b;
}

void Tweakables::SetBool(const char* name, bool value)
{
	auto* tweakable = GetTweakable(name);
	assert(tweakable);
	*tweakable->value.b = value;
}


int Tweakables::GetInt(const char* name) const
{
	const auto* tweakable = GetTweakable(name);
	assert(tweakable);
	return *tweakable->value.i;
}

void Tweakables::SetInt(const char* name, int value)
{
	auto* tweakable = GetTweakable(name);
	assert(tweakable);
	*tweakable->value.i = value;
}


float Tweakables::GetFloat(const char* name) const
{
	const auto* tweakable = GetTweakable(name);
	assert(tweakable);
	return *tweakable->value.f;
}

void Tweakables::SetFloat(const char* name, float value)
{
	auto* tweakable = GetTweakable(name);
	assert(tweakable);
	*tweakable->value.f = value;
}


Vector2 Tweakables::GetVector2(const char* name) const
{
	const auto* tweakable = GetTweakable(name);
	assert(tweakable);
	return *tweakable->value.vec2;
}

void Tweakables::SetVector2(const char* name, Vector2 value)
{
	auto* tweakable = GetTweakable(name);
	assert(tweakable);
	*tweakable->value.vec2 = value;
}


Vector3 Tweakables::GetVector3(const char* name) const
{
	const auto* tweakable = GetTweakable(name);
	assert(tweakable);
	return *tweakable->value.vec3;
}

void Tweakables::SetVector3(const char* name, Vector3 value)
{
	auto* tweakable = GetTweakable(name);
	assert(tweakable);
	*tweakable->value.vec3 = value;
}


Color Tweakables::GetColor(const char* name) const
{
	const auto* tweakable = GetTweakable(name);
	assert(tweakable);
	return *tweakable->value.color;
}

void Tweakables::SetColor(const char* name, Color value)
{
	auto* tweakable = GetTweakable(name);
	assert(tweakable);
	*tweakable->value.color = value;
}



const Tweakables::Tweakable* Tweakables::GetTweakable(const char* name) const
{
	auto nameCompare = [] (const Tweakable& tweakable, const char* name) {
		return strcmp(tweakable.name, name) < 0;
	};
	auto iter = lower_bound(cbegin(m_tweakables), cend(m_tweakables), name, nameCompare);
	if ((iter != cend(m_tweakables)) && (strcmp(iter->name, name) == 0))
		return &*iter;
	else
		return nullptr;
}

Tweakables::Tweakable* Tweakables::GetTweakable(const char* name)
{
	auto nameCompare = [] (const Tweakable& tweakable, const char* name) {
		return strcmp(tweakable.name, name) < 0;
	};
	auto iter = lower_bound(begin(m_tweakables), end(m_tweakables), name, nameCompare);
	if ((iter != end(m_tweakables)) && (strcmp(iter->name, name) == 0))
		return &*iter;
	else
		return nullptr;
}



Tweakables::Tweakable::Tweakable(const char* name, bool* valuePtr, bool minValue, bool maxValue)
	: name(name)
	, type(Type::Bool)
{
	value.b = valuePtr;
	min.b = minValue;
	max.b = maxValue;
}

Tweakables::Tweakable::Tweakable(const char* name, int* valuePtr, int minValue, int maxValue)
	: name(name)
	, type(Type::Int)
{
	value.i = valuePtr;
	min.i = minValue;
	max.i = maxValue;
}

Tweakables::Tweakable::Tweakable(const char* name, float* valuePtr, float minValue, float maxValue)
: name(name)
, type(Type::Float)
{
	value.f = valuePtr;
	min.f = minValue;
	max.f = maxValue;
}

Tweakables::Tweakable::Tweakable(const char* name, Vector2* valuePtr, Vector2 minValue, Vector2 maxValue)
: name(name)
, type(Type::Vector2)
{
	value.vec2 = valuePtr;
	min.vec2 = minValue;
	max.vec2 = maxValue;
}

Tweakables::Tweakable::Tweakable(const char* name, Vector3* valuePtr, Vector3 minValue, Vector3 maxValue)
: name(name)
, type(Type::Vector3)
{
	value.vec3 = valuePtr;
	min.vec3 = minValue;
	max.vec3 = maxValue;
}

Tweakables::Tweakable::Tweakable(const char* name, Color* valuePtr, Color minValue, Color maxValue)
: name(name)
, type(Type::Color)
{
	value.color = valuePtr;
	min.color = minValue;
	max.color = maxValue;
}

