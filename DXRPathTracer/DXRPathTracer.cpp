#include "pch.h"
#include "DXRPathTracer.h"
#include "Camera.h"
#include "Scene.h"


namespace DescriptorID {
	enum {
		// First RootParameter
		outUAV = 0,	
		
		// Third RootParameter
		sceneObjectBuff = 1,
		vertexBuff = 2,		
		tridexBuff = 3,
		materialBuff = 4,
		cdfBuff = 5,
		transformBuff = 6,
		
		// Not used since we use RootPointer instead of RootTable
		accelerationStructure = 10,

		maxDesciptors = 32
	};
}

namespace RootParamID {
	enum {
		tableForOutBuffer = 0,
		pointerForAccelerationStructure = 1,
		tableForGeometryInputs = 2,
		pointerForGlobalConstants = 3,
		numParams = 4
	};
}

namespace HitGroupParamID {
	enum {
		constantsForObject = 0,
		numParams = 1
	};
}

DXRPathTracer::~DXRPathTracer()
{
	SAFE_RELEASE(mCmdQueue);
	SAFE_RELEASE(mCmdList);
	SAFE_RELEASE(mCmdAllocator);
	SAFE_RELEASE(mDevice);
}

DXRPathTracer::DXRPathTracer(uint width, uint height)
	: IGRTTracer(width, height)
{
	initD3D12();
	
	mSrvUavHeap.create(DescriptorID::maxDesciptors);

	declareRootSignatures();

	buildRaytracingPipeline();

	initializeApplication();

	//mFence.waitCommandQueue(mCmdQueue);
}

void DXRPathTracer::initD3D12()
{
	mDevice = (ID3D12Device5*) createDX12Device(getRTXAdapter());

	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowFailedHR(mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mCmdQueue)));

	ThrowFailedHR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAllocator)));

	ThrowFailedHR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator, nullptr, IID_PPV_ARGS(&mCmdList)));
	
	ThrowFailedHR(mCmdList->Close());
	ThrowFailedHR(mCmdAllocator->Reset());
	ThrowFailedHR(mCmdList->Reset(mCmdAllocator, nullptr));

	mFence.create(mDevice);
}

