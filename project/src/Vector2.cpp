#include "Vector2.h"

#include <cassert>

#include <cmath>

#include "MathHelpers.h"

namespace dae {
	const Vector2 Vector2::UnitX = Vector2{ 1, 0 };
	const Vector2 Vector2::UnitY = Vector2{ 0, 1 };
	const Vector2 Vector2::Zero = Vector2{ 0, 0 };

	Vector2::Vector2(float _x, float _y) : x{ _x }, y{ _y } {}


	Vector2::Vector2(const Vector2& from, const Vector2& to) : x(to.x - from.x), y(to.y - from.y) {}

	float Vector2::Magnitude() const
	{
		return sqrtf(x * x + y * y);
	}

	float Vector2::SqrMagnitude() const
	{
		return x * x + y * y;
	}

	float Vector2::Normalize()
	{
		const float m = Magnitude();
		const float mInv = 1.f / m;

		x *= mInv;
		y *= mInv;

		return m;
	}

	Vector2 Vector2::Normalized() const
	{
		const float m = Magnitude();
		const float mInv = 1.f / m;
		return { x * mInv, y * mInv };
	}

	float Vector2::Dot(const Vector2& v1, const Vector2& v2)
	{
		return v1.x * v2.x + v1.y * v2.y;
	}

	float Vector2::Cross(const Vector2& v1, const Vector2& v2)
	{
		return v1.x * v2.y - v1.y * v2.x;
	}

	Vector2 Vector2::Max(const Vector2& v1, const Vector2& v2)
	{
		return {
			std::fmax(v1.x, v2.x),
			std::fmax(v1.y, v2.y),
		};
	}
	Vector2 Vector2::Min(const Vector2& v1, const Vector2& v2)
	{
		return {
			std::fmin(v1.x, v2.x),
			std::fmin(v1.y, v2.y),
		};
	}

	bool Vector2::NearZero()
	{
		float epsilon = 0.0001f;
		return (std::abs(x) < epsilon) and (std::abs(y) < epsilon);
	}

#pragma region Operator Overloads
	Vector2 Vector2::operator*(float scale) const
	{
		return { x * scale, y * scale };
	}

	Vector2 Vector2::operator/(float scale) const
	{
		const float invScale = 1.f / scale;
		return { x * invScale, y * invScale };
	}

	Vector2 Vector2::operator+(const Vector2& v) const
	{
		return { x + v.x, y + v.y };
	}

	Vector2 Vector2::operator-(const Vector2& v) const
	{
		return { x - v.x, y - v.y };
	}

	Vector2 Vector2::operator-() const
	{
		return { -x ,-y };
	}

	Vector2& Vector2::operator*=(float scale)
	{
		x *= scale;
		y *= scale;
		return *this;
	}

	Vector2& Vector2::operator/=(float scale)
	{
		const float invScale = 1.f / scale;
		x *= invScale;
		y *= invScale;
		return *this;
	}

	Vector2& Vector2::operator-=(const Vector2& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vector2& Vector2::operator+=(const Vector2& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	float& Vector2::operator[](int index)
	{
		assert(index <= 1 && index >= 0);
		return index == 0 ? x : y;
	}

	float Vector2::operator[](int index) const
	{
		assert(index <= 1 && index >= 0);
		return index == 0 ? x : y;
	}

	bool Vector2::operator==(const Vector2& v) const
	{
		return AreEqual(x, v.x) && AreEqual(y, v.y);
	}
#pragma endregion
}
