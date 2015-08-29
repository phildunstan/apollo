#pragma once

#include <cstdio>
#include <unordered_map>

#define TWEAKABLE(type, variable, initial_value, min, max)  type variable { initial_value }; \
															Tweakables::AutoRegister auto_tweakable_##variable(#variable, variable, min, max);

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
		fprintf(stderr, "Tweakables does not support this type.");
	}

	template <typename T>
	T GetValue(const char* name)
	{
		fprintf(stderr, "Tweakables does not support this type.");
	}

	template <typename T>
	void SetValue(const char* name, T value)
	{
		fprintf(stderr, "Tweakables does not support this type.");
	}


	// overloads for supported types
	const std::unordered_map<const char*, Tweakable<float>>& GetFloatVariables() const { return m_floats; }
	bool Register(const char* name, float& variable, float minValue, float maxValue);
	float GetValue(const char* name);
	void SetValue(const char* name, float value);

	void Deregister(void* variablePtr);



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
	std::unordered_map<const char*, Tweakable<float>> m_floats;
};


template <typename T>
Tweakables::AutoRegister::AutoRegister(const char* name, T& variable, T minValue, T maxValue)
	: m_variablePtr(&variable)
{
	if (!Tweakables::GetInstance().Register(name, variable, minValue, maxValue))
		m_variablePtr = nullptr;
}

