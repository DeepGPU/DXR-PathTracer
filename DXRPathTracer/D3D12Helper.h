#pragma once
#include "pch.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl.h>
using Microsoft::WRL::ComPtr;


inline void ThrowFailedHR(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw hr;
	}
}

inline constexpr UINT _bpp(DXGI_FORMAT format)	//bytes per pixel
{
	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return 4;

	default:
		throw Error();
	}
}

template<typename UnsignedType, typename PowerOfTwo>
inline constexpr UnsignedType _align(UnsignedType val, PowerOfTwo base)
{
	return (val + base - 1) & ~(base - 1);
}

inline constexpr UINT64 _textureDataSize(DXGI_FORMAT format, UINT width, UINT height)
{
	const UINT64 rowSize = _bpp(format) * width;
	const UINT64 rowPitch = _align(rowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	const UINT64 dataSize = (height - 1) * rowPitch + rowSize;
	return dataSize;
}

inline IDXGIFactory2* getFactory()
{
	static ComPtr<IDXGIFactory2> factory;

	if (!factory)
	{
#ifdef _DEBUG
		ComPtr<ID3D12Debug> debuger;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debuger))))
		{
			debuger->EnableDebugLayer();
		}
#endif
		ThrowFailedHR(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
	}

	return factory.Get();
}

inline IDXGIAdapter* getRTXAdapter()
{
	static ComPtr<IDXGIAdapter> adapter;

	if (!adapter)
	{
		for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != getFactory()->EnumAdapters(i, &adapter); i++)
		{
			ComPtr<ID3D12Device> tempDevice;
			ThrowFailedHR(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&tempDevice)));

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
			HRESULT hr = tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

			if (FAILED(hr))
			{
				throw Error("Your Window 10 version must be at least 1809 to support D3D12_FEATURE_D3D12_OPTIONS5.");
			}

			if (features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
			{
				return adapter.Get();
			}
		}

		throw Error("There are not DXR supported adapters(video cards such as Nvidia's Volta or Turing RTX). Also, The DXR fallback layer is not supported in this application.");
	}

	return adapter.Get();
}

inline IDXGIAdapter* getNonIntelAdapter()
{
	static ComPtr<IDXGIAdapter> adapter;

	if (!adapter)
	{
		for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != getFactory()->EnumAdapters(i, &adapter); i++)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			if (wcsncmp(desc.Description, L"Intel", 5) != 0)
				break;
		}
	}

	return adapter.Get();
}

inline D3D12_RESOURCE_DESC Resource_Desc_Buffer(UINT64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
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

inline D3D12_RESOURCE_DESC Resource_Desc_Texture(DXGI_FORMAT format, UINT width, UINT height, D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE)
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
	desc.Flags = flags;
	return desc;
}

inline D3D12_HEAP_PROPERTIES Heap_Property(D3D12_HEAP_TYPE type)
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

inline D3D12_RESOURCE_BARRIER Barrier_Transition(ID3D12Resource* resource,
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

inline D3D12_INPUT_ELEMENT_DESC Vertex_Attribute(const char* semantic, DXGI_FORMAT format, UINT offset=D3D12_APPEND_ALIGNED_ELEMENT)
{
	D3D12_INPUT_ELEMENT_DESC vertexLayout = {};
	vertexLayout.SemanticName = semantic;
	vertexLayout.Format = format;
	vertexLayout.AlignedByteOffset = offset;
	vertexLayout.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	return vertexLayout;
}

inline D3D12_GRAPHICS_PIPELINE_STATE_DESC Graphics_Pipeline_Desc(
	D3D12_INPUT_LAYOUT_DESC inputLayout,
	ID3D12RootSignature* rootSig,
	D3D12_SHADER_BYTECODE vertexShader, 
	D3D12_SHADER_BYTECODE pixelShader,
	DXGI_FORMAT renderTargetFormat,
	UINT numRenderTargets = 1)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};

	pipelineDesc.InputLayout = inputLayout;
	pipelineDesc.pRootSignature = rootSig;
	pipelineDesc.VS = vertexShader;
	pipelineDesc.PS = pixelShader;
	pipelineDesc.SampleMask = UINT_MAX;
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.SampleDesc.Count = 1;
	pipelineDesc.NumRenderTargets = numRenderTargets;
	for(UINT i=0; i<numRenderTargets; ++i)
		pipelineDesc.RTVFormats[i] = renderTargetFormat;

	D3D12_RASTERIZER_DESC rasDesc = {};
	{
		rasDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasDesc.DepthClipEnable = TRUE;
	}
	pipelineDesc.RasterizerState = rasDesc;

	D3D12_BLEND_DESC blendDesc = {};
	{
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			FALSE,FALSE,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}
	pipelineDesc.BlendState = blendDesc;

	return pipelineDesc;
}