void DXRPathTracer::declareRootSignatures()
{
	assert(mSrvUavHeap.get() != nullptr);

	// Global(usual) Root Signature
	mGlobalRS.resize(RootParamID::numParams);
	mGlobalRS[RootParamID::tableForOutBuffer] 
		= new RootTable("u0", mSrvUavHeap[DescriptorID::outUAV].getGpuHandle());
	mGlobalRS[RootParamID::pointerForAccelerationStructure] 
		= new RootPointer("(100) t0");					// It will be bound to mAccelerationStructure that is not initialized yet.
	mGlobalRS[RootParamID::tableForGeometryInputs] 
		= new RootTable("(0) t0-t5", mSrvUavHeap[DescriptorID::sceneObjectBuff].getGpuHandle());
	mGlobalRS[RootParamID::pointerForGlobalConstants] 
		= new RootPointer("b0");						// It will be bound to mGlobalConstantsBuffer that is not initialized yet.
	mGlobalRS.build();

	// Local Root Sinatures
	mHitGroupRS.resize(HitGroupParamID::numParams);
	mHitGroupRS[HitGroupParamID::constantsForObject] 
		= new RootConstants<ObjectContants>("b1");		// Every local root signature's parameter is bound to its values via shader table.
	mHitGroupRS.build(D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void DXRPathTracer::buildRaytracingPipeline()
{
	dxrLib.load(L"DXRShader.cso");

	mRtPipeline.setDXRLib(&dxrLib);
	mRtPipeline.setGlobalRootSignature(&mGlobalRS);
	mRtPipeline.addHitGroup(HitGroup(L"hitGp", L"closestHit", nullptr));
	mRtPipeline.addHitGroup(HitGroup(L"hitGpGlass", L"closestHitGlass", nullptr));
	mRtPipeline.addLocalRootSignature(LocalRootSignature(&mHitGroupRS, { L"hitGp", L"hitGpGlass" }));
	mRtPipeline.setMaxPayloadSize(sizeof(float) * 16);
	mRtPipeline.setMaxRayDepth(2);
	mRtPipeline.build();
}

void DXRPathTracer::initializeApplication()
{
	camera.setFovY(60.0f);
	camera.setScreenSize((float) tracerOutW, (float) tracerOutH);
	camera.initOrbit(float3(0.0f, 1.5f, 0.0f), 10.0f, 0.0f, 0.0f);

	mGlobalConstants.rayTmin = 0.001f;  // 1mm
	mGlobalConstants.accumulatedFrames = 0;
	mGlobalConstants.numSamplesPerFrame = 32;
	mGlobalConstants.maxPathLength = 6;
	mGlobalConstants.backgroundLight = float3(.0f);

	mGlobalConstantsBuffer.create(sizeof(GloabalContants));
	* (RootPointer*) mGlobalRS[RootParamID::pointerForGlobalConstants] 
		= mGlobalConstantsBuffer.getGpuAddress();

	uint64 maxBufferSize = _bpp(tracerOutFormat) * 1920 *1080;
	mReadBackBuffer.create(maxBufferSize);

	uint64 bufferSize = _bpp(tracerOutFormat) * tracerOutW * tracerOutH;
	mTracerOutBuffer.create(bufferSize);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = tracerOutFormat;
		uavDesc.Buffer.NumElements = tracerOutW * tracerOutH;
	}
	mSrvUavHeap[DescriptorID::outUAV].assignUAV(mTracerOutBuffer, &uavDesc);
}

void DXRPathTracer::onSizeChanged(uint width, uint height)
{
	if (width == tracerOutW && height == tracerOutH)
		return;

	width = width ? width : 1;
	height = height ? height : 1;

	tracerOutW = width;
	tracerOutH = height;

	camera.setScreenSize((float) tracerOutW, (float) tracerOutH);

	UINT64 bufferSize = _bpp(tracerOutFormat) * tracerOutW * tracerOutH;
	mTracerOutBuffer.destroy();
	mTracerOutBuffer.create(bufferSize);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	{
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = tracerOutFormat;
		uavDesc.Buffer.NumElements = tracerOutW * tracerOutH;
	}
	mSrvUavHeap[DescriptorID::outUAV].assignUAV(mTracerOutBuffer, &uavDesc);
}

void DXRPathTracer::update(const InputEngine& input)
{
	camera.update(input);

	if (camera.notifyChanged())
	{
		mGlobalConstants.cameraPos = camera.getCameraPos();
		mGlobalConstants.cameraX = camera.getCameraX();
		mGlobalConstants.cameraY = camera.getCameraY();
		mGlobalConstants.cameraZ = camera.getCameraZ();
		mGlobalConstants.cameraAspect = camera.getCameraAspect();
		mGlobalConstants.accumulatedFrames = 0;
	}
	else
		mGlobalConstants.accumulatedFrames++;

	
	* (GloabalContants*) mGlobalConstantsBuffer.map() = mGlobalConstants;
}

TracedResult DXRPathTracer::shootRays()
{
	mReadBackBuffer.unmap();
	
	mRtPipeline.bind(mCmdList);
	mSrvUavHeap.bind(mCmdList);
	mGlobalRS.bindCompute(mCmdList);

	D3D12_DISPATCH_RAYS_DESC desc = {};
	{
		desc.Width = tracerOutW;
		desc.Height = tracerOutH;
		desc.Depth = 1;
		desc.RayGenerationShaderRecord = mShaderTable.getRecord(0);
		desc.MissShaderTable = mShaderTable.getSubTable(1, 2);
		desc.HitGroupTable = mShaderTable.getSubTable(3, scene->numObjects());
	}
	mCmdList->DispatchRays(&desc);

	mReadBackBuffer.readback(mCmdList, mTracerOutBuffer);
	
	ThrowFailedHR(mCmdList->Close());
	ID3D12CommandList* cmdLists[] = { mCmdList };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);
	mFence.waitCommandQueue(mCmdQueue);
	ThrowFailedHR(mCmdAllocator->Reset());
	ThrowFailedHR(mCmdList->Reset(mCmdAllocator, nullptr));

	TracedResult result;
	result.data = mReadBackBuffer.map();
	result.width = tracerOutW;
	result.height = tracerOutH;
	result.pixelSize = _bpp(tracerOutFormat);

	return result;
}

