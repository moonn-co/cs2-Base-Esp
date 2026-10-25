#pragma once
#include <numbers>
#include <cmath>
#include <limits>

// The project is built with MSVC, but keeping the hint portable costs nothing
// and lets the geometry headers be syntax-checked with other compilers.
#if defined(_MSC_VER)
#define CS2_FORCEINLINE __forceinline
#else
#define CS2_FORCEINLINE inline
#endif

class Vec3_t {
public:
	float x, y, z;
public:
	CS2_FORCEINLINE Vec3_t() : x{}, y{}, z{} {}
	CS2_FORCEINLINE Vec3_t(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}

	CS2_FORCEINLINE float& at(const size_t index) {
		return ((float*)this)[index];
	}
	CS2_FORCEINLINE float& at(const size_t index) const {
		return ((float*)this)[index];
	}

	CS2_FORCEINLINE float& operator( )(const size_t index) {
		return at(index);
	}
	CS2_FORCEINLINE const float& operator( )(const size_t index) const {
		return at(index);
	}
	CS2_FORCEINLINE float& operator[ ](const size_t index) {
		return at(index);
	}
	CS2_FORCEINLINE const float& operator[ ](const size_t index) const {
		return at(index);
	}

	CS2_FORCEINLINE bool operator==(const Vec3_t& v) const {
		return v.x == x && v.y == y && v.z == z;
	}
	CS2_FORCEINLINE bool operator!=(const Vec3_t& v) const {
		return v.x != x || v.y != y || v.z != z;
	}

	CS2_FORCEINLINE Vec3_t& operator=(const Vec3_t& v) = default;

	CS2_FORCEINLINE Vec3_t operator-() const {
		return Vec3_t{ -x, -y, -z };
	}

	CS2_FORCEINLINE Vec3_t operator+(const Vec3_t& v) const {
		return {
			x + v.x,
			y + v.y,
			z + v.z
		};
	}

	CS2_FORCEINLINE Vec3_t operator-(const Vec3_t& v) const {
		return {
			x - v.x,
			y - v.y,
			z - v.z
		};
	}

	CS2_FORCEINLINE Vec3_t operator*(const Vec3_t& v) const {
		return {
			x * v.x,
			y * v.y,
			z * v.z
		};
	}

	CS2_FORCEINLINE Vec3_t operator/(const Vec3_t& v) const {
		return {
			x / v.x,
			y / v.y,
			z / v.z
		};
	}

	CS2_FORCEINLINE Vec3_t& operator+=(const Vec3_t& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t& operator-=(const Vec3_t& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t& operator*=(const Vec3_t& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t& operator/=(const Vec3_t& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t operator+(float f) const {
		return {
			x + f,
			y + f,
			z + f
		};
	}

	CS2_FORCEINLINE Vec3_t operator-(float f) const {
		return {
			x - f,
			y - f,
			z - f
		};
	}

	CS2_FORCEINLINE Vec3_t operator*(float f) const {
		return {
			x * f,
			y * f,
			z * f
		};
	}

	CS2_FORCEINLINE Vec3_t operator/(float f) const {
		return {
			x / f,
			y / f,
			z / f
		};
	}

	CS2_FORCEINLINE Vec3_t& operator+=(float f) {
		x += f;
		y += f;
		z += f;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t& operator-=(float f) {
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t& operator*=(float f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	CS2_FORCEINLINE Vec3_t& operator/=(float f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	CS2_FORCEINLINE void clear() {
		x = y = z = 0.f;
	}

	CS2_FORCEINLINE float length_sqr() const {
		return ((x * x) + (y * y) + (z * z));
	}

	CS2_FORCEINLINE float length_2d_sqr() const {
		return ((x * x) + (y * y));
	}

	CS2_FORCEINLINE float length() const {
		return std::sqrt(length_sqr());
	}

	CS2_FORCEINLINE float length_2d() const {
		return std::sqrt(length_2d_sqr());
	}

	CS2_FORCEINLINE float dot(const Vec3_t& v) const {
		return (x * v.x + y * v.y + z * v.z);
	}

	CS2_FORCEINLINE float dot(float* v) const {
		return (x * v[0] + y * v[1] + z * v[2]);
	}

	constexpr bool zero() const noexcept {
		return x == 0.f && y == 0.f && z == 0.f;
	}

	CS2_FORCEINLINE Vec3_t cross(const Vec3_t& v) const {
		return {
			(y * v.z) - (z * v.y),
			(z * v.x) - (x * v.z),
			(x * v.y) - (y * v.x)
		};
	}

	CS2_FORCEINLINE float dist_to(const Vec3_t& vOther) const {
		Vec3_t delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.length_2d();
	}

	CS2_FORCEINLINE float dist_to_3d(const Vec3_t& vOther) const {
		Vec3_t delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.length();
	}

	Vec3_t ToAngle() const noexcept {
		return Vec3_t{
			std::atan2(-z, std::hypot(x, y)) * (180.0f / std::numbers::pi_v<float>),
			std::atan2(y, x) * (180.0f / std::numbers::pi_v<float>),
			0.0f
		};
	}

	CS2_FORCEINLINE float normalize() {
		float len = length();

		(*this) /= (length() + std::numeric_limits<float>::epsilon());

		return len;
	}

	CS2_FORCEINLINE Vec3_t normalized() const {
		auto vec = *this;

		vec.normalize();

		return vec;
	}
};