inline ID3D12Resource* createCommittedBuffer(ID3D12Device* device, UINT64 bufferSize, 
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_UPLOAD,
	D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_GENERIC_READ,
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE)
{
	ID3D12Resource* resource;
	ThrowFailedHR(device->CreateCommittedResource(
		&Heap_Property(heapType), D3D12_HEAP_FLAG_NONE,
		&Resource_Desc_Buffer(bufferSize, resourceFlags), resourceStates,
		nullptr, IID_PPV_ARGS(&resource)));
	return resource;
}

inline ID3D12Resource* createCommittedBuffer(ID3D12Device* device, UINT64 bufferSize, void* data)
{
	ID3D12Resource* resource = createCommittedBuffer(device, bufferSize);
	void* mappedAddress;
	resource->Map(0, &Range(0), &mappedAddress);
    memcpy(mappedAddress, data, bufferSize);
	resource->Unmap(0, nullptr);
	return resource;
}

enum Tearing
{
	no = 0, yes = 1
};

class SwapChain {
	ComPtr<IDXGISwapChain3>			mSwapChain;
	UINT							mNumBuffers;
	UINT							mCurrentBufferIndex;
	DXGI_FORMAT						mFormat;
	UINT							mWidth;
	UINT							mHeight;
	UINT							mFlags;
	
	ComPtr<ID3D12DescriptorHeap>	mRtvHeap;
	ID3D12Resource**				mRenderTarget = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE*	mRtvHandle = nullptr;
	
public:
	~SwapChain()
	{
		if (mRtvHandle)		
			delete[] mRtvHandle;
		if (mRenderTarget)
		{
			for (UINT i = 0; i < mNumBuffers; ++i)
				delete mRenderTarget[i];
			delete[] mRenderTarget;
		}
	}

	DXGI_FORMAT bufferFormat()
	{
		return mFormat;
	}
	
	UINT currentBufferIndex()
	{
		return mCurrentBufferIndex;
	}

	ID3D12Resource* getRenderTatget()
	{
		return mRenderTarget[mCurrentBufferIndex];
	}

	D3D12_CPU_DESCRIPTOR_HANDLE getRtv()
	{
		return mRtvHandle[mCurrentBufferIndex];
	}

	void create(IDXGIFactory2* factory, ID3D12CommandQueue* cmdQueue, HWND hwnd, UINT numBackBuffers,
		DXGI_FORMAT format, UINT width, UINT height, Tearing mode = Tearing::no)
	{
		mNumBuffers = numBackBuffers;
		mWidth = width;
		mHeight = height;
		mRenderTarget = new ID3D12Resource* [numBackBuffers];
		mRtvHandle = new D3D12_CPU_DESCRIPTOR_HANDLE[numBackBuffers];
		mFormat = format;
		mFlags = (mode == Tearing::yes) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = mNumBuffers;
		swapChainDesc.Width = mWidth;
		swapChainDesc.Height = mHeight;
		swapChainDesc.Format = mFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Flags = mFlags;
		swapChainDesc.SampleDesc.Count = 1;

		ThrowFailedHR(factory->CreateSwapChainForHwnd(
			cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr,
			(IDXGISwapChain1**)(IDXGISwapChain3**)&mSwapChain));

		ID3D12Device* device;
		cmdQueue->GetDevice(IID_PPV_ARGS(&device));

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = mNumBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		ThrowFailedHR(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

		for (UINT i = 0; i < mNumBuffers; ++i)
		{
			ThrowFailedHR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTarget[i])));

