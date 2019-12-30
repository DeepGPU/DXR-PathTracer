#pragma once
#include "Mesh.h"
#include "Material.h"


struct GPUSceneObject
{
	uint vertexOffset;
	uint tridexOffset;
	uint numTridices;

	float objectArea;	// objectArea = meshArea * objectScale * objectScale
	
	uint twoSided;
	uint materialIdx;
	uint backMaterialIdx;
	//Material material;
	//float3 emittance;	// emittance = lightColor * lightIntensity

	Transform modelMatrix;
};


struct SceneObject
{	
	uint vertexOffset;
	uint tridexOffset;	
	uint numVertices;
	uint numTridices;

	float meshArea			= 0.0f;

	uint twoSided			= 0;
	uint materialIdx		= uint(-1);	
	uint backMaterialIdx	= uint(-1);	
	//Material material;		// Make sense only when materialIdx == uint(-1).
	//float3 lightColor		= float3(1.0f);
	//float lightIntensity	= 0.0f;

	float3 translation		= float3(0.0f);
	float4 rotation			= float4(0.f, 0.f, 0.f, 1.f);
	float scale				= 1.0f;	// Do not support anisotropic scale for several reasons (especially, cdf calculation).
	Transform modelMatrix	= Transform::identity();
	//uint transformIdx		= 0;	// zero transformIdx means identity matrix.
};


class Scene
{
	Array<SceneObject>	objArr;
	Array<Vertex>		vtxArr;
	Array<Tridex>		tdxArr;
	Array<float>		cdfArr;
	Array<Transform>	trmArr;
	Array<Material>		mtlArr;
	
	friend class SceneLoader;

public:
	void clear() {
		objArr.clear();
		vtxArr.clear();
		tdxArr.clear();
		cdfArr.clear();
		trmArr.clear();
		mtlArr.clear();
	}
	const Array<Vertex>& getVertexArray() const			{ return vtxArr; }
	const Array<Tridex>& getTridexArray() const			{ return tdxArr; }
	const Array<float >& getCdfArray() const			{ return cdfArr; }
	const Array<Transform>& getTransformArray() const	{ return trmArr; }
	const Array<Material>& getMaterialArray() const		{ return mtlArr; }
	const SceneObject& getObject(uint i) const			{ return objArr[i]; }
	uint numObjects() const								{ return objArr.size(); }
};
