#pragma once
#include "basic_types.h"
#include <cmath>
#define PI 3.14159265358979323846f
#define DEGREE (PI / 180.0f)

template<typename T> inline constexpr T _min(T x, T y)
{
	return x < y ? x : y;
}

template<typename T> inline constexpr T _max(T x, T y)
{
	return x > y ? x : y;
}

template<typename T> inline constexpr T _clamp(T value, T lowerBound, T upperBound)
{
	return _min( _max(lowerBound, value), upperBound);
}

inline int2 operator-(const int2& v, const int2& w)
{
	return int2(v.x-w.x, v.y-w.y);
}

inline float3 operator+(const float3& v, const float3& w)
{
	return float3(v.x+w.x, v.y+w.y, v.z+w.z);
}
inline float3 operator-(const float3& v, const float3& w)
{
	return float3(v.x-w.x, v.y-w.y, v.z-w.z);
}
inline float3 operator-(const float3& v)
{
	return float3(-v.x, -v.y, -v.z);
}
template<typename T>
inline float3 operator*(const float3& v, T s)
{
	float ss = (float)s;
	return float3(v.x*ss, v.y*ss, v.z*ss);
}
template<typename T>
inline float3 operator*(T s, const float3& v)
{
	float ss = (float)s;
	return float3(v.x*ss, v.y*ss, v.z*ss);
}
template<typename T>
inline float3 operator/(const float3& v, T s)
{
	float ss = 1.0f / (float)s;
	return float3(v.x*ss, v.y*ss, v.z*ss);
}
inline float dot(const float3& v, const float3& w)
{
	return v.x * w.x + v.y * w.y + v.z * w.z;
}
inline float squaredLength(const float3& v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}
inline float length(const float3& v)
{
	return sqrtf(squaredLength(v));
}
inline float3 normalize(const float3& v)
{
	return v / length(v);
}
inline float3 cross(const float3& v, const float3& w)
{
	return float3(v.y*w.z - v.z*w.y, v.z*w.x - v.x*w.z, v.x*w.y - v.y*w.x);
}

inline Transform composeMatrix(const float3& translation, const float4& rotation, float scale)
{
	Transform ret;
	ret.mat[0][0] = scale * (1.f - 2.f * (rotation.y*rotation.y + rotation.z*rotation.z));
	ret.mat[0][1] = scale * (2.f * (rotation.x*rotation.y - rotation.z*rotation.w));
	ret.mat[0][2] = scale * (2.f * (rotation.x*rotation.z + rotation.y*rotation.w));
	ret.mat[0][3] = translation.x;			
	ret.mat[1][0] = scale * (2.f * (rotation.x*rotation.y + rotation.z*rotation.w));
	ret.mat[1][1] = scale * (1.f - 2.f * (rotation.x*rotation.x + rotation.z*rotation.z));
	ret.mat[1][2] = scale * (2.f * (rotation.y*rotation.z - rotation.x*rotation.w));
	ret.mat[1][3] = translation.y;			
	ret.mat[2][0] = scale * (2.f * (rotation.x*rotation.z - rotation.y*rotation.w));
	ret.mat[2][1] = scale * (2.f * (rotation.y*rotation.z + rotation.x*rotation.w));
	ret.mat[2][2] = scale * (1.f - 2.f * (rotation.x*rotation.x + rotation.y*rotation.y));
	ret.mat[2][3] = translation.z;
	ret.mat[3][0] = 0.f;
	ret.mat[3][1] = 0.f;
	ret.mat[3][2] = 0.f;
	ret.mat[3][3] = 1.f;
	return ret;
}

inline void composeMatrix(float matrix[16], const float3& translation, const float4& rotation, float scale)
{
	matrix[ 0] = scale * (1.f - 2.f * (rotation.y*rotation.y + rotation.z*rotation.z));
	matrix[ 1] = scale * (2.f * (rotation.x*rotation.y - rotation.z*rotation.w));
	matrix[ 2] = scale * (2.f * (rotation.x*rotation.z + rotation.y*rotation.w));
	matrix[ 3] = translation.x;			
	matrix[ 4] = scale * (2.f * (rotation.x*rotation.y + rotation.z*rotation.w));
	matrix[ 5] = scale * (1.f - 2.f * (rotation.x*rotation.x + rotation.z*rotation.z));
	matrix[ 6] = scale * (2.f * (rotation.y*rotation.z - rotation.x*rotation.w));
	matrix[ 7] = translation.y;			
	matrix[ 8] = scale * (2.f * (rotation.x*rotation.z - rotation.y*rotation.w));
	matrix[ 9] = scale * (2.f * (rotation.y*rotation.z + rotation.x*rotation.w));
	matrix[10] = scale * (1.f - 2.f * (rotation.x*rotation.x + rotation.y*rotation.y));
	matrix[11] = translation.z;
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
	matrix[15] = 1.f;
}

inline float4 getRotationAsQuternion(const float3& axis, float degree)
{
	float angle = degree * DEGREE;

	return float4(sinf(0.5f*angle) * axis, cosf(0.5f*angle));
}