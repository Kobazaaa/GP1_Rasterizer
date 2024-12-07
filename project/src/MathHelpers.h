#pragma once
#include <cfloat>
#include <cmath>

namespace dae
{
	/* --- HELPER STRUCTS --- */
	struct Int2
	{
		int x{};
		int y{};
	};

	/* --- CONSTANTS --- */
	constexpr auto PI = 3.14159265358979323846f;
	constexpr auto PI_DIV_2 = 1.57079632679489661923f;
	constexpr auto PI_DIV_4 = 0.785398163397448309616f;
	constexpr auto PI_2 = 6.283185307179586476925f;
	constexpr auto PI_4 = 12.56637061435917295385f;
	constexpr auto ONE_DIV_PI = 0.31830988618379067153776752674503f;
	constexpr auto ONE_DIV_2PI = 0.15915494309189533576888376337251f;

	constexpr auto TO_DEGREES = (180.0f * ONE_DIV_PI);
	constexpr auto TO_RADIANS(PI / 180.0f);

	/* --- HELPER FUNCTIONS --- */
	inline float Square(float a)
	{
		return a * a;
	}

	// Returns -1 if < 0, returns 1 if > 0, returns 0 if == 0
	inline int SignOf(float value)
	{
		if (value > -FLT_EPSILON and value < FLT_EPSILON) return 0;

		return value < 0.f ? -1 : 1;
	}

	inline float Lerpf(float a, float b, float factor)
	{
		return ((1 - factor) * a) + (factor * b);
	}
	inline float Remap01(float value, float rangeLow, float rangeHigh)
	{
		if (value < rangeLow)	return 0;
		if (value > rangeHigh)	return 1;
		return ((value - rangeLow) / (rangeHigh - rangeLow));
	}

	inline bool AreEqual(float a, float b, float epsilon = FLT_EPSILON)
	{
		return (a - b) < epsilon and (a - b) > -epsilon;
	}

	inline int Clamp(const int v, int min, int max)
	{
		if (v < min) return min;
		if (v > max) return max;
		return v;
	}

	inline float Clamp(const float v, float min, float max)
	{
		if (v < min) return min;
		if (v > max) return max;
		return v;
	}

	inline float Saturate(const float v)
	{
		if (v < 0.f) return 0.f;
		if (v > 1.f) return 1.f;
		return v;
	}
}