void DXRPathTracer::setupScene(const Scene* scene)
{
	uint numObjs = scene->numObjects();

	const Array<Vertex> vtxArr = scene->getVertexArray();
	const Array<Tridex> tdxArr = scene->getTridexArray();
	const Array<Transform> trmArr = scene->getTransformArray();
	const Array<float> cdfArr = scene->getCdfArray();
	const Array<Material> mtlArr = scene->getMaterialArray();

	assert(cdfArr.size() == 0 || cdfArr.size() == tdxArr.size());

	uint64 vtxBuffSize = vtxArr.size() * sizeof(Vertex);
	uint64 tdxBuffSize = tdxArr.size() * sizeof(Tridex);
	uint64 trmBuffSize = trmArr.size() * sizeof(Transform);
	uint64 cdfBuffSize = cdfArr.size() * sizeof(float);
	uint64 mtlBuffSize = mtlArr.size() * sizeof(Material);
	uint64 objBuffSize = numObjs * sizeof(GPUSceneObject);

	UploadBuffer uploader(vtxBuffSize + tdxBuffSize + trmBuffSize + cdfBuffSize + mtlBuffSize + objBuffSize);
	uint64 uploaderOffset = 0;

	auto initBuffer = [&](DefaultBuffer& buff, uint64 buffSize, void* srcData) {
		if(buffSize == 0)
			return;
		buff.create(buffSize); 
		memcpy((uint8*) uploader.map() + uploaderOffset, srcData, buffSize);
		buff.uploadData(mCmdList, uploader, uploaderOffset);
		uploaderOffset += buffSize;
	};

	initBuffer(mVertexBuffer,	 vtxBuffSize, (void*) vtxArr.data());
	initBuffer(mTridexBuffer,	 tdxBuffSize, (void*) tdxArr.data());
	initBuffer(mTransformBuffer, trmBuffSize, (void*) trmArr.data());
	initBuffer(mCdfBuffer,		 cdfBuffSize, (void*) cdfArr.data());
	initBuffer(mMaterialBuffer,	 mtlBuffSize, (void*) mtlArr.data());

	mSceneObjectBuffer.create(objBuffSize);
	GPUSceneObject* copyDst = (GPUSceneObject*) ((uint8*) uploader.map() + uploaderOffset);
	for (uint objIdx = 0; objIdx < numObjs; ++objIdx)
	{
		const SceneObject& obj = scene->getObject(objIdx);

		GPUSceneObject gpuObj = {};
		gpuObj.vertexOffset = obj.vertexOffset;
		gpuObj.tridexOffset = obj.tridexOffset;
		gpuObj.numTridices = obj.numTridices;
		gpuObj.objectArea = obj.meshArea * obj.scale * obj.scale;
		gpuObj.twoSided = obj.twoSided;
		gpuObj.materialIdx = obj.materialIdx;
		gpuObj.backMaterialIdx = obj.backMaterialIdx;
		//gpuObj.material = obj.material;
		//gpuObj.emittance = obj.lightColor * obj.lightIntensity;
		gpuObj.modelMatrix = obj.modelMatrix;
		
		copyDst[objIdx] = gpuObj;
	}
	mSceneObjectBuffer.uploadData(mCmdList, uploader, uploaderOffset);

	ThrowFailedHR(mCmdList->Close());
	ID3D12CommandList* cmdLists[] = { mCmdList };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);
	mFence.waitCommandQueue(mCmdQueue);
	ThrowFailedHR(mCmdAllocator->Reset());
	ThrowFailedHR(mCmdList->Reset(mCmdAllocator, nullptr));

	this->scene = const_cast<Scene*>(scene);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.StructureByteStride = sizeof(GPUSceneObject);
		srvDesc.Buffer.NumElements = numObjs;
	}
	mSrvUavHeap[DescriptorID::sceneObjectBuff].assignSRV(mSceneObjectBuffer, &srvDesc);

	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
		srvDesc.Buffer.NumElements = vtxArr.size();
	}
	mSrvUavHeap[DescriptorID::vertexBuff].assignSRV(mVertexBuffer, &srvDesc);

	{
		srvDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.NumElements = tdxArr.size();
	}
	mSrvUavHeap[DescriptorID::tridexBuff].assignSRV(mTridexBuffer, &srvDesc);

	{
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.StructureByteStride = sizeof(Material);
		srvDesc.Buffer.NumElements = mtlArr.size();
	}
	mSrvUavHeap[DescriptorID::materialBuff].assignSRV(mMaterialBuffer, &srvDesc);

	setupShaderTable();

	buildAccelerationStructure();
	
	* (RootPointer*) mGlobalRS[RootParamID::pointerForAccelerationStructure]
		= mAccelerationStructure.getGpuAddress();
}