			mRtvHandle[i].ptr = mRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr
				+ i * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			device->CreateRenderTargetView(mRenderTarget[i], nullptr, mRtvHandle[i]);
		}

		mCurrentBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	}

	void resize(UINT width, UINT height)
	{
		if (mWidth == width && mHeight == height)
			return;

		ID3D12Device* device;
		mSwapChain->GetDevice(IID_PPV_ARGS(&device));

		/*ID3D12DebugDevice* debug;
		device->QueryInterface(IID_PPV_ARGS(&debug));
		debug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);*/

		for (UINT i = 0; i < mNumBuffers; ++i)
		{
			mRenderTarget[i]->Release();
		}

		/*debug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		debug->Release();*/

		//ThrowFailedHR(mSwapChain->Present(1, 0));
		ThrowFailedHR(mSwapChain->ResizeBuffers(mNumBuffers, width, height, mFormat, mFlags));

		for (UINT i = 0; i < mNumBuffers; ++i)
		{
			ThrowFailedHR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTarget[i])));

			device->CreateRenderTargetView(mRenderTarget[i], nullptr, mRtvHandle[i]);
		}

		mCurrentBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	}
	
	void present()
	{
		ThrowFailedHR(mSwapChain->Present(1, 0));
		mCurrentBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
	}
};

class ShaderCode
{
	ComPtr<ID3DBlob> code;
	
public:
	ShaderCode(const std::string& csoFile)
	{
		static const std::string csoFolder = "bin/cso/";

		if (csoFile.compare(csoFile.size() - 4, 4, ".cso") != 0)
		{
			throw Error("This is not .cso file.\n");
		}

		std::string filepath = csoFolder + csoFile;
		std::wstring wfilepath(filepath.begin(), filepath.end());

		ThrowFailedHR(D3DReadFileToBlob(wfilepath.c_str(), &code));
	}

	ShaderCode(const std::string& hlslFile, const std::string& entry, const std::string& target)
	{
		static const std::string hlslFolder = "";

		if (hlslFile.compare(hlslFile.size() - 5, 5, ".hlsl") != 0)
		{
			throw Error("This is not .hlsl file.\n");
		}

		std::string filepath = hlslFolder + hlslFile;
		std::wstring wfilepath(filepath.begin(), filepath.end());

		ComPtr<ID3DBlob> error;
		UINT compileFlags = 0;
#if defined(_DEBUG)
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
		D3DCompileFromFile(wfilepath.c_str(), nullptr, nullptr, entry.c_str(), target.c_str(), compileFlags, 0, &code, &error);
		if (error)
		{
			throw Error((char*)error->GetBufferPointer());
		}
	}

	D3D12_SHADER_BYTECODE refCode()
	{
		D3D12_SHADER_BYTECODE shadercode = { code->GetBufferPointer(), code->GetBufferSize() };
		return shadercode;
	}
};

class RootTable
{
	static const int maxRanges = 256;
	UINT numRanges = 0;
	D3D12_DESCRIPTOR_RANGE rangesPool[maxRanges];
	D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

public:
	RootTable(D3D12_SHADER_VISIBILITY shaderVisibility = D3D12_SHADER_VISIBILITY_ALL)
		: shaderVisibility(shaderVisibility) {}

	void setShaderVisibility(D3D12_SHADER_VISIBILITY visibleShader) { shaderVisibility = visibleShader; }

