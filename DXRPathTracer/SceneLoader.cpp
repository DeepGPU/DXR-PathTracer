#include "pch.h"
#include "SceneLoader.h"
#include "generateMesh.h"
#include "loadMesh.h"


void SceneLoader::initializeGeometryFromMeshes(Scene* scene, const Array<Mesh*>& meshes)
{
	scene->clear();

	uint numObjs = meshes.size();
	Array<Vertex>& vtxArr = scene->vtxArr;
	Array<Tridex>& tdxArr = scene->tdxArr;
	Array<SceneObject>& objArr = scene->objArr;

	objArr.resize(numObjs);

	uint totVertices = 0;
	uint totTridices = 0;
	
	for (uint i = 0; i < numObjs; ++i)
	{
		uint nowVertices = meshes[i]->vtxArr.size();
		uint nowTridices = meshes[i]->tdxArr.size();

		objArr[i].vertexOffset = totVertices;
		objArr[i].tridexOffset = totTridices;
		objArr[i].numVertices = nowVertices;
		objArr[i].numTridices = nowTridices;

		totVertices += nowVertices;
		totTridices += nowTridices;
	}

	vtxArr.resize(totVertices);
	tdxArr.resize(totTridices);

	for (uint i = 0; i < numObjs; ++i)
	{
		memcpy(&vtxArr[objArr[i].vertexOffset], &meshes[i]->vtxArr[0], sizeof(Vertex) * objArr[i].numVertices);
		memcpy(&tdxArr[objArr[i].tridexOffset], &meshes[i]->tdxArr[0], sizeof(Tridex) * objArr[i].numTridices);
	}
}

void SceneLoader::computeModelMatrices(Scene* scene)
{
	for (auto& obj : scene->objArr)
	{
		obj.modelMatrix = composeMatrix(obj.translation, obj.rotation, obj.scale);
	}
}

Scene* SceneLoader::push_testScene1()
{
	Scene* scene = new Scene;
	sceneArr.push_back(scene);

	Mesh ground = generateRectangleMesh(float3(0.0f), float3(20.f, 0.f, 20.f), FaceDir::up);
	Mesh box = generateCubeMesh(float3(0.0f, 1.0f, 0.0f), float3(5.f, 2.f, 3.f)); 
	Mesh quadLight = generateRectangleMesh(float3(0.0f), float3(3.0f, 0.f, 3.0f), FaceDir::down);
	
	initializeGeometryFromMeshes(scene, { &ground, &box, &quadLight });

	Array<Material>& mtlArr = scene->mtlArr;
	mtlArr.resize(3);

	mtlArr[0].albedo = float3(0.7f, 0.3f, 0.4f);
	
	mtlArr[1].type = Plastic;
	mtlArr[1].albedo = float3(0.1f);
	mtlArr[1].reflectivity = 0.01f;
	mtlArr[1].roughness = 0.2f;
	
	mtlArr[2].emittance = float3(200.0f);

	//scene->objArr[0].twoSided = true;
	scene->objArr[0].materialIdx = 0;
	scene->objArr[0].backMaterialIdx = 0;
	scene->objArr[1].materialIdx = 1;
	scene->objArr[2].materialIdx = 2;

	scene->objArr[0].translation = float3(0.0f);
	scene->objArr[1].translation = float3(0.0f, 0.5f, 0.0f);
	scene->objArr[2].translation = float3(-20.0f, 17.f, 0.0f);
	
	computeModelMatrices(scene);

	return scene;
}