void DXRPathTracer::setupShaderTable()
{
	ShaderIdentifier* rayGenID = mRtPipeline.getIdentifier(L"rayGen");
	ShaderIdentifier* missRayID = mRtPipeline.getIdentifier(L"missRay");
	ShaderIdentifier* missShadowID = mRtPipeline.getIdentifier(L"missShadow");
	ShaderIdentifier* hitGpID = mRtPipeline.getIdentifier(L"hitGp");
	ShaderIdentifier* hitGpGlassID = mRtPipeline.getIdentifier(L"hitGpGlass");

	uint numObjs = scene->numObjects();
	mShaderTable.create(recordSize, numObjs + 3);

	HitGroupRecord* table = (HitGroupRecord*) mShaderTable.map();
	table[0].shaderIdentifier = *rayGenID;
	table[1].shaderIdentifier = *missRayID;
	table[2].shaderIdentifier = *missShadowID;

	auto& mtlArr = scene->getMaterialArray();
	for (uint i = 0; i < numObjs; ++i)
	{
		if(mtlArr[ scene->getObject(i).materialIdx ].type == Glass)
			table[3 + i].shaderIdentifier = *hitGpGlassID;
		else
			table[3 + i].shaderIdentifier = *hitGpID;

		table[3 + i].objConsts.objectIdx = i;
	}

	mShaderTable.uploadData(mCmdList);
}

void DXRPathTracer::buildAccelerationStructure()
{
	uint numObjs = scene->numObjects();
	Array<GPUMesh> gpuMeshArr(numObjs);
	Array<dxTransform> transformArr(numObjs);

	D3D12_GPU_VIRTUAL_ADDRESS vtxAddr = mVertexBuffer.getGpuAddress();
	D3D12_GPU_VIRTUAL_ADDRESS tdxAddr = mTridexBuffer.getGpuAddress();
	for (uint objIdx = 0; objIdx < numObjs; ++objIdx)
	{
		const SceneObject& obj = scene->getObject(objIdx);
		
		gpuMeshArr[objIdx].numVertices = obj.numVertices;
		gpuMeshArr[objIdx].vertexBufferVA = vtxAddr + obj.vertexOffset * sizeof(Vertex);
		gpuMeshArr[objIdx].numTridices = obj.numTridices;
		gpuMeshArr[objIdx].tridexBufferVA = tdxAddr + obj.tridexOffset * sizeof(Tridex);
	
		transformArr[objIdx] = obj.modelMatrix;
		//transformArr[objIdx] = obj.modelMatrix;
	}
	
	mAccelerationStructure.build(mCmdList, gpuMeshArr, transformArr, 
		sizeof(Vertex), 1, buildMode, buildFlags);

	ThrowFailedHR(mCmdList->Close());
	ID3D12CommandList* cmdLists[] = { mCmdList };
	mCmdQueue->ExecuteCommandLists(1, cmdLists);
	mFence.waitCommandQueue(mCmdQueue);
	ThrowFailedHR(mCmdAllocator->Reset());
	ThrowFailedHR(mCmdList->Reset(mCmdAllocator, nullptr));

	//mAccelerationStructure.flush();
}

