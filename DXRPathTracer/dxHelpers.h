#pragma once
#include "pch.h"
#include <d3d12.h>
#include <dxgi1_4.h>


#define SAFE_DELETE(x) if(x) delete x; x=nullptr
#define SAFE_DELETE_ARR(x) if(x) delete[] x; x=nullptr
#define SAFE_RELEASE(x) if(x) (x)->Release(); x=nullptr


extern ID3D12DebugDevice* gDebugDevice;


inline void ThrowFailedHR(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw hr;
	}
}

inline constexpr uint _bpp(DXGI_FORMAT format)	//bytes per pixel
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return 4;

	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 16;

	default:
		assert(true);
	}
}

template<typename UnsignedType, typename PowerOfTwo>
inline constexpr UnsignedType _align(UnsignedType val, PowerOfTwo base)
{
	return (val + (UnsignedType)base - 1) & ~((UnsignedType)base - 1);
}

inline constexpr uint64 _textureDataSize(DXGI_FORMAT format, uint width, uint height)
{
	const uint64 rowSize = _bpp(format) * width;
	const uint64 rowPitch = _align(rowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	const uint64 dataSize = (height - 1) * rowPitch + rowSize;
	return dataSize;
}

inline void memcpyPitch(void* dstData, uint64 dstPitch, void* srcData, uint64 srcPitch, uint64 rowSize, uint numRows)
{
	assert(dstPitch >= rowSize && srcPitch >= rowSize);

	if (dstPitch == srcPitch)
	{
		memcpy(dstData, srcData, srcPitch * numRows);
	}
	else
	{
		for (uint j = 0; j < numRows; ++j)
		{
			memcpy( (uint8*) dstData + j * dstPitch, (uint8*) srcData + j * srcPitch, rowSize);
		}
	}
}

inline void memcpyPitch(void* dstData, uint64 dstPitch, void* srcData, uint64 srcPitch, uint numRows)
{
	memcpyPitch(dstData, dstPitch, srcData, srcPitch, srcPitch, numRows);
}

inline D3D12_RESOURCE_DESC BufferDesc(uint64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Width = width;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Flags = flags;
	return desc;
}

inline D3D12_RESOURCE_DESC TextureDesc(DXGI_FORMAT format, uint width, uint height, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Format = format;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Flags = flags;
	return desc;
}

inline D3D12_HEAP_PROPERTIES HeapProperty(D3D12_HEAP_TYPE type)
{
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = type;
	return heapProp;
}

inline D3D12_RANGE Range(SIZE_T end, SIZE_T begin = 0)
{
	D3D12_RANGE range;
	range.Begin = begin;
	range.End = end;
	return range;
}

inline D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,
	D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = stateBefore;
	barrier.Transition.StateAfter = stateAfter;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return barrier;
}

inline D3D12_INPUT_ELEMENT_DESC VertexAttribute(const char* semantic, DXGI_FORMAT format, uint offset=D3D12_APPEND_ALIGNED_ELEMENT)
{
	D3D12_INPUT_ELEMENT_DESC vertexLayout = {};
	vertexLayout.SemanticName = semantic;
	vertexLayout.Format = format;
	vertexLayout.AlignedByteOffset = offset;
	vertexLayout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	return vertexLayout;
}

inline D3D12_STATIC_SAMPLER_DESC StaticSampler(uint registerBase=0, uint registerSpace=0, D3D12_SHADER_VISIBILITY visible = D3D12_SHADER_VISIBILITY_ALL)
{
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	
	sampler.MipLODBias = 0.0f;
	sampler.MaxAnisotropy = 1;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	
	sampler.ShaderRegister = registerBase;
	sampler.RegisterSpace = registerSpace;
	sampler.ShaderVisibility = visible;

	return sampler;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE operator+(const D3D12_GPU_DESCRIPTOR_HANDLE& cpuAddr, uint offset)
{
	return D3D12_GPU_DESCRIPTOR_HANDLE{ cpuAddr.ptr + offset };
}

inline D3D12_CPU_DESCRIPTOR_HANDLE operator+(const D3D12_CPU_DESCRIPTOR_HANDLE& cpuAddr, uint offset)
{
	return D3D12_CPU_DESCRIPTOR_HANDLE{ cpuAddr.ptr + offset };
}

IDXGIFactory2* getFactory();

IDXGIAdapter* getRTXAdapter();

ID3D12Device* createDX12Device(IDXGIAdapter* adapter);

D3D12_GRAPHICS_PIPELINE_STATE_DESC Graphics_Pipeline_Desc(
	D3D12_INPUT_LAYOUT_DESC inputLayout,
	ID3D12RootSignature* rootSig,
	D3D12_SHADER_BYTECODE vertexShader, 
	D3D12_SHADER_BYTECODE pixelShader,
	DXGI_FORMAT renderTargetFormat,
	uint numRenderTargets = 1);

ID3D12Resource* createCommittedBuffer(
	uint64 bufferSize, 
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD,
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE,
	D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_GENERIC_READ);


class ResourceUploader;
class DescriptorHeap;
class Descriptor;
class dxResource;

enum class DescriptorType
{
	SRV, UAV, CBV, RTV, DSV, Sampler
};


class Descriptor
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpuAddr;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr;

	friend class DescriptorHeap;
	Descriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuAddr, D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr) 
		: cpuAddr(cpuAddr), gpuAddr(gpuAddr) {}
