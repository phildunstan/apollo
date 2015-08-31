#pragma once

#include <cstdio>
#include <unordered_map>
#include "math_helpers.h"
#include "rendering.h"

#define TWEAKABLE(type, variable, initial_value, min, max)  \
	type variable { initial_value }; \
	Tweakables::AutoRegister auto_tweakable_##variable(#variable, variable, min, max)

#define REGISTER_TWEAKABLE(name, variable, min, max) Tweakables::GetInstance().Register(name, variable, min, max)


class Tweakables
{
public:
	template <typename T>
	struct Tweakable
	{
		const char* name;
		T* variablePtr;
		T minValue;
		T maxValue;
	};

	template <typename T>
	bool Register(const char* name, T& variable, T minValue, T maxValue)
	{
		auto& map = GetTweakablesMap<T>();
		assert(map.find(name) == end(map));
		map[name] = { name, &variable, minValue, maxValue };
		return true;
	}

	void Deregister(void* variablePtr);

	// setters and getters for each type
	const auto GetIntVariables() const { return m_ints; }
	int GetInt(const char* name);
	void SetInt(const char* name, int value);

	const auto& GetFloatVariables() const { return m_floats; }
	float GetFloat(const char* name);
	void SetFloat(const char* name, float value);

	const auto& GetVector2Variables() const { return m_vector2s; }
	Vector2 GetVector2(const char* name);
	void SetVector2(const char* name, Vector2 value);

	const auto& GetVector3Variables() const { return m_vector3s; }
	Vector3 GetVector3(const char* name);
	void SetVector3(const char* name, Vector3 value);

	const auto& GetColorVariables() const { return m_colors; }
	Color GetColor(const char* name);
	void SetColor(const char* name, Color value);




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
		~AutoRegister();
	private:
		void* m_variablePtr;
	};


private:
	template <typename T>
	auto& GetTweakablesMap()
	{
		fprintf(stderr, "Tweakables does not support this type.");
		assert(false);
	}
	template <> auto& GetTweakablesMap<int>() { return m_ints; }
	template <> auto& GetTweakablesMap<float>() { return m_floats; }
	template <> auto& GetTweakablesMap<Vector2>() { return m_vector2s; }
	template <> auto& GetTweakablesMap<Vector3>() { return m_vector3s; }
	template <> auto& GetTweakablesMap<Color>() { return m_colors; }

	std::unordered_map<const char*, Tweakable<int>> m_ints;
	std::unordered_map<const char*, Tweakable<float>> m_floats;
	std::unordered_map<const char*, Tweakable<Vector2>> m_vector2s;
	std::unordered_map<const char*, Tweakable<Vector3>> m_vector3s;
	std::unordered_map<const char*, Tweakable<Color>> m_colors;
};


template <typename T>
Tweakables::AutoRegister::AutoRegister(const char* name, T& variable, T minValue, T maxValue)
	: m_variablePtr(&variable)
{
	if (!Tweakables::GetInstance().Register(name, variable, minValue, maxValue))
		m_variablePtr = nullptr;
}