	void addRange(const char* registerName, UINT numDescriptors = 1)
	{
		UINT baseNumber;
		try {
			baseNumber = (UINT) std::stoi(registerName + 1);
		}
		catch (...) {
			throw Error();
		}

		if (registerName[0] == 't')
			rangesPool[numRanges].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		else if (registerName[0] == 'u')
			rangesPool[numRanges].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		else if (registerName[0] == 'b')
			rangesPool[numRanges].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		else
			throw Error();
		
		rangesPool[numRanges].BaseShaderRegister = baseNumber;
		rangesPool[numRanges].RegisterSpace = 0;
		rangesPool[numRanges].NumDescriptors = numDescriptors;
		rangesPool[numRanges].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		++numRanges;
	}
	
	D3D12_ROOT_PARAMETER getRootParam()
	{
		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = numRanges;
		param.DescriptorTable.pDescriptorRanges = rangesPool;
		param.ShaderVisibility = shaderVisibility;
		return param;
	}
};

class RootSignatureGenerator
{
	static const int maxParams = 256;
	static const int maxSamplers = 16;

	UINT numParams = 0;
	D3D12_ROOT_PARAMETER paramPool[maxParams];
	UINT numSamplers = 0;
	D3D12_STATIC_SAMPLER_DESC samplerPool[maxSamplers];
	D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

public:
	static const D3D12_ROOT_SIGNATURE_FLAGS pixelOnly =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	static const D3D12_ROOT_SIGNATURE_FLAGS vertexOnly =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	void addRootParam(const D3D12_ROOT_PARAMETER& param)
	{
		paramPool[numParams] = param;
		++numParams;
	}

	void addStaticSampler(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL)
	{
		samplerPool[numSamplers] = {};
		samplerPool[numSamplers].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerPool[numSamplers].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerPool[numSamplers].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerPool[numSamplers].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerPool[numSamplers].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerPool[numSamplers].MaxLOD = D3D12_FLOAT32_MAX;
		samplerPool[numSamplers].ShaderRegister = shaderRegister;
		samplerPool[numSamplers].ShaderVisibility = visibility;
		++numSamplers;
	}

	void addStaticSampler(D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL) { addStaticSampler(numSamplers, visibility); }
	void setFlags(D3D12_ROOT_SIGNATURE_FLAGS newFlags) { flags = newFlags; }

	void createRootSignature(ID3D12Device* device, ID3D12RootSignature** ppRootSig)
	{
		D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
		{
			rootSigDesc.NumParameters = numParams;
			rootSigDesc.pParameters = numParams ? paramPool : nullptr;
			rootSigDesc.NumStaticSamplers = numSamplers;
			rootSigDesc.pStaticSamplers = numSamplers ? samplerPool : nullptr;
			rootSigDesc.Flags = flags;
		}

		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> error;
		D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
		if (error)
		{
			throw Error((char*)error->GetBufferPointer());
		}
		ThrowFailedHR(device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(ppRootSig)));
	}
};

enum DescriptorType {
	RTV, CBV, SRV, UAV, DSV, Sampler
};

struct DescriptorDesc
{
	DescriptorType								type;

	union {
		D3D12_RENDER_TARGET_VIEW_DESC*			rtvDesc;
		D3D12_CONSTANT_BUFFER_VIEW_DESC*		cbvDesc;
		D3D12_SHADER_RESOURCE_VIEW_DESC*		srvDesc;
		struct {
			D3D12_UNORDERED_ACCESS_VIEW_DESC*	uavDesc;
			ID3D12Resource*						counterResource;
		};
		D3D12_SAMPLER_DESC*						samplerDesc;
		D3D12_DEPTH_STENCIL_VIEW_DESC*			dsvDesc;
	};
};

inline DescriptorDesc Srv_Desc(D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr)
{
	DescriptorDesc dscrDesc;
	dscrDesc.type = SRV;
	dscrDesc.srvDesc = srvDesc;
	return dscrDesc;
}

inline DescriptorDesc Uav_Desc(D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr, ID3D12Resource* counterResource = nullptr)
{
	DescriptorDesc dscrDesc;
	dscrDesc.type = UAV;
	dscrDesc.uavDesc = uavDesc;
	dscrDesc.counterResource = counterResource;
	return dscrDesc;
}

