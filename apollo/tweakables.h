#pragma once

#include <cstdio>
#include <unordered_map>
#include <vector>
#include "math_helpers.h"
#include "rendering.h"

#define TWEAKABLE(type, variable, text, initial_value, min, max)  \
	type variable { initial_value }; \
	Tweakables::AutoRegister auto_tweakable_##variable(text, variable, min, max)

#define REGISTER_TWEAKABLE(name, variable, min, max) Tweakables::GetInstance().Register(name, variable, min, max)


class Tweakables
{
public:
	template <typename TweakableType>
	bool Tweakables::Register(const char* name, TweakableType& variable, TweakableType minValue, TweakableType maxValue)
	{
		InsertTweakable(name, &variable, minValue, maxValue);
		return true;
	}


	// setters and getters for each type
	int GetInt(const char* name) const;
	void SetInt(const char* name, int value);

	float GetFloat(const char* name) const;
	void SetFloat(const char* name, float value);

	Vector2 GetVector2(const char* name) const;
	void SetVector2(const char* name, Vector2 value);

	Vector3 GetVector3(const char* name) const;
	void SetVector3(const char* name, Vector3 value);

	Color GetColor(const char* name) const;
	void SetColor(const char* name, Color value);



	struct Tweakable
	{
		Tweakable(const char* name, int* valuePtr, int minValue, int maxValue);
		Tweakable(const char* name, float* valuePtr, float minValue, float maxValue);
		Tweakable(const char* name, Vector2* valuePtr, Vector2 minValue, Vector2 maxValue);
		Tweakable(const char* name, Vector3* valuePtr, Vector3 minValue, Vector3 maxValue);
		Tweakable(const char* name, Color* valuePtr, Color minValue, Color maxValue);

		const char* name;
		enum class Type { Int, Float, Vector2, Vector3, Color };
		Type type;

		union TweakablePtr
		{
			TweakablePtr() {}
			int* i;
			float* f;
			Vector2* vec2;
			Vector3* vec3;
			Color* color;
		};
		TweakablePtr value;

		union TweakableValue
		{
			TweakableValue() {}
			int i;
			float f;
			Vector2 vec2;
			Vector3 vec3;
			Color color;
		};
		TweakableValue min;
		TweakableValue max;
	};	
	const std::vector<Tweakable>& GetTweakables() { return m_tweakables; }


	static Tweakables& GetInstance()
	{
		static Tweakables instance;
		return instance;
	}


	class AutoRegister
	{
	public:
		template <typename T>
		AutoRegister(const char* name, T& variable, T minValue, T maxValue);

	private:
		void* m_variablePtr;
	};


private:
	template <typename TweakableType>
	Tweakable& InsertTweakable(const char* name, TweakableType* valuePtr, TweakableType minValue, TweakableType maxValue);

	const Tweakable* GetTweakable(const char* name) const;
	Tweakable* GetTweakable(const char* name);

	std::vector<Tweakable> m_tweakables;
};


template <typename T>
Tweakables::AutoRegister::AutoRegister(const char* name, T& variable, T minValue, T maxValue)
	: m_variablePtr(&variable)
{
	if (!Tweakables::GetInstance().Register(name, variable, minValue, maxValue))
		m_variablePtr = nullptr;
}


template <typename TweakableType>
Tweakables::Tweakable& Tweakables::InsertTweakable(const char* name, TweakableType* valuePtr, TweakableType minValue, TweakableType maxValue)
{
	auto nameCompare = [] (const Tweakable& tweakable, const char* name) {
		return strcmp(tweakable.name, name) < 0;
	};
	auto iter = lower_bound(begin(m_tweakables), end(m_tweakables), name, nameCompare);
	assert((iter == end(m_tweakables)) || (strcmp(iter->name, name) > 0));
	iter = m_tweakables.emplace(iter, name, valuePtr, minValue, maxValue);
	return *iter;
}