public:
	Descriptor() = delete;
	D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle() const { return cpuAddr; }
	D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle() const { return gpuAddr; }
	
	void assignSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
	void assignUAV(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, ID3D12Resource* counter=nullptr);
	void assignRTV(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr);
	void assignCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc);

	void assignSRV(dxResource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc);
	void assignUAV(dxResource& resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, ID3D12Resource* counter=nullptr);
	void assignRTV(dxResource& resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc = nullptr);
};


class DescriptorHeap
{
	ID3D12DescriptorHeap*	heap = nullptr;
	Array<Descriptor>		descriptorArr;

public:
	~DescriptorHeap() { destroy(); }
	void destroy();

	DescriptorHeap() {}
	DescriptorHeap(uint maxDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) { 
		create(maxDescriptors, heapType); 
	}
	void create(uint maxDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	ID3D12DescriptorHeap* get() const { 
		return heap; 
	}
	Descriptor& operator[] (uint heapIdx) {
		assert(heapIdx < descriptorArr.size());
		return descriptorArr[heapIdx]; 
	}
	const Descriptor& operator[] (uint heapIdx)	const { 
		assert(heapIdx < descriptorArr.size());
		return descriptorArr[heapIdx]; 
	}
	
	void bind(ID3D12GraphicsCommandList* cmdList);
};


class RootParameter
{
protected:
	D3D12_ROOT_PARAMETER param = {};