class Descriptor
{
	DescriptorType					mType;
	D3D12_CPU_DESCRIPTOR_HANDLE		mCpuAddress;
	D3D12_GPU_DESCRIPTOR_HANDLE		mGpuAddress;

public:
	D3D12_CPU_DESCRIPTOR_HANDLE& cpuAddress() { return mCpuAddress; }
	D3D12_GPU_DESCRIPTOR_HANDLE& gpuAddress() { return mGpuAddress; }

	Descriptor(DescriptorType type,
		const D3D12_CPU_DESCRIPTOR_HANDLE& cpuAddress,
		const D3D12_GPU_DESCRIPTOR_HANDLE& gpuAddress)
	: mType(type), mCpuAddress(cpuAddress), mGpuAddress(gpuAddress) {}

	void create(ID3D12Resource* resource, const DescriptorDesc& dscrDesc)
	{
		ID3D12Device* device; resource->GetDevice(IID_PPV_ARGS(&device));
		create(device, resource, dscrDesc);
	}

	void create(ID3D12Device* device, ID3D12Resource* resource, const DescriptorDesc& dscrDesc)
	{
		if (mType != dscrDesc.type)
			throw Error();

		switch (mType)
		{
		case RTV:
			device->CreateRenderTargetView(resource, dscrDesc.rtvDesc, mCpuAddress);
			break;
		case CBV:
			device->CreateConstantBufferView(dscrDesc.cbvDesc, mCpuAddress);
			break;
		case SRV:
			device->CreateShaderResourceView(resource, dscrDesc.srvDesc, mCpuAddress);
			break;
		case UAV:
			device->CreateUnorderedAccessView(resource, dscrDesc.counterResource, dscrDesc.uavDesc, mCpuAddress);
			break;
		case DSV:
			device->CreateDepthStencilView(resource, dscrDesc.dsvDesc, mCpuAddress);
			break;
		case Sampler:
			device->CreateSampler(dscrDesc.samplerDesc, mCpuAddress);
			break;
		}
	}
};

class Resource
{
	ComPtr<ID3D12Resource>			mResource;
	D3D12_RESOURCE_STATES			mResourceState;

	static const UINT				maxDeRefCount = 8;
	Descriptor*						mDeRefDescriptors[maxDeRefCount] = {};
	UINT							mDeRefCount = 0;

	friend class DescriptorHeap;
	void addDescriptor(Descriptor* pDscr)
	{
		if (mDeRefCount >= maxDeRefCount)
			throw Error();
		mDeRefDescriptors[mDeRefCount++] = pDscr;
	}

public:
	ID3D12Resource* Get()					{ return mResource.Get(); }
	
	void allocate(
		ID3D12Device* device,
		D3D12_HEAP_TYPE heapType,
		D3D12_RESOURCE_DESC* resouceDesc,
		D3D12_RESOURCE_STATES initialState,
		D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE)
	{
		mResourceState = initialState;

		ThrowFailedHR(device->CreateCommittedResource(
			&Heap_Property(heapType), flags, resouceDesc, mResourceState, nullptr, IID_PPV_ARGS(&mResource)));
	}

	void resize(UINT64 width, UINT height = 1)
	{
		ID3D12Device* device;
		D3D12_HEAP_PROPERTIES hp;
		D3D12_RESOURCE_DESC desc = mResource->GetDesc();
		mResource->GetDevice(IID_PPV_ARGS(&device));
		mResource->GetHeapProperties(&hp, nullptr);

		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			desc.Width = width;
		}
		else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		{
			desc.Width = width;
			desc.Height = height;
		}
		else throw Error();

