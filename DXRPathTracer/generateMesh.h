#pragma once
#include "Mesh.h"
#include "basic_types.h"

enum FaceDir { unknown=-1, down, up, front, back, left, right };

Mesh generateParallelogramMesh(const float3& corner, const float3& side1, const float3& side2);
Mesh generateRectangleMesh(const float3& center, const float3& size, FaceDir dir);
Mesh generateBoxMesh(const float3& lowerCroner, const float3& upperCroner);
Mesh generateCubeMesh(const float3& center, const float3& size, bool bottomCenter=false);
Mesh generateSphereMesh(const float3& center, float radius, uint numSegmentsInMeridian=50, uint numSegmentsInEquator=100);