	friend class RootSignature;
	virtual ~RootParameter() {}			// Only RootSignature can delete it.
	
public:
	virtual void bindCompute(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const = 0;
	virtual void bindGraphics(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const = 0;
};


class RootTable : public RootParameter
{
	D3D12_GPU_DESCRIPTOR_HANDLE tableStart;
	Array<D3D12_DESCRIPTOR_RANGE> rangeArr;

public:
	RootTable() = delete;
	RootTable(const char* code)										{ create(code); }
	RootTable(const char* code, D3D12_GPU_DESCRIPTOR_HANDLE handle) { create(code); setTableStart(handle); }
	void operator=(D3D12_GPU_DESCRIPTOR_HANDLE handle)				{ setTableStart(handle); }
	
	void setTableStart(D3D12_GPU_DESCRIPTOR_HANDLE handle) { 
		tableStart = handle; 
	}
	
	void create(const char* code);
	void bindCompute(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const;
	void bindGraphics(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const;
};


class RootPointer : public RootParameter
{
	D3D12_GPU_VIRTUAL_ADDRESS resourceAddress;

public:
	RootPointer() = delete;
	RootPointer(const char* code)									{ create(code); }
	RootPointer(const char* code, D3D12_GPU_VIRTUAL_ADDRESS address){ create(code); setResourceAddress(address); }
	void operator=(D3D12_GPU_VIRTUAL_ADDRESS address)				{ setResourceAddress(address); }
	
	void setResourceAddress(D3D12_GPU_VIRTUAL_ADDRESS address) { 
		resourceAddress = address; 
	}
	
	void create(const char* code);
	void bindCompute(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const;
	void bindGraphics(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const;
};


template<typename T>
class RootConstants : public RootParameter
{
	Array<uint> _32BitConsts;

public:
	RootConstants() = delete;
	RootConstants(const char* code)						{ create(code); }
	RootConstants(const char* code, const T& constants)	{ create(code); setConstants(constants); }
	void operator=(const T& constants)					{ setConstants(constants); }

	void setConstants(const T& constants) {
		memcpy(_32BitConsts.data(), &constants, sizeof(T));
	}
	
	void create(const char* code) {
		_32BitConsts.resize((sizeof(T) + 3) / 4);

		void createRootConstantsParam(D3D12_ROOT_PARAMETER& param, const char* code, uint num32BitConsts);
		createRootConstantsParam(param, code, _32BitConsts.size());
	}

	void bindCompute(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const {
		cmdList->SetComputeRoot32BitConstants(paramIdx, _32BitConsts.size(), _32BitConsts.data(), 0);
	}
	void bindGraphics(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const {
		cmdList->SetGraphicsRoot32BitConstants(paramIdx, _32BitConsts.size(), _32BitConsts.data(), 0);
	}
};


class RootSignature
{
	ID3D12RootSignature* rootSig = nullptr;
	Array<RootParameter*> pRootParamArr;

public:
	~RootSignature();
	RootSignature() {}
	ID3D12RootSignature* get() const { 
		return rootSig; 
	}
	void resize(uint size) { 
		assert(rootSig == nullptr);		// It can be called before build.
		pRootParamArr.resize(size, nullptr); 
	}
	void push_back(RootParameter* param) {
		assert(rootSig == nullptr);		// It can be called before build.
		pRootParamArr.push_back(param); 
	}
	RootParameter*& operator[](uint paramIdx) { 
		assert(paramIdx < pRootParamArr.size());
		return pRootParamArr[paramIdx]; 
	}
	RootParameter* const& operator[](uint paramIdx) const { 
		assert(paramIdx < pRootParamArr.size());
		return pRootParamArr[paramIdx]; 
	}

	void build(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	void build(const D3D12_STATIC_SAMPLER_DESC& sampler, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	void build(const Array<D3D12_STATIC_SAMPLER_DESC>& samplerArr, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);
	void bindCompute(ID3D12GraphicsCommandList* cmdList) const;
	void bindGraphics(ID3D12GraphicsCommandList* cmdList) const;
};


class dxResource
{
	const Descriptor* srv = nullptr;
	const Descriptor* uav = nullptr;
	const Descriptor* rtv = nullptr;

protected:
	ID3D12Resource* resource = nullptr;
	D3D12_RESOURCE_STATES resourceState;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
	
public:
	virtual ~dxResource() { destroy(); }
	void destroy();

	ID3D12Resource* get() const						{ return resource; }
	D3D12_GPU_VIRTUAL_ADDRESS getGpuAddress() const	{ return gpuAddress; }
	const Descriptor* getSRV() const				{ return srv; }
	const Descriptor* getUAV() const				{ return uav; }
	const Descriptor* getRTV() const				{ return rtv; }
	void setSRV(Descriptor* srv)					{ this->srv = srv; }
	void setUAV(Descriptor* uav)					{ this->uav = uav; }
	void setRTV(Descriptor* rtv)					{ this->rtv = rtv; }
	
	D3D12_RESOURCE_STATES changeResourceState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState);
};


class dxBuffer : public dxResource
{
	uint64 bufferSize = 0;

public:
	~dxBuffer() { partialDestroy(); }
	void destroy() { partialDestroy(); dxResource::destroy(); }
	void partialDestroy() { bufferSize = 0; }

	dxBuffer() {}
	dxBuffer(uint64 buffSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES rscState) {
		create(buffSize, heapType, rscFlag, rscState);
	}
	void create(uint64 bufferSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES rscState);
	
	uint64 getBufferSize() const { return bufferSize; }
};


class UnorderAccessBuffer : public dxBuffer
{
public:
	~UnorderAccessBuffer() { partialDestroy(); }
	void destroy() { partialDestroy(); dxBuffer::destroy(); }
	void partialDestroy() {}

	UnorderAccessBuffer() {}
	UnorderAccessBuffer(uint64 bufferSize, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS) { 
		create(bufferSize, state); 
	}
	void create(uint64 bufferSize, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
		dxBuffer::create(bufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, state);
	}
};


class UploadBuffer : public dxBuffer
{
	void* cpuAddress = nullptr;

public:
	~UploadBuffer() { partialDestroy(); }
	void destroy() { partialDestroy(); dxBuffer::destroy(); }
	void partialDestroy() { cpuAddress = nullptr; }

	UploadBuffer() {}
	UploadBuffer(uint64 bufferSize, void* initData = nullptr) { 
		create(bufferSize, initData); 
	}
	void create(uint64 bufferSize, void* initData = nullptr);
	
	void* map();
	void unmap();
};


class DefaultBuffer : public dxBuffer
{
public:
	~DefaultBuffer() { partialDestroy(); }
	void destroy() { partialDestroy(); dxBuffer::destroy(); }
	void partialDestroy() {}

	DefaultBuffer() {}
	DefaultBuffer(uint64 bufferSize, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON) { 
		create(bufferSize, state); 
	}
	void create(uint64 bufferSize, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON) {
		dxBuffer::create(bufferSize, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE, state);
	}
	
	void uploadData(ID3D12GraphicsCommandList* cmdList, 
		const UploadBuffer& srcBuffer, uint64 srcBufferOffset = 0) {
		uploadData(cmdList, 0, getBufferSize(), srcBuffer, srcBufferOffset);
	}
	void uploadData(ID3D12GraphicsCommandList* cmdList, uint64 copyDstOffset, uint64 copySize,
		const UploadBuffer& srcBuffer, uint64 srcBufferOffset);
};


class SelfBuffer : public DefaultBuffer
{
	UploadBuffer uploader;

public:
	~SelfBuffer() { partialDestroy(); }
	void destroy() { partialDestroy(); DefaultBuffer::destroy(); uploader.destroy(); }
	void partialDestroy() {}

	SelfBuffer() {}
	SelfBuffer(uint64 bufferSize, uint uploaderSizeFactor = 1, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON) {
		create(bufferSize, uploaderSizeFactor, state);
	}
	void create(uint64 bufferSize, uint uploaderSizeFactor = 1, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON) {
		DefaultBuffer::create(bufferSize, state);
		uploader.create(bufferSize * uploaderSizeFactor);
	}

	void* map()  { return uploader.map(); }
	void unmap() { uploader.unmap(); }
	void uploadData(ID3D12GraphicsCommandList* cmdList, uint64 uploaderOffset = 0) {
		DefaultBuffer::uploadData(cmdList, uploader, uploaderOffset);
	}
	void uploadData(ID3D12GraphicsCommandList* cmdList, uint64 copyDstOffset, uint64 copySize, uint64 uploaderOffset) {
		DefaultBuffer::uploadData(cmdList, copyDstOffset, copySize, uploader, uploaderOffset);
	}
};


class DefaultTexture : public dxResource
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	uint width = 0;
	uint height = 0;
	uint rowDataSize = 0;	// width * bytesPerPixel
	uint rowPitch = 0;		// align( rowSize, 256 )
	uint64 textureSize = 0;	// (height - 1) * rowPitch + rowSize

public:
	~DefaultTexture() { partialDestroy(); }
	void destroy() { partialDestroy(); dxResource::destroy(); }
	void partialDestroy() {
		format = DXGI_FORMAT_UNKNOWN; 
		width = height = rowDataSize = rowPitch = 0;
		textureSize = 0;
	}

	DefaultTexture() {}
	DefaultTexture(DXGI_FORMAT format, uint width, uint height, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON) {
		create(format, width, height, state);
	}
	void create(DXGI_FORMAT format, uint width, uint height, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);
	
	DXGI_FORMAT getFormat() const	{ return format; }
	uint getWidth() const			{ return width; }
	uint getHeight() const			{ return height; }
	uint getRowDataSize() const		{ return rowDataSize; }
	uint getRowPitch() const		{ return rowPitch; }
	uint64 getTextureSize() const	{ return textureSize; }

	void uploadData(ID3D12GraphicsCommandList* cmdList, 
		const UploadBuffer& srcBuffer, uint64 srcBufferOffset = 0);
};


class ReadbackBuffer
{
protected:
	ID3D12Resource* readbackBuffer = nullptr;
	void* cpuAddress = nullptr;
	uint64 maxReadbackSize;

public:
	~ReadbackBuffer() { destroy(); }
	ReadbackBuffer()  {}
	ReadbackBuffer(uint64 requiredSize) { create(requiredSize); }

	void destroy();
	void create(uint64 requiredSize);
	void* map();
	void unmap();
	void readback(ID3D12GraphicsCommandList* cmdList, dxResource& source);
};


class SwapChain {
	IDXGISwapChain3*		swapChain = nullptr;
	uint					numBuffers = 0;
	uint					currentBufferIdx;
	DXGI_FORMAT				bufferFormat;
	uint					bufferW;
	uint					bufferH;
	uint					flags;

	Array<ID3D12Resource*>	renderTargetArr;
	DescriptorHeap			rtvHeap;
	
public:
	DXGI_FORMAT getBufferFormat() const			{ return bufferFormat; }
	uint getCurrentBufferIdx() const			{ return currentBufferIdx; }
	ID3D12Resource* getRenderTatget() const		{ return renderTargetArr[currentBufferIdx]; }
	D3D12_CPU_DESCRIPTOR_HANDLE getRtv() const	{ return rtvHeap[currentBufferIdx].getCpuHandle(); }
	
	~SwapChain() { destroy(); }
	
	void present();
	void resize(uint width, uint height);
	void destroy();
	void create(
		ID3D12CommandQueue* cmdQueue, 
		HWND hwnd, 
		uint numBackBuffers,
		DXGI_FORMAT format, 
		uint width, 
		uint height, 
		bool allowTearing = false);
};


class dxShader
{
	ID3DBlob* code;

	static const LPCWSTR csoFolder;
	static const LPCWSTR hlslFolder;

public:
	~dxShader() { flush(); }
	dxShader() {}
	dxShader(LPCWSTR csoFile) { load(csoFile); }
	dxShader(LPCWSTR hlslFile, const char* entryFtn, const char* target) { load(hlslFile, entryFtn, target); }

	D3D12_SHADER_BYTECODE getCode() const {
		D3D12_SHADER_BYTECODE shadercode;
		shadercode.pShaderBytecode = code->GetBufferPointer();
		shadercode.BytecodeLength = code->GetBufferSize();
		return shadercode;
	}

	void flush();
	void load(LPCWSTR csoFile);
	void load(LPCWSTR hlslFile, const char* entryFtn, const char* target);
};


class BinaryFence
{
	ID3D12Fence*	mFence;
	HANDLE			mFenceEvent = nullptr;
	
	enum FenceValue {
		zero = 0, one = 1
	};

public:

	~BinaryFence()
	{
		if(mFenceEvent)
			CloseHandle(mFenceEvent);
		SAFE_RELEASE(mFence);
	}

	void create(ID3D12Device* device)
	{
		ThrowFailedHR(device->CreateFence(zero, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	void waitCommandQueue(ID3D12CommandQueue* cmdQueue)
	{
		ThrowFailedHR(cmdQueue->Signal(mFence, one));

		if (mFence->GetCompletedValue() != one)
		{
			ThrowFailedHR(mFence->SetEventOnCompletion(one, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}

		ThrowFailedHR(cmdQueue->Signal(mFence, zero));
	}
};


//---------------------From here, DXR related helper---------------------------------//

struct GPUMesh
{
	UINT numVertices;
	D3D12_GPU_VIRTUAL_ADDRESS vertexBufferVA;
	UINT numTridices;
	D3D12_GPU_VIRTUAL_ADDRESS tridexBufferVA;
};


using pFloat4 = float(*)[4];
struct dxTransform
{
	float mat[3][4];
	dxTransform() {}
	dxTransform(float scala) : mat{}
	{
		mat[0][0] = mat[1][1] = mat[2][2] = scala;
	}
	void operator=(const Transform& tm)
	{
		*this = *(dxTransform* ) &tm;
	}
	pFloat4 data() { return mat; }
};


void buildBLAS(ID3D12GraphicsCommandList4* cmdList,
	ID3D12Resource** blas,
	ID3D12Resource** scrach,
	const GPUMesh gpuMeshArr[],
	const D3D12_GPU_VIRTUAL_ADDRESS gpuTransformAddressArr[],
	uint numMeshes,
	uint vertexStride,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags);


void buildTLAS(ID3D12GraphicsCommandList4* cmdList,
	ID3D12Resource** tlas,
	ID3D12Resource** scrach,
	ID3D12Resource** instanceDescArr,
	ID3D12Resource* const blasArr[],
	//const ID3D12Resource* blasArr[],
	const dxTransform transformArr[],
	uint numBlas,
	uint instanceMultiplier,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags);


enum AccelerationStructureBuildMode{
	ONLY_ONE_BLAS,
	BLAS_PER_OBJECT_AND_BOTTOM_LEVEL_TRANSFORM,
	BLAS_PER_OBJECT_AND_TOP_LEVEL_TRANSFORM
};


class dxAccelerationStructure
{
	Array<ID3D12Resource*> blas;
	ID3D12Resource* tlas = nullptr;
	Array<ID3D12Resource*> scratch;
	ID3D12Resource* instances = nullptr;
	UploadBuffer transformBuff;

public:
	~dxAccelerationStructure()						{ destroy(); }
	ID3D12Resource* get() const						{ return tlas; }
	D3D12_GPU_VIRTUAL_ADDRESS getGpuAddress() const	{ return tlas->GetGPUVirtualAddress(); }
	void flush();
	void destroy();
	void build(ID3D12GraphicsCommandList4* cmdList,
		const Array<GPUMesh>& gpuMeshArr, 
		const Array<dxTransform>& transformArr, 
		//const Array<const Transform*>& pTransformArr, 
		uint vertexStride,
		uint instanceMultiplier,
		AccelerationStructureBuildMode buildMode,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags);
};


struct HitGroup
{
	LPCWSTR groupName = nullptr;
	LPCWSTR closestHit = nullptr;
	LPCWSTR anyHit = nullptr;
	HitGroup() = delete;
	HitGroup(LPCWSTR groupName, LPCWSTR cloestHit, LPCWSTR anyHit)
		: groupName(groupName), closestHit(cloestHit), anyHit(anyHit) {}
};


struct LocalRootSignature
{
	const RootSignature* rootSig = nullptr;
	Array<LPCWSTR> exportNames;
	LocalRootSignature() = delete;
	LocalRootSignature(const RootSignature* rootSig, const Array<LPCWSTR>& exportNames)
		: rootSig(rootSig), exportNames(exportNames) {}
	
	LocalRootSignature(const LocalRootSignature& lrs) 
		: rootSig(lrs.rootSig), exportNames(lrs.exportNames) {}
	LocalRootSignature(LocalRootSignature&& lrs) 
		: rootSig(lrs.rootSig), exportNames(move2(lrs.exportNames)) {}
};


struct ShaderIdentifier
{
	uint8 identifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};


class RaytracingPipeline
{
	ID3D12StateObject* rtPipeline = nullptr;
	ID3D12StateObjectProperties* sop = nullptr;
	const dxShader* dxrLib = nullptr;
	const RootSignature* globalRS = nullptr;
	Array<HitGroup> hitGroupArr;
	Array<LocalRootSignature> localRSArr;
	uint maxPayloadSize = 0;
	uint maxAttributeSize = 8;
	uint maxRayDepth = 2;
	
public:
	~RaytracingPipeline() { destroy(); }

	void destroy();
	void setDXRLib(const dxShader* DXRShader) {
		assert(rtPipeline == nullptr);
		dxrLib = DXRShader;
	}
	void setGlobalRootSignature(RootSignature* globalRs) {
		assert(rtPipeline == nullptr);
		globalRS = globalRs;
	}
	void setMaxRayDepth(uint depth) {
		assert(rtPipeline == nullptr);
		maxRayDepth = depth;
	}
	void setMaxPayloadSize(uint size) {
		assert(rtPipeline == nullptr);
		maxPayloadSize = size;
	}
	void setMaxAttributeSize(uint size) {
		assert(rtPipeline == nullptr);
		maxAttributeSize = size;
	}
	void addHitGroup(HitGroup hitGp) {
		assert(rtPipeline == nullptr);
		hitGroupArr.push_back(hitGp);
	}
	void addLocalRootSignature(const LocalRootSignature& localRs) {
		assert(rtPipeline == nullptr);
		localRSArr.push_back(localRs);
	}
	void addLocalRootSignature(LocalRootSignature&& localRs) {
		assert(rtPipeline == nullptr);
		localRSArr.push_back(move2(localRs));
	}

	void build();
	void bind(ID3D12GraphicsCommandList4* cmdList);
	ShaderIdentifier* getIdentifier(LPCWSTR exportName);
};


class ShaderTable : public SelfBuffer
{
protected:
	uint recordSize = 0;
	uint maxRecords = 0;

public:
	~ShaderTable() { partialDestroy(); }
	void destroy() { partialDestroy(); SelfBuffer::destroy(); }
	void partialDestroy() { recordSize = maxRecords = 0; }

	ShaderTable() {}
	ShaderTable(uint recordSize, uint maxRecords, uint uploaderSizeFactor = 1) {
		create(recordSize, maxRecords, uploaderSizeFactor);
	}
	void create(uint recordSize, uint maxRecords, uint uploaderSizeFactor = 1);
	
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE getTable() const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE getSubTable(uint recordOffset, uint numRecords) const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE getRecord(uint recordOffset) const;
};