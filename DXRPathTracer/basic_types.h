#pragma once

typedef wchar_t wchar;
typedef unsigned char uint8;
typedef unsigned int uint;
typedef unsigned long long uint64;

struct float2
{
	union {
		float data[2];
		struct { float x, y; };
	};

	float2() {}
	//float2(const float2& v) :x(v.x), y(v.y) {}
	template<typename X>
	float2(X x) : x((float)x), y((float)x) {}
	template<typename X, typename Y>
	float2(X x, Y y) : x((float)x), y((float)y) {}
	float& operator[](int order) { return data[order]; }
	float operator[](int order) const { return data[order]; }
	//float2& operator=(const float2& v) { x = v.x, y = v.y; return *this; }
};

struct float3
{
	union {
		float data[3];
		struct {
			union {
				struct { float x, y; };
				float2 xy;
			};
			float z;
		};
	};

	float3() {}
	//float3(const float3& v) :x(v.x), y(v.y), z(v.z) {}
	template<typename X>
	float3(X x) : x((float)x), y((float)x), z((float)x) {}
	template<typename X, typename Y, typename Z>
	float3(X x, Y y, Z z) : x((float)x), y((float)y), z((float)z) {}
	float3(const float2& xy, float z) : xy(xy), z(z) {}
	float& operator[](int order) { return data[order]; }
	float operator[](int order) const { return data[order]; }
	//float3& operator=(const float3& v) { x = v.x, y = v.y, z = v.z; return *this; }
};

struct float4
{
	union {
		float data[4];
		struct { 
			union {
				struct { float x, y, z; };
				float3 xyz;
			};
			float w; 
		};
	};

	float4() {}
	//float4(const float4& v) :x(v.x), y(v.y), z(v.z), w(v.w) {}
	template<typename X>
	float4(X x) : x((float)x), y((float)x), z((float)x), w((float)x) {}
	template<typename X, typename Y, typename Z, typename W>
	float4(X x, Y y, Z z, W w) : x((float)x), y((float)y), z((float)z), w((float)w) {}
	float4(const float3& xyz, float w) : xyz(xyz), w(w) {}
	float& operator[](int order) { return data[order]; }
	float operator[](int order) const { return data[order]; }
	//float4& operator=(const float4& v) { x = v.x, y = v.y, z = v.z, w = v.w; return *this; }
};

struct uint2;
struct int2
{
	union {
		int data[2];
		struct { int x, y; };
		struct { int i, j; };
	};

	int2() {}
	//int2(const int2& v) :x(v.x), y(v.y) {}
	template<typename I, typename J>
	int2(I i, J j) : i((int)i), j((int)j) {}
	int& operator[](int order) { return data[order]; }
	int operator[](int order) const { return data[order]; }
	explicit operator uint2();
	//uint2& operator=(const uint2& v) { x = v.x, y = v.y; return *this; }
};

struct uint2
{
	union {
		uint data[2];
		struct { uint x, y; };
		struct { uint i, j; };
	};

	uint2() {}
	//uint2(const uint2& v) :x(v.x), y(v.y) {}
	template<typename I, typename J>
	uint2(I i, J j) : i((uint)i), j((uint)j) {}
	uint& operator[](int order) { return data[order]; }
	uint operator[](int order) const { return data[order]; }
	explicit operator int2();
	//uint2& operator=(const uint2& v) { x = v.x, y = v.y; return *this; }
};

inline int2::operator uint2()
{
	return uint2( (uint) i, (uint) j );
}

inline uint2::operator int2()
{
	return int2( (int) i, (int) j );
}

struct uint3
{
	union {
		uint data[3];
		struct { uint x, y, z; };
		struct { uint i, j, k; };
	};

	uint3() {}
	//uint3(const uint3& v) :x(v.x), y(v.y), z(v.z) {}
	template<typename I, typename J, typename K>
	uint3(I i, J j, K k) : i((uint)i), j((uint)j), k((uint)k) {}
	uint& operator[](int order) { return data[order]; }
	uint operator[](int order) const { return data[order]; }
	//uint3& operator=(const uint3& v) { x = v.x, y = v.y, z = v.z; return *this; }
};

struct Transform
{
	float mat[4][4];
	Transform() {}
	constexpr Transform(float scala) : mat{}
	{
		mat[0][0] = mat[1][1] = mat[2][2] = scala;
		mat[3][3] = 1.0f;
	}
	static constexpr Transform identity()
	{
		return Transform(1.0f);
	}
};
