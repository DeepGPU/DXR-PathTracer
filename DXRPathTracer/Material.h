#pragma once
#include "basic_types.h"

//enum class MaterialType : uint
//{
//	Lambertian = 0,
//	//Phong
//	//Disney
//};

static const int Lambertian = 0;
static const int Metal = 1;
static const int Plastic = 2;
static const int Glass = 3;
static const int Plastic1 = 201;
static const int Plastic2 = 202;
static const int Plastic3 = 203;

struct Material 
{
	float3 emittance	 = float3(0.0f);
	uint type			 = Lambertian;
	float3 albedo		 = float3(0.1f);
	float roughness		 = 0.01f;
	float reflectivity	 = 0.1f;
	float transmittivity = 0.96f;	// transmittivity of glass at normal incidence
};
