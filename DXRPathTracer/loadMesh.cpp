#include "pch.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "Mesh.h"
#include <map>

class compTynyIdx
{
public:
	bool operator() (const tinyobj::index_t& a, const tinyobj::index_t& b) 
	{
		return
			(a.vertex_index != b.vertex_index) ? (a.vertex_index < b.vertex_index) :
				(a.normal_index != b.normal_index) ? (a.normal_index < b.normal_index) : (a.texcoord_index < b.texcoord_index);
	}

};

Mesh loadMeshFromOBJFile(const char* filename, bool optimizeVertexxCount)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename);
	
	if (!err.empty())
	{
		throw Error(err.c_str());
	}

	if (shapes.size() != 1)
	{
		throw Error("The obj file includes several meshes.\n");
	}

	std::vector<tinyobj::index_t>& I = shapes[0].mesh.indices;
	uint numTri = (uint) shapes[0].mesh.num_face_vertices.size();
	
	if (I.size() != 3 * numTri)
	{
		throw Error("The mesh includes non-triangle faces.\n");
	}
	
	Mesh mesh;
	mesh.vtxArr.resize(numTri * 3);
	mesh.tdxArr.resize(numTri);
	
	std::map<tinyobj::index_t, uint, compTynyIdx> tinyIdxToVtxIdx;
	uint numVtx = 0;

	if (optimizeVertexxCount)
	{
		for (uint i = 0; i < 3 * numTri; ++i)
		{
			tinyobj::index_t& tinyIdx = I[i];
			auto iterAndBool = tinyIdxToVtxIdx.insert({ tinyIdx, numVtx });

			if (iterAndBool.second)
			{
				mesh.vtxArr[numVtx].position = *((float3*)&attrib.vertices[tinyIdx.vertex_index * 3]);
				mesh.vtxArr[numVtx].normal = *((float3*)&attrib.normals[tinyIdx.normal_index * 3]);
				mesh.vtxArr[numVtx].texcoord = (tinyIdx.texcoord_index == -1) ? float2(0, 0) :
					*((float2*)&attrib.texcoords[tinyIdx.texcoord_index * 2]);
				numVtx++;
			}

			((uint*)mesh.tdxArr.data())[i] = iterAndBool.first->second;
		}
		mesh.vtxArr.resize(numVtx);
	}
	
	else
	{
		for (uint i = 0; i < 3 * numTri; ++i)
		{
			tinyobj::index_t& idx = I[i];
			mesh.vtxArr[i].position = *((float3*) &attrib.vertices[idx.vertex_index*3]);
			mesh.vtxArr[i].normal = *((float3*) &attrib.normals[idx.normal_index*3]);
			mesh.vtxArr[i].texcoord = (idx.texcoord_index == -1) ? 
				float2(0, 0) : *((float2*) &attrib.texcoords[idx.texcoord_index*2]) ;

			((uint*) mesh.tdxArr.data()) [i] = i;
		}
	}

	return mesh;
}