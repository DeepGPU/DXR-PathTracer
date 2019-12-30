#include "pch.h"
#include "Mesh.h"

enum class FaceDir { down, up, front, back, left, right };

Mesh generateParallelogramMesh(const float3& corner, const float3& side1, const float3& side2)
{
	Mesh mesh;
	float3 faceNormal = normalize(cross(side1, side2));
	
	mesh.vtxArr.resize(4);
	mesh.vtxArr[0].position = corner;
	mesh.vtxArr[1].position = corner + side1;
	mesh.vtxArr[2].position = corner + side1 + side2;
	mesh.vtxArr[3].position = corner + side2;
	mesh.vtxArr[0].normal = faceNormal;
	mesh.vtxArr[1].normal = faceNormal;
	mesh.vtxArr[2].normal = faceNormal;
	mesh.vtxArr[3].normal = faceNormal;
	mesh.vtxArr[0].texcoord = float2(0.f, 0.f);
	mesh.vtxArr[1].texcoord = float2(0.f, 0.f);
	mesh.vtxArr[2].texcoord = float2(0.f, 0.f);
	mesh.vtxArr[3].texcoord = float2(0.f, 0.f);

	mesh.tdxArr.resize(2);
	mesh.tdxArr[0] = uint3(0, 1, 2);
	mesh.tdxArr[1] = uint3(2, 3, 0);

	return mesh;
}

Mesh generateRectangleMesh(const float3& center, const float3& size, FaceDir dir)
{
	float3 corner, side1, side2;

	if (dir == FaceDir::up || dir == FaceDir::down)
	{
		if (size.x == 0.f || size.y != 0.f || size.z == 0.f)
		{
			throw Error("You must give width and depth values for up or down faced rectangle.");
		}
		side1 = float3(0.f, 0.f, size.z);
		side2 = float3(size.x, 0.f, 0.f);
		
		if (dir == FaceDir::down)
			side2 = -side2;
	}
	else if (dir == FaceDir::front || dir == FaceDir::back)
	{
		if (size.x == 0.f || size.y == 0.f || size.z != 0.f)
		{
			throw Error("You must give width and height values for front or back faced rectangle.");
		}
		side1 = float3(size.x, 0.f, 0.f);
		side2 = float3(0.f, size.y, 0.f);

		if (dir == FaceDir::back)
			side2 = -side2;
	}
	else if (dir == FaceDir::left || dir == FaceDir::right)
	{
		if (size.x != 0.f || size.y == 0.f || size.z == 0.f)
		{
			throw Error("You must give height and depth values for left or right faced rectangle.");
		}
		side1 = float3(0.f, size.y, 0.f);
		side2 = float3(0.f, 0.f, size.z);

		if (dir == FaceDir::left)
			side2 = -side2;
	}

	corner = center - 0.5f * (side1 + side2);

	return generateParallelogramMesh(corner, side1, side2);
}

Mesh generateBoxMesh(const float3& lowerCroner, const float3& upperCroner)
{
	float3 diff = upperCroner - lowerCroner;
	Mesh top	= generateParallelogramMesh(upperCroner, float3(0.f, 0.f, -diff.z), float3(-diff.x, 0.f, 0.f));
	Mesh right	= generateParallelogramMesh(upperCroner, float3(0.f, -diff.y, 0.f), float3(0.f, 0.f, -diff.z));
	Mesh front	= generateParallelogramMesh(upperCroner, float3(-diff.x, 0.f, 0.f), float3(0.f, -diff.y, 0.f));
	Mesh bottom	= generateParallelogramMesh(lowerCroner, float3(diff.x, 0.f, 0.f), float3(0.f, 0.f, diff.z));
	Mesh left	= generateParallelogramMesh(lowerCroner, float3(0.f, 0.f, diff.z), float3(0.f, diff.y, 0.f));
	Mesh back	= generateParallelogramMesh(lowerCroner, float3(0.f, diff.y, 0.f), float3(diff.x, 0.f, 0.f));

	Mesh box;
	box.vtxArr.resize(24);
	memcpy(box.vtxArr.data() + 0,	top.vtxArr.data(),		4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 4,	bottom.vtxArr.data(),	4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 8,	left.vtxArr.data(),		4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 12,	right.vtxArr.data(),	4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 16,	front.vtxArr.data(),	4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 20,	back.vtxArr.data(),		4 * sizeof(Vertex));

	box.tdxArr.resize(12);
	memcpy(box.tdxArr.data() + 0,	top.tdxArr.data(),		2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 2,	bottom.tdxArr.data(),	2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 4,	left.tdxArr.data(),		2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 6,	right.tdxArr.data(),	2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 8,	front.tdxArr.data(),	2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 10,	back.tdxArr.data(),		2 * sizeof(Tridex));

	for (uint i = 1; i < 6; ++i)
	{
		box.tdxArr[i*2 + 0].x += i * 4;
		box.tdxArr[i*2 + 0].y += i * 4;
		box.tdxArr[i*2 + 0].z += i * 4;
		box.tdxArr[i*2 + 1].x += i * 4;
		box.tdxArr[i*2 + 1].y += i * 4;
		box.tdxArr[i*2 + 1].z += i * 4;
	}

	return box;
}

