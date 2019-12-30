#pragma once
#include "IGRTTracer.h"
#include "dxHelpers.h"
#include "Camera.h"
#define NextAlignedLine __declspec(align(16))


// This structure will be bind to dxr shader directly via [global] root signature. 
struct GloabalContants	
{	
NextAlignedLine
	float3 backgroundLight;
NextAlignedLine	
	float3 cameraPos;
NextAlignedLine	
	float3 cameraX;
NextAlignedLine	
	float3 cameraY;
NextAlignedLine	
	float3 cameraZ;
NextAlignedLine	
	float2 cameraAspect;
	float rayTmin = 1e-4f;
	float rayTmax = 1e27f;
NextAlignedLine
	uint accumulatedFrames;
	uint numSamplesPerFrame;
	uint maxPathLength;
};


// This structure will be bind to dxr shader directly via [local] root signature.
struct ObjectContants
{
	uint objectIdx;
};

static const uint xxx = sizeof(GloabalContants);
static const uint identifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
static const uint hitGroupRecordSize = _align(
	identifierSize + (uint) sizeof(ObjectContants), D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);


union HitGroupRecord
{
	struct {
		ShaderIdentifier shaderIdentifier;
		ObjectContants objConsts;
	};
private:
	char pad[hitGroupRecordSize];
};


class Scene;
class InputEngine;
class DXRPathTracer : public IGRTTracer
{
	static const DXGI_FORMAT			tracerOutFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	static const uint					recordSize = hitGroupRecordSize;

	ID3D12Device5*						mDevice;
	ID3D12CommandQueue*					mCmdQueue;
	ID3D12GraphicsCommandList4*			mCmdList;
	ID3D12CommandAllocator*				mCmdAllocator;
	BinaryFence							mFence;
	void initD3D12();
	
	DescriptorHeap						mSrvUavHeap;

	RootSignature						mGlobalRS;
	RootSignature						mHitGroupRS;
	void declareRootSignatures();

	dxShader							dxrLib;
	RaytracingPipeline					mRtPipeline;
	void buildRaytracingPipeline();
	
	GloabalContants						mGlobalConstants;
	UploadBuffer						mGlobalConstantsBuffer;
	UnorderAccessBuffer					mTracerOutBuffer;
	ReadbackBuffer						mReadBackBuffer;
	void initializeApplication();
//------Until here, scene independent members-------------------------//

	OrbitCamera camera;

//------From now, scene dependent members-----------------------------//
	DefaultBuffer						mSceneObjectBuffer;
	DefaultBuffer						mVertexBuffer;
	DefaultBuffer						mTridexBuffer;
	DefaultBuffer						mCdfBuffer;			// Now not use.
	DefaultBuffer						mTransformBuffer;	// Now not use.
	DefaultBuffer						mMaterialBuffer;	// Now not use.
	
	ShaderTable							mShaderTable;
	void setupShaderTable();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = 
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		//D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	AccelerationStructureBuildMode		buildMode = 
		//ONLY_ONE_BLAS;
		//BLAS_PER_OBJECT_AND_BOTTOM_LEVEL_TRANSFORM;
		BLAS_PER_OBJECT_AND_TOP_LEVEL_TRANSFORM;
	dxAccelerationStructure				mAccelerationStructure;
	void buildAccelerationStructure();
	//void buildAccelerationStructure1();

public:
	~DXRPathTracer();
	DXRPathTracer(uint width, uint height);
	virtual void onSizeChanged(uint width, uint height);
	virtual void update(const InputEngine& input);
	virtual TracedResult shootRays();
	virtual void setupScene(const Scene* scene);
};