		ThrowFailedHR(device->CreateCommittedResource(
			&hp, D3D12_HEAP_FLAG_NONE, &desc, mResourceState, nullptr, IID_PPV_ARGS(&mResource)));
	}

	void modifyDescriptor(UINT dscrOrder, const DescriptorDesc& dscrDesc)
	{
		if (dscrOrder >= mDeRefCount)
			throw Error();
		mDeRefDescriptors[dscrOrder]->create(mResource.Get(), dscrDesc);
	}

	D3D12_RESOURCE_STATES changeResourceState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
	{
		D3D12_RESOURCE_STATES prevState = mResourceState;
		mResourceState = newState;
		cmdList->ResourceBarrier(1, &Barrier_Transition(mResource.Get(), prevState, newState));
		return prevState;
	}
};

class DescriptorHeap
{
	D3D12_DESCRIPTOR_HEAP_TYPE			mHeapType;
	ComPtr<ID3D12DescriptorHeap>		mHeap;
	Descriptor**						mDscrArr;
	UINT								mMaxDescriptors = 0;
	UINT								mNumDescriptors = 0;
	UINT								mDescriptorSize = 0;

public:
	ID3D12DescriptorHeap* Get()										{ return mHeap.Get(); }
	Descriptor& getDescriptor(UINT idx)								{ return *(mDscrArr[idx]); }
	D3D12_GPU_DESCRIPTOR_HANDLE& getDescriptorGPUAddress(UINT idx)	{ return mDscrArr[idx]->gpuAddress(); }

	~DescriptorHeap()
	{
		if (mDscrArr)
		{
			for (UINT i = 0; i < mNumDescriptors; ++i)
				delete mDscrArr[i];
			delete[] mDscrArr;
		}
	}

	void release()
	{
		if (mHeap)
		{
			mMaxDescriptors = 0;
			mNumDescriptors = 0;
			if (mDscrArr)
			{
				for (UINT i = 0; i < mNumDescriptors; ++i)
					delete mDscrArr[i];
				delete[] mDscrArr;
			}
			mHeap.Reset();
		}
	}

	void allocate(ID3D12Device* device, UINT maxDescriptors,
		D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		release();

		mHeapType = heapType;

		mDescriptorSize = device->GetDescriptorHandleIncrementSize(mHeapType);
		
		mMaxDescriptors = maxDescriptors;

		mDscrArr = new Descriptor* [mMaxDescriptors];

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		{
			heapDesc.Type = mHeapType;
			heapDesc.NumDescriptors = mMaxDescriptors;
			heapDesc.Flags = (mHeapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ?
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		}

		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap));
	}

	void pushDescriptor(Resource* resource, const DescriptorDesc& dscrDesc)
	{
		if (mNumDescriptors >= mMaxDescriptors)
			throw Error();

		D3D12_CPU_DESCRIPTOR_HANDLE cpuAddress = mHeap->GetCPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE	gpuAddress = mHeap->GetGPUDescriptorHandleForHeapStart();
		cpuAddress.ptr += mNumDescriptors * mDescriptorSize;
		gpuAddress.ptr += mNumDescriptors * mDescriptorSize;

		Descriptor* dscr = new Descriptor(dscrDesc.type, cpuAddress, gpuAddress);

		if (resource)
		{
			dscr->create(resource->Get(), dscrDesc);
			resource->addDescriptor(dscr);
		}
		else
		{
			ID3D12Device* device; mHeap->GetDevice(IID_PPV_ARGS(&device));
			dscr->create(device, nullptr, dscrDesc);
		}
		
		mDscrArr[mNumDescriptors] = dscr;
		
		mNumDescriptors++;
	}
};

class UploadBuffer
{
	static const int					resourceAlignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

	ComPtr<ID3D12Resource>				mBuffer;
	UINT64								mBufferSize = 0;
	BYTE*								mBufferVirtualAddress = nullptr;

public:
	~UploadBuffer()
	{
		if (mBufferVirtualAddress)
			mBuffer->Unmap(0, nullptr);
	}

	void release()
	{
		if (mBuffer)
		{
			if (mBufferVirtualAddress)
				mBuffer->Unmap(0, nullptr);

			mBufferVirtualAddress = nullptr;
			mBuffer.Reset();
			mBufferSize = 0;
		}
	}