Mesh generateCubeMesh(const float3& center, const float3& size, bool bottomCenter)
{
	float3 add = bottomCenter ? float3(0.f, size.y / 2.f, 0.f) : float3(0.f);
	
	Mesh top	= generateRectangleMesh(center + float3(0.f,  size.y / 2.f, 0.f) + add, float3(size.x, 0.f, size.z), FaceDir::up);
	Mesh bottom	= generateRectangleMesh(center + float3(0.f, -size.y / 2.f, 0.f) + add, float3(size.x, 0.f, size.z), FaceDir::down);
	Mesh left	= generateRectangleMesh(center + float3(-size.x / 2.f, 0.f, 0.f) + add, float3(0.f, size.y, size.z), FaceDir::left);
	Mesh right	= generateRectangleMesh(center + float3( size.x / 2.f, 0.f, 0.f) + add, float3(0.f, size.y, size.z), FaceDir::right);
	Mesh front	= generateRectangleMesh(center + float3(0.f, 0.f,  size.z / 2.f) + add, float3(size.x, size.y, 0.f), FaceDir::front);
	Mesh back	= generateRectangleMesh(center + float3(0.f, 0.f, -size.z / 2.f) + add, float3(size.x, size.y, 0.f), FaceDir::back);

	Mesh box;
	box.vtxArr.resize(24);
	memcpy(box.vtxArr.data() + 0,	top.vtxArr.data(),		4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 4,	bottom.vtxArr.data(),	4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 8,	left.vtxArr.data(),		4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 12,	right.vtxArr.data(),	4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 16,	front.vtxArr.data(),	4 * sizeof(Vertex));
	memcpy(box.vtxArr.data() + 20,	back.vtxArr.data(),		4 * sizeof(Vertex));

	box.tdxArr.resize(12);
	memcpy(box.tdxArr.data() + 0,	top.tdxArr.data(),		2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 2,	bottom.tdxArr.data(),	2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 4,	left.tdxArr.data(),		2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 6,	right.tdxArr.data(),	2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 8,	front.tdxArr.data(),	2 * sizeof(Tridex));
	memcpy(box.tdxArr.data() + 10,	back.tdxArr.data(),		2 * sizeof(Tridex));

	for (uint i = 1; i < 6; ++i)
	{
		box.tdxArr[i*2 + 0].x += i * 4;
		box.tdxArr[i*2 + 0].y += i * 4;
		box.tdxArr[i*2 + 0].z += i * 4;
		box.tdxArr[i*2 + 1].x += i * 4;
		box.tdxArr[i*2 + 1].y += i * 4;
		box.tdxArr[i*2 + 1].z += i * 4;
	}

	return box;
}

Mesh generateSphereMesh(const float3& center, float radius, uint numSegmentsInMeridian, uint numSegmentsInEquator)
{
	uint M = numSegmentsInMeridian;
	uint E = numSegmentsInEquator;

	if (M < 2)
		throw Error("Give number of sides of meridian more than two.");

	if (E < 3)
		throw Error("Give number of sides of equator more than three.");

	Mesh mesh;

	mesh.vtxArr.resize( (M-1) * E + 2 );
	mesh.tdxArr.resize( ((M - 2) * 2 + 2) * E );

	const float meridianStep = PI / M;
	const float equatorStep = 2*PI / E;
	float phi;		// latitude which ranges from 0¡Æ at the Equator to 90¡Æ (North or South) at the poles.
	float lambda;	// longitude 
	
	uint vertexCount = 0;
	uint triangleCount = 0;
	
	mesh.vtxArr[vertexCount].position = center + radius * float3(0.f, 1.f, 0.f);
	mesh.vtxArr[vertexCount].normal = float3(0.f, 1.f, 0.f);
	mesh.vtxArr[vertexCount].texcoord = float2(0.f, 0.f);
	++vertexCount;

	for (uint e = 1; e <= E; ++e)
	{
		mesh.tdxArr[triangleCount++] = { 0, e, e%E + 1 };
	}

	for (uint m = 1; m < M; ++m)
	{
		phi = 0.5f*PI - m * meridianStep;
		
		for (uint e = 1; e <= E; ++e)
		{
			lambda = e * equatorStep;
			float cosPhi = cosf(phi);
			float x = cosPhi * sinf(lambda);
			float y = sinf(phi);
			float z = cosPhi * cosf(lambda);

			if(m < M - 1)
			{
				uint i = vertexCount;
				uint j = (e < E) ? (i + 1) : (i + 1 - E);
				uint k = i + E;
				uint l = j + E;

				mesh.tdxArr[triangleCount++] = { j, i ,k };
				mesh.tdxArr[triangleCount++] = { j, k, l };
			}

			mesh.vtxArr[vertexCount].position = center + radius * float3(x, y, z);
			mesh.vtxArr[vertexCount].normal = float3(x, y, z);
			mesh.vtxArr[vertexCount].texcoord = float2(0.f, 0.f);
			++vertexCount;
		}
	}

	uint baseIdx = vertexCount - E - 1;
	for (uint e = 1; e <= E; ++e)
	{
		mesh.tdxArr[triangleCount++] = { vertexCount, baseIdx + e%E + 1, baseIdx + e };
	}

	mesh.vtxArr[vertexCount].position = center + radius * float3(0.f, -1.f, 0.f);
	mesh.vtxArr[vertexCount].normal = float3(0.f, -1.f, 0.f);
	mesh.vtxArr[vertexCount].texcoord = float2(0.f, 0.f);
	++vertexCount;

	return mesh;
}