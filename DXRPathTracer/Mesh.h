#pragma once
#include "Array.h"
#include "basic_types.h"


struct Vertex
{
	float3 position;
	float3 normal;
	float2 texcoord;
};

typedef uint3 Tridex;

/*
We define the word 'tridex' for clarity. It means triple indices for a triangle w.r.t some vetex buffer.
For example, the following structure could be defined.

struct Triangle
{
	uint3 tridex;
	float3 normal;
	float area;
}
*/

struct Mesh
{
	Array<Vertex> vtxArr;
	Array<Tridex> tdxArr;
};