	void allocate(ID3D12Device* device, UINT64 requiredSize)
	{
		if (requiredSize > mBufferSize)
		{
			release();
			mBufferSize = _align(requiredSize, resourceAlignment);
			mBuffer.Attach(createCommittedBuffer(device, mBufferSize, D3D12_HEAP_TYPE_UPLOAD));
			mBuffer->Map(0, &Range(0), (void**)&mBufferVirtualAddress);
		}
	}

	void upload(
		ID3D12GraphicsCommandList* cmdList,
		ID3D12Resource* target,
		D3D12_RESOURCE_STATES currentState,
		void* srcData)
	{
		ID3D12Device* device; target->GetDevice(IID_PPV_ARGS(&device));
		upload(device, cmdList, target, currentState, srcData);
	}

	void upload(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12Resource* target,
		D3D12_RESOURCE_STATES currentState,
		void* srcData)
	{
		D3D12_RESOURCE_DESC desc = target->GetDesc();
		
		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			allocate(device, desc.Width);

			memcpy(mBufferVirtualAddress, srcData, desc.Width);

			D3D12_RESOURCE_BARRIER barrier = Barrier_Transition(target, currentState, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->ResourceBarrier(1, &barrier);
			cmdList->CopyBufferRegion(target, 0, mBuffer.Get(), 0, desc.Width);
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = currentState;
			cmdList->ResourceBarrier(1, &barrier);
		}
		else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
		{
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
			UINT64 rowPureSize;		// width * bytesPerPixel
			UINT rowPitch;			// align( width * bytesPerPixel, 256 )
			UINT rows;				// height
			UINT64 copyableSize;	// (rows-1) * rowPitch + rowPureSize  ( == _textureDataSize(desc.Format, desc.Width, desc.Height) )
			
			device->GetCopyableFootprints(&desc, 0, 1, 0,
				&textureLayout, &rows, &rowPureSize, &copyableSize);
			rowPitch = textureLayout.Footprint.RowPitch;

			allocate(device, copyableSize);
			
			if (rowPitch == rowPureSize)
			{
				memcpy(mBufferVirtualAddress, srcData, rowPitch * rows);
			}
			else
			{
				for (UINT j = 0; j < rows; ++j)
				{
					memcpy(mBufferVirtualAddress + j * rowPitch,
						(BYTE*)srcData + j * rowPureSize, rowPureSize);
				}
			}

			D3D12_RESOURCE_BARRIER barrier = Barrier_Transition(target, currentState, D3D12_RESOURCE_STATE_COPY_DEST);
			cmdList->ResourceBarrier(1, &barrier);

			D3D12_TEXTURE_COPY_LOCATION src;
			src.pResource = mBuffer.Get();
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = textureLayout;
			D3D12_TEXTURE_COPY_LOCATION dst;
			dst.pResource = target;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.SubresourceIndex = 0;
			cmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			barrier.Transition.StateAfter = currentState;
			cmdList->ResourceBarrier(1, &barrier);
		}
		else throw Error();
	}
};

class BinaryFence
{
	ComPtr<ID3D12Fence>	mFence;
	HANDLE				mFenceEvent = nullptr;
	
	enum FenceValue {
		zero = 0, one = 1
	};

public:

	~BinaryFence()
	{
		if(mFenceEvent)
			CloseHandle(mFenceEvent);
	}

	void create(ID3D12Device* device)
	{
		ThrowFailedHR(device->CreateFence(zero, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	void waitCommandQueue(ID3D12CommandQueue* cmdQueue)
	{
		ThrowFailedHR(cmdQueue->Signal(mFence.Get(), one));

		if (mFence->GetCompletedValue() != one)
		{
			ThrowFailedHR(mFence->SetEventOnCompletion(one, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}

		ThrowFailedHR(cmdQueue->Signal(mFence.Get(), zero));
	}
};