/*
void DXRPathTracer::buildAccelerationStructure()
{
	uint numPrims = scene.numPrimitives;
	
	UINT64 vBufferSize = 0;
	UINT64 iBufferSize = 0;
	uint* vOffset = new uint [numPrims];
	uint* iOffset = new uint [numPrims];

	for (uint i = 0; i < numPrims; ++i)
	{
		const Mesh& mesh = scene.primitives[i].mesh;
		vOffset[i] = (uint) vBufferSize;
		vBufferSize += mesh.numVertices * sizeof(Vertex);
		iOffset[i] = (uint) vBufferSize;
		iBufferSize += mesh.numTriangles * sizeof(uint3);
	}

	ID3D12Resource* vBuffer = createCommittedBuffer(vBufferSize);
	ID3D12Resource* iBuffer = createCommittedBuffer(iBufferSize);
	uint8* vAddr; vBuffer->Map(0, &Range(0), (void**)&vAddr);
	uint8* iAddr; iBuffer->Map(0, &Range(0), (void**)&iAddr);
	for (uint i = 0; i < numPrims; ++i)
	{
		const Mesh& mesh = scene.primitives[i].mesh;
		memcpy(vAddr + vOffset[i], mesh.attributes, mesh.numVertices * sizeof(Vertex));
		memcpy(iAddr + iOffset[i], mesh.indices, mesh.numTriangles * sizeof(uint3));
	}
	vBuffer->Unmap(0, nullptr);
	iBuffer->Unmap(0, nullptr);

	GPUMesh* gpuMeshArr = new GPUMesh[numPrims];
	for (uint i = 0; i < numPrims; ++i)
	{
		gpuMeshArr[i].numVertices = scene.primitives[i].mesh.numVertices;
		gpuMeshArr[i].vertexBufferVA = vBuffer->GetGPUVirtualAddress() + vOffset[i];
		gpuMeshArr[i].numTriangles = scene.primitives[i].mesh.numTriangles;
		gpuMeshArr[i].indexBufferVA = iBuffer->GetGPUVirtualAddress() + iOffset[i];
	}
	
	Transform* cpuTransArr = new Transform[numPrims];
	for (uint i = 0; i < numPrims; ++i)
	{
		cpuTransArr[i] = Transform(scene.primitives[i].scale);
		cpuTransArr[i].mat[0][3] = scene.primitives[i].translation.x;
		cpuTransArr[i].mat[1][3] = scene.primitives[i].translation.y;
		cpuTransArr[i].mat[2][3] = scene.primitives[i].translation.z;
	}

	ID3D12Resource* gpuTransArr = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS* gpuTransAddrArr = new D3D12_GPU_VIRTUAL_ADDRESS[numPrims];

	if (buildMode != BLAS_PER_MESH_TOP_TRANS)
	{
		uint gpuTransSize = (uint) _align(sizeof(Transform), D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT);
		gpuTransArr = createCommittedBuffer(mDevice.Get(), gpuTransSize * numPrims);
			
		uint8* pGpuTrans; gpuTransArr->Map(0, &Range(0), (void**)&pGpuTrans);
		Transform identity(1.0f);

		for (uint i = 0; i < numPrims; ++i)
		{
			memcpy(pGpuTrans + i * gpuTransSize, &cpuTransArr[i], sizeof(Transform));
			cpuTransArr[i] = identity;
		}

		gpuTransArr->Unmap(0, nullptr);
		
		for (uint i = 0; i < numPrims; ++i)
		{
			gpuTransAddrArr[i] = gpuTransArr->GetGPUVirtualAddress() + i * gpuTransSize;
			
			// Below is not necessary, but expect a few time decrement in BLAS build
			if (scene.primitives[i].scale == 1.f ||
				scene.primitives[i].translation.x == 0.f ||
				scene.primitives[i].translation.y == 0.f ||
				scene.primitives[i].translation.z == 0.f)
			{
				gpuTransAddrArr[i] = 0;
			}
		}
	}
	else
	{
		for (uint i = 0; i < numPrims; ++i)
		{
			gpuTransAddrArr[i] = 0;
		}
	}
	
	ID3D12Resource *instanceDescArr, *tScrach, *tlas;
	ID3D12Resource **bScrachArr, **blasArr;
	uint numPrimsPerBlas;

	numPrimsPerBlas = (buildMode == ONLY_ONE_BLAS) ? numPrims : 1;
	numBottomLevels = (buildMode == ONLY_ONE_BLAS) ? 1 : numPrims;

	bScrachArr = new ID3D12Resource* [numBottomLevels];
	blasArr = new ID3D12Resource* [numBottomLevels];

	mCmdList->Reset(mCmdAllocator.Get(), nullptr);

	for (uint i = 0; i < numBottomLevels; ++i)
	{
		buildBLAS(mCmdList.Get(), numPrimsPerBlas, sizeof(Vertex), gpuMeshArr + i, gpuTransAddrArr + i, &bScrachArr[i], &blasArr[i], buildFlags);
	}
	buildTLAS(mCmdList.Get(), numBottomLevels, blasArr, cpuTransArr, &instanceDescArr, &tScrach, &tlas, buildFlags);

	ThrowFailedHR(mCmdList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCmdList.Get() };
	mCmdQueue->ExecuteCommandLists(1, ppCommandLists);
	mFence.waitCommandQueue(mCmdQueue.Get());

	// free and release blas related (except blas itself)
	for (uint i = 0; i < numBottomLevels; ++i)
	{
		bottomLevelAS[i].Attach(blasArr[i]);
		bScrachArr[i]->Release();
	}
	delete[] bScrachArr;
	delete[] blasArr;

	// free and release tlas related (except tlas itself)
	topLevelAS.Attach(tlas);
	tScrach->Release();
	instanceDescArr->Release();

	// free and release transform related
	gpuTransArr->Release();
	delete[] gpuTransAddrArr;
	delete[] cpuTransArr;
	
	// free and release geometry related
	vBuffer->Release();
	iBuffer->Release();
	delete[] vOffset;
	delete[] iOffset;
	delete[] gpuMeshArr;
}
*/