Scene* SceneLoader::push_hyperionTestScene()
{
	Scene* scene = new Scene;
	sceneArr.push_back(scene);

	Mesh groundM	= generateRectangleMesh(float3(0.0, -0.4, 0.0), float3(40.0, 0.0, 40.0), FaceDir::up);
	Mesh tableM		= generateBoxMesh(float3(-5.0, -0.38, -4.0), float3(5.0, -0.01, 3.0));
	Mesh sphereM	= generateSphereMesh(float3(0,1,0), 1.0f);
	Mesh ringM		= loadMeshFromOBJFile("../data/mesh/ring.obj", true);
	Mesh golfBallM	= loadMeshFromOBJFile("../data/mesh/golfball.obj", true);
	Mesh puzzleM	= loadMeshFromOBJFile("../data/mesh/burrPuzzle.obj", true);
	initializeGeometryFromMeshes(scene, { &groundM, &tableM, &sphereM, &ringM, &golfBallM, &puzzleM });

	enum SceneObjectId {
		ground, table, light, glass, metal, pingpong, bouncy, orange, wood, golfball, marble1, 
		marble2, ring1, ring2, ring3, numObjs
	};
	enum MaterialId {
		groundMtl, tableMtl, lightMtl, glassMtl, metalMtl, pingpongMtl, bouncyMtl, orangeMtl, woodMtl, golfballMtl, marble1Mtl, 
		marble2Mtl, ringMtl, numMtls
	};

	Array<SceneObject> objArr(numObjs, scene->objArr[2]);
	objArr[ground  ] = scene->objArr[0];
	objArr[table   ] = scene->objArr[1];
	objArr[ring1   ] = scene->objArr[3];
	objArr[ring2   ] = scene->objArr[3];
	objArr[ring3   ] = scene->objArr[3];
	objArr[golfball] = scene->objArr[4];
	objArr[wood    ] = scene->objArr[5];

	objArr[light   ].scale = 2.0f;
	objArr[light   ].translation = float3(-20, 17, 0);

	objArr[ground  ].translation = float3(0.0, -0.04, 0.0);
	objArr[table   ].translation = float3(0.0, -0.02, 0.0);
	objArr[glass   ].translation = float3(3.5, 0.0, 0.0);
	objArr[metal   ].translation = float3(-3.5, 0.0, 0.0);
	objArr[pingpong].translation = float3(-1.5, 0.0, 1.1);
	objArr[bouncy  ].translation = float3(-2.0, 0.0, -1.1);
	objArr[orange  ].translation = float3(2.0, 0.0, -1.1);
	objArr[marble1 ].translation = float3(-0.5, 0.0, 2.0);
	objArr[marble2 ].translation = float3(0.5, 0.0, 2.0);
	objArr[ring1   ].translation = float3(0.0, -0.02, 0.0);
	objArr[ring2   ].translation = float3(0.6, -0.02, 0.3);
	objArr[ring3   ].translation = float3(-1.3, -0.02, -0.3);

	objArr[ground  ].scale = 1.0f;
	objArr[table   ].scale = 1.0f;
	objArr[glass   ].scale = 0.55f;
	objArr[metal   ].scale = 0.6f;
	objArr[pingpong].scale = 0.45f;
	objArr[bouncy  ].scale = 0.25f;
	objArr[orange  ].scale = 0.5f;
	objArr[marble1 ].scale = 0.1f;
	objArr[marble2 ].scale = 0.15f;
	objArr[ring1   ].scale = 0.005f;
	objArr[ring2   ].scale = 0.005f;
	objArr[ring3   ].scale = 0.005f;

	objArr[golfball].translation = float3(-12.3, -13.1, -140.0);
	objArr[golfball].scale = 0.25f;
	
	// burrPuzzle.obj
	objArr[wood    ].translation = float3(-0.2, 1.0, -2.3);
	objArr[wood    ].rotation = getRotationAsQuternion({0,1,0}, 30.0f);
	objArr[wood    ].scale = 20.0f;

	objArr.swap(scene->objArr);


	Array<Material>& mtlArr = scene->mtlArr;
	mtlArr.resize(numMtls);
	mtlArr[groundMtl  ].albedo = float3(0.75, 0.6585, 0.5582);
	mtlArr[tableMtl   ].albedo = float3(0.87, 0.7785, 0.6782);
	mtlArr[lightMtl   ].albedo = float3(0);
	mtlArr[glassMtl   ].albedo = float3(0);
	mtlArr[metalMtl   ].albedo = float3(0.3);
	//mtlArr[pingpongMtl].albedo = float3(0.93, 0.89, 0.85);
	mtlArr[pingpongMtl].albedo = float3(0.4, 0.2, 0.2);
	mtlArr[bouncyMtl  ].albedo = float3(0.9828262, 0.180144, 0.0780565);
	mtlArr[orangeMtl  ].albedo = float3(0.7175, 0.17, 0.005);
	mtlArr[woodMtl    ].albedo = float3(0.3992, 0.21951971, 0.10871);
	mtlArr[golfballMtl].albedo = float3(0.9, 0.87, 0.95);
	mtlArr[marble1Mtl ].albedo = float3(0.276, 0.344, 0.2233);
	mtlArr[marble2Mtl ].albedo = float3(0.2549, 0.3537, 0.11926);
	mtlArr[ringMtl    ].albedo = float3(0.95, 0.93, 0.88);

	mtlArr[lightMtl   ].emittance = float3(200.0f);
	mtlArr[bouncyMtl  ].emittance = 10.0f * float3(0.9828262, 0.180144, 0.0780565);
	mtlArr[marble1Mtl ].emittance = 10.0f * float3(0.276, 0.344, 0.2233);
	mtlArr[marble2Mtl ].emittance = 10.0f * float3(0.2549, 0.3537, 0.11926);
	//mtlArr[glassMtl	  ].emittance = float3(0.01f);

	mtlArr[pingpongMtl].type = Metal;
	mtlArr[pingpongMtl].roughness = 0.2f;

	mtlArr[metalMtl   ].type = Metal;
	mtlArr[metalMtl   ].roughness = 0.003f;
	mtlArr[ringMtl    ].type = Metal;
	mtlArr[ringMtl    ].roughness = 0.02f;

	mtlArr[orangeMtl  ].type = Plastic;
	mtlArr[orangeMtl  ].roughness = 0.01f;
	mtlArr[orangeMtl  ].reflectivity = 0.1f;

	mtlArr[woodMtl    ].type = Plastic;
	mtlArr[woodMtl    ].roughness = 0.3f;
	mtlArr[woodMtl    ].reflectivity = 0.1f;

	mtlArr[golfballMtl].type = Plastic;
	mtlArr[golfballMtl].reflectivity = 0.1f;
	mtlArr[golfballMtl].roughness = 0.05f;

	mtlArr[glassMtl	  ].type = Glass;
	mtlArr[glassMtl	  ].transmittivity = 0.96f;


	for(uint i=0; i<ringMtl; ++i)
		scene->objArr[i].materialIdx = i;
	scene->objArr[ring1].materialIdx = ringMtl;
	scene->objArr[ring2].materialIdx = ringMtl;
	scene->objArr[ring3].materialIdx = ringMtl;

	computeModelMatrices(scene);

	return scene;
}