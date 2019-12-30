#include "pch.h"
#include "dxHelpers.h"
#include <d3dcompiler.h>


static IDXGIFactory2* gFactory = nullptr;
static ID3D12Device* gDevice = nullptr;
ID3D12DebugDevice* gDebugDevice = nullptr;


IDXGIFactory2* getFactory()
{
	if (!gFactory)
	{
#ifdef _DEBUG
		ID3D12Debug* debuger;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debuger))))
		{
			debuger->EnableDebugLayer();
		}
#endif
		ThrowFailedHR(CreateDXGIFactory1(IID_PPV_ARGS(&gFactory)));
	}

	return gFactory;
}


IDXGIAdapter* getRTXAdapter()
{
	static IDXGIAdapter* adapter = nullptr;

	if (!adapter)
	{
		for (uint i = 0; DXGI_ERROR_NOT_FOUND != getFactory()->EnumAdapters(i, &adapter); i++)
		{
			ID3D12Device* tempDevice;
			ThrowFailedHR(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&tempDevice)));

			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
			HRESULT hr = tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

			if (FAILED(hr))
			{
				throw Error("Your Window 10 version must be at least 1809 to support D3D12_FEATURE_D3D12_OPTIONS5.");
			}

			if (features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
			{
				return adapter;
			}
		}

		throw Error("There are not DXR supported adapters(video cards such as Nvidia's Volta or Turing RTX). Also, The DXR fallback layer is not supported in this application.");
	}

	return adapter;
}


ID3D12Device* createDX12Device(IDXGIAdapter* adapter)
{
	ThrowFailedHR(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&gDevice)));
#ifdef _DEBUG
	gDevice->QueryInterface(IID_PPV_ARGS(&gDebugDevice));
#endif
	return gDevice;
}


D3D12_GRAPHICS_PIPELINE_STATE_DESC Graphics_Pipeline_Desc(
	D3D12_INPUT_LAYOUT_DESC inputLayout,
	ID3D12RootSignature* rootSig,
	D3D12_SHADER_BYTECODE vertexShader, 
	D3D12_SHADER_BYTECODE pixelShader,
	DXGI_FORMAT renderTargetFormat,
	uint numRenderTargets)
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
	for(uint i=0; i<numRenderTargets; ++i)
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
		for (uint i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
	}
	pipelineDesc.BlendState = blendDesc;

	return pipelineDesc;
}


ID3D12Resource* createCommittedBuffer(
	uint64 bufferSize, 
	D3D12_HEAP_TYPE heapType,
	D3D12_RESOURCE_FLAGS resourceFlags,
	D3D12_RESOURCE_STATES resourceStates)
{
	ID3D12Resource* resource;
	ThrowFailedHR(gDevice->CreateCommittedResource(
		&HeapProperty(heapType), D3D12_HEAP_FLAG_NONE,
		&BufferDesc(bufferSize, resourceFlags), resourceStates,
		nullptr, IID_PPV_ARGS(&resource)));
	return resource;
}


// [Descriptor]
void Descriptor::assignSRV(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
	gDevice->CreateShaderResourceView(resource, desc, cpuAddr);
}

void Descriptor::assignUAV(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, ID3D12Resource* counter)
{
	gDevice->CreateUnorderedAccessView(resource, counter, desc, cpuAddr);
}

void Descriptor::assignRTV(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
	gDevice->CreateRenderTargetView(resource, desc, cpuAddr);
}

void Descriptor::assignCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC* desc)
{
	gDevice->CreateConstantBufferView(desc, cpuAddr);
}

void Descriptor::assignSRV(dxResource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
	assignSRV(resource.get(), desc);
	resource.setSRV(this);
}

void Descriptor::assignUAV(dxResource& resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc, ID3D12Resource* counter)
{
	assignUAV(resource.get(), desc, counter);
	resource.setUAV(this);
}

void Descriptor::assignRTV(dxResource& resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
	assignRTV(resource.get(), desc);
	resource.setRTV(this);
}


// [DescriptorHeap]
void DescriptorHeap::destroy()
{
	SAFE_RELEASE(heap);
	descriptorArr.clear();
}

void DescriptorHeap::create(uint maxDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	if(heap != nullptr)
		throw Error("destroy() must be called before re-creating.");

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	{
		heapDesc.Type = heapType;
		heapDesc.NumDescriptors = maxDescriptors;
		heapDesc.Flags = (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) ?
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}
	gDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

	D3D12_CPU_DESCRIPTOR_HANDLE cpuAddr = heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuAddr = heap->GetGPUDescriptorHandleForHeapStart();
	uint descriptorSize = gDevice->GetDescriptorHandleIncrementSize(heapType);

	descriptorArr.reserve(maxDescriptors);

	for (uint i = 0; i < maxDescriptors; ++i)
	{
		descriptorArr.push_back(Descriptor(cpuAddr, gpuAddr));
		cpuAddr = cpuAddr + descriptorSize;
		gpuAddr = gpuAddr + descriptorSize;
	}
}

void DescriptorHeap::bind(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetDescriptorHeaps(1, &heap);
}


// [RootParameter]
static inline uint findCommaOrNull(const char* codePos)
{
	uint i=0;
	for (; codePos[i] != '\0'; ++i)
	{
		if(codePos[i] == ',')
			break;
	}
	return i;
}

static inline uint spaceRemoveCopy(char* dst, const char* src, uint length)
{
	uint j = 0;
	for (uint i = 0; i < length; ++i)
	{
		if(src[i] == ' ' || src[i] == '\t')
			continue;
		dst[j++] = src[i];
	}
	dst[j] = '\0';
	return j;
}

static inline uint readDigit(const char* digitPos, uint* num)
{
	*num = 0;

	uint i = 0;
	for ( ; digitPos[i] != '\0'; ++i)
	{
		if ('0' <= digitPos[i] && digitPos[i] <= '9')
		{
			*num *= 10;
			*num += (uint) (digitPos[i] - '0');
		}
		else
		{
			if(i==0)
				throw Error("At least one digit must be placed even zero");
			break;
		}
	}

	return i;
}

static void readRegisterInfo(const char* token, 
	DescriptorType* type, uint* registerSpace, uint* registerBase, uint* numDescriptors)
{
	uint i = 0;

	if (token[0] == '(')
	{
		++i;
		i += readDigit(token + i, registerSpace);
		if(token[i] != ')')
			throw Error();
		++i;
	}
	else
	{
		*registerSpace = 0;
	}

	uint typePos = i;

	if (token[i] == 't')
	{
		*type = DescriptorType::SRV;
	}
	else if (token[i] == 'u')
	{
		*type = DescriptorType::UAV;
	}
	else if (token[i] == 'b')
	{
		*type = DescriptorType::CBV;
	}
	else 
		throw Error();

	++i;

	i += readDigit(token + i, registerBase);

	if (token[i] == '\0')
	{
		*numDescriptors = 1;
		return;
	}
	else if (token[i] != '-')
	{
		throw Error();
	}

	++i;
	
	if (token[i] == '\0')
	{
		*numDescriptors = uint(-1);		// unbounded number of descriptors
		return;
	}
	else if (token[i] != token[typePos])
	{
		throw Error();
	}

	++i;

	uint registerEnd;
	i += readDigit(token + i, &registerEnd);

	*numDescriptors = registerEnd - *registerBase + 1;

	if(token[i] != '\0')
		throw Error();
}


// [RootTable]
void RootTable::create(const char* code)
{
	char token[256];
	
	const char* codePos = code;

	while (*codePos != '\0')
	{
		DescriptorType type;
		uint registerBase;
		uint registerSpace;
		uint numDescriptors;

		uint tokenLengthWithSpace = findCommaOrNull(codePos);
		spaceRemoveCopy(token, codePos, tokenLengthWithSpace);
		readRegisterInfo(token, &type, &registerSpace, &registerBase, &numDescriptors);

		D3D12_DESCRIPTOR_RANGE range = {};
		range.RangeType = 
			(type == DescriptorType::SRV) ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV :
				(type == DescriptorType::UAV) ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV : D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		range.NumDescriptors = numDescriptors;
		range.BaseShaderRegister = registerBase;
		range.RegisterSpace = registerSpace;
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		rangeArr.push_back(range);

		codePos += tokenLengthWithSpace;
	}

	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param.DescriptorTable.NumDescriptorRanges = rangeArr.size();
	param.DescriptorTable.pDescriptorRanges = rangeArr.data();
}

void RootTable::bindCompute(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const
{
	cmdList->SetComputeRootDescriptorTable(paramIdx, tableStart);
}

void RootTable::bindGraphics(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const
{
	cmdList->SetGraphicsRootDescriptorTable(paramIdx, tableStart);
}


// [RootPointer]
void RootPointer::create(const char* code)
{
	char token[256];

	DescriptorType type;
	uint registerBase;
	uint registerSpace;
	uint numDescriptors;

	uint tokenLengthWithSpace = findCommaOrNull(code);
	spaceRemoveCopy(token, code, tokenLengthWithSpace);
	readRegisterInfo(token, &type, &registerSpace, &registerBase, &numDescriptors);

	assert(numDescriptors == 1 && code[tokenLengthWithSpace] == '\0');

	param.ParameterType = 
		(type == DescriptorType::SRV) ? D3D12_ROOT_PARAMETER_TYPE_SRV :
			(type == DescriptorType::UAV) ? D3D12_ROOT_PARAMETER_TYPE_UAV : D3D12_ROOT_PARAMETER_TYPE_CBV;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param.Descriptor.ShaderRegister = registerBase;
	param.Descriptor.RegisterSpace = registerSpace;
}

void RootPointer::bindCompute(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const
{
	if(param.ParameterType==D3D12_ROOT_PARAMETER_TYPE_SRV)
		cmdList->SetComputeRootShaderResourceView(paramIdx, resourceAddress);
	else if(param.ParameterType==D3D12_ROOT_PARAMETER_TYPE_UAV)
		cmdList->SetComputeRootUnorderedAccessView(paramIdx, resourceAddress);
	else if(param.ParameterType==D3D12_ROOT_PARAMETER_TYPE_CBV)
		cmdList->SetComputeRootConstantBufferView(paramIdx, resourceAddress);
	else throw Error();
}

void RootPointer::bindGraphics(ID3D12GraphicsCommandList* cmdList, uint paramIdx) const
{
	if(param.ParameterType==D3D12_ROOT_PARAMETER_TYPE_SRV)
		cmdList->SetGraphicsRootShaderResourceView(paramIdx, resourceAddress);
	else if(param.ParameterType==D3D12_ROOT_PARAMETER_TYPE_UAV)
		cmdList->SetGraphicsRootUnorderedAccessView(paramIdx, resourceAddress);
	else if(param.ParameterType==D3D12_ROOT_PARAMETER_TYPE_CBV)
		cmdList->SetGraphicsRootConstantBufferView(paramIdx, resourceAddress);
	else throw Error();
}


// [RootConstants]
void createRootConstantsParam(D3D12_ROOT_PARAMETER& param, const char* code, uint num32BitConsts)
{
	char token[256];

	DescriptorType type;
	uint registerBase;
	uint registerSpace;
	uint numDescriptors;

	uint tokenLengthWithSpace = findCommaOrNull(code);
	spaceRemoveCopy(token, code, tokenLengthWithSpace);
	readRegisterInfo(token, &type, &registerSpace, &registerBase, &numDescriptors);

	assert(type == DescriptorType::CBV);
	assert(numDescriptors == 1 && code[tokenLengthWithSpace] == '\0');

	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param.Constants.Num32BitValues = num32BitConsts;
	param.Constants.RegisterSpace = registerSpace;
	param.Constants.ShaderRegister = registerBase;
}


// [RootSignature]
RootSignature::~RootSignature()
{
	for (uint i = 0; i < pRootParamArr.size(); ++i)
	{
		SAFE_DELETE(pRootParamArr[i]);
	}
	SAFE_RELEASE(rootSig);
}

void RootSignature::build(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	Array<D3D12_STATIC_SAMPLER_DESC> empty;
	build(empty, flags);
}

void RootSignature::build(const D3D12_STATIC_SAMPLER_DESC& sampler, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	Array<D3D12_STATIC_SAMPLER_DESC> samplers(1);
	samplers[0] = sampler;
	build(samplers, flags);
}

void RootSignature::build(const Array<D3D12_STATIC_SAMPLER_DESC>& samplerArr, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	if(rootSig != nullptr)
		throw Error("It cannot be re-built.");

	Array<D3D12_ROOT_PARAMETER> paramArr(pRootParamArr.size());
	for (uint i = 0; i < paramArr.size(); ++i)
	{
		assert(pRootParamArr[i] != nullptr);
		paramArr[i] = pRootParamArr[i]->param;
	}

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = paramArr.size();
	rootSigDesc.pParameters = paramArr.size() > 0 ? paramArr.data() : nullptr;
	rootSigDesc.NumStaticSamplers = samplerArr.size();
	rootSigDesc.pStaticSamplers = samplerArr.size() > 0 ? samplerArr.data() : nullptr;
	rootSigDesc.Flags = flags;

	ID3DBlob* blob = nullptr, * error = nullptr;
	D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
	if (error)
	{
		throw Error((char*)error->GetBufferPointer());
	}

	ThrowFailedHR(gDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSig)));
	
	blob->Release();
}

void RootSignature::bindCompute(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetComputeRootSignature(rootSig);

	for (uint paramIdx = 0; paramIdx < pRootParamArr.size(); ++paramIdx)
	{
		pRootParamArr[paramIdx]->bindCompute(cmdList, paramIdx);
	}
}

void RootSignature::bindGraphics(ID3D12GraphicsCommandList* cmdList) const
{
	cmdList->SetGraphicsRootSignature(rootSig);

	for (uint paramIdx = 0; paramIdx < pRootParamArr.size(); ++paramIdx)
	{
		pRootParamArr[paramIdx]->bindGraphics(cmdList, paramIdx);
	}
}


// [dxResource]
void dxResource::destroy()
{
	SAFE_RELEASE(resource);
	gpuAddress = 0;
	srv = nullptr;
	uav = nullptr;
	rtv = nullptr;
}

D3D12_RESOURCE_STATES dxResource::changeResourceState(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)
{
	assert(resourceState != newState);
	D3D12_RESOURCE_STATES prevState = resourceState;
	resourceState = newState;
	cmdList->ResourceBarrier(1, &Transition(resource, prevState, newState));
	return prevState;
}


// [dxBuffer]
void dxBuffer::create(uint64 bufferSize, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS rscFlag, D3D12_RESOURCE_STATES rscState) 
{
	if(resource != nullptr)
		throw Error("destroy() must be called before re-creating.");

	resourceState = rscState;
	resource = createCommittedBuffer(bufferSize, heapType, rscFlag, rscState);
	gpuAddress = resource->GetGPUVirtualAddress();

	this->bufferSize = bufferSize;
}


// [UploadBuffer]
void UploadBuffer::create(uint64 bufferSize, void* initData)
{
	dxBuffer::create(bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ);

	if (initData)
	{
		map();
		memcpy(cpuAddress, initData, bufferSize);
		unmap();
	}
}

void* UploadBuffer::map()
{
	if (!cpuAddress)
		resource->Map(0, &Range(0), &cpuAddress);
	return cpuAddress;
}
	
void UploadBuffer::unmap()
{
	if (cpuAddress)
	{
		resource->Unmap(0, nullptr);
		cpuAddress = nullptr;
	}
}


// [DefaultBuffer]
void DefaultBuffer::uploadData(
	ID3D12GraphicsCommandList* cmdList,
	uint64 copyDstOffset, 
	uint64 copySize,
	const UploadBuffer& srcBuffer, 
	uint64 srcBufferOffset) 
{
	assert(copyDstOffset + copySize <= getBufferSize());
	assert(srcBufferOffset + copySize <= srcBuffer.getBufferSize());

	D3D12_RESOURCE_STATES prevState = changeResourceState(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->CopyBufferRegion(
		resource, copyDstOffset, srcBuffer.get(), srcBufferOffset, copySize);
	changeResourceState(cmdList, prevState);	
}


// [DefaultTexture]
void DefaultTexture::create(DXGI_FORMAT format, uint width, uint height, D3D12_RESOURCE_STATES state)
{
	if(resource != nullptr)
		throw Error("destroy() must be called before re-creating.");
	
	resourceState = state;
	ThrowFailedHR(gDevice->CreateCommittedResource(
		&HeapProperty(D3D12_HEAP_TYPE_DEFAULT), 
		D3D12_HEAP_FLAG_NONE, 
		&TextureDesc(format, width, height, D3D12_RESOURCE_FLAG_NONE), 
		resourceState, nullptr, IID_PPV_ARGS(&resource)));
	//gpuAddress = resource->GetGPUVirtualAddress();

	this->format = format;
	this->width = width;
	this->height = height;
	this->rowDataSize = _bpp(format) * width;
	this->rowPitch = _align(rowDataSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	this->textureSize = (height - 1) * rowPitch + rowDataSize;

#ifdef _DEBUG
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
	uint64 copyableSize;		
	gDevice->GetCopyableFootprints(&resource->GetDesc(), 0, 1, 0,
		&textureLayout, nullptr, nullptr, &copyableSize);
	assert(rowPitch == textureLayout.Footprint.RowPitch && textureSize == copyableSize);
#endif
}

void DefaultTexture::uploadData(
	ID3D12GraphicsCommandList* cmdList, 
	const UploadBuffer& srcBuffer, 
	uint64 srcBufferOffset)
{
	assert(srcBufferOffset + textureSize <= srcBuffer.getBufferSize());

	D3D12_TEXTURE_COPY_LOCATION dstDesc;
	dstDesc.pResource = resource;
	dstDesc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstDesc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION srcDesc;
	srcDesc.pResource = srcBuffer.get();
	srcDesc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcDesc.PlacedFootprint.Footprint.Depth = 1;
	srcDesc.PlacedFootprint.Footprint.Format = format;
	srcDesc.PlacedFootprint.Footprint.Width = width;
	srcDesc.PlacedFootprint.Footprint.Height = height;
	srcDesc.PlacedFootprint.Footprint.RowPitch = rowPitch;
	srcDesc.PlacedFootprint.Offset = srcBufferOffset;

	D3D12_RESOURCE_STATES prevState = changeResourceState(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->CopyTextureRegion(&dstDesc, 0, 0, 0, &srcDesc, nullptr);
	changeResourceState(cmdList, prevState);
}


// [ReadbackBuffer]
void ReadbackBuffer::destroy()
{
	SAFE_RELEASE(readbackBuffer);
	cpuAddress = nullptr;
}

void ReadbackBuffer::create(uint64 requiredSize)
{
	if(readbackBuffer != nullptr)
		throw Error("destroy() must be called before re-creating.");

	maxReadbackSize = _align(requiredSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	readbackBuffer = createCommittedBuffer(maxReadbackSize, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
}

void* ReadbackBuffer::map()
{
	if (!cpuAddress)
		readbackBuffer->Map(0, &Range(0), &cpuAddress);
	return cpuAddress;
}

void ReadbackBuffer::unmap()
{
	if (cpuAddress)
	{
		readbackBuffer->Unmap(0, nullptr);
		cpuAddress = nullptr;
	}
}

void ReadbackBuffer::readback(ID3D12GraphicsCommandList* cmdList, dxResource& source)
{
	D3D12_RESOURCE_DESC desc = source.get()->GetDesc();
		
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		if(desc.Width > maxReadbackSize)
			create(desc.Width * 2);

		D3D12_RESOURCE_STATES prevState = source.changeResourceState(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		cmdList->CopyBufferRegion(readbackBuffer, 0, source.get(), 0, desc.Width);
		source.changeResourceState(cmdList, prevState);
	}
	else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
	{
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
		UINT64 rowPureSize;		// width * bytesPerPixel
		UINT rowPitch;			// align( width * bytesPerPixel, 256 )
		UINT rows;				// height
		UINT64 copyableSize;	// (rows-1) * rowPitch + rowPureSize  ( == _textureDataSize(desc.Format, desc.Width, desc.Height) )
			
		gDevice->GetCopyableFootprints(&desc, 0, 1, 0,
			&textureLayout, &rows, &rowPureSize, &copyableSize);
		rowPitch = textureLayout.Footprint.RowPitch;

		if(copyableSize > maxReadbackSize)
			create(copyableSize * 2);

		D3D12_RESOURCE_STATES prevState = source.changeResourceState(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

		D3D12_TEXTURE_COPY_LOCATION srcTex;
		srcTex.pResource = source.get();
		srcTex.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcTex.SubresourceIndex = 0;
		D3D12_TEXTURE_COPY_LOCATION dstBuff;
		dstBuff.pResource = readbackBuffer;
		dstBuff.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dstBuff.PlacedFootprint = textureLayout;
		
		cmdList->CopyTextureRegion(&dstBuff, 0, 0, 0, &srcTex, nullptr);

		source.changeResourceState(cmdList, prevState);
	}
	else
	{
		assert(false);
	}
}


// [SwapChain]
void SwapChain::destroy()
{
	rtvHeap.destroy();

	for (uint i = 0; i < numBuffers; ++i)
	{
		SAFE_RELEASE(renderTargetArr[i]);
	}
	renderTargetArr.clear();

	SAFE_RELEASE(swapChain);
}

void SwapChain::present()
{
	ThrowFailedHR(swapChain->Present(1, 0));
	currentBufferIdx = swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::resize(uint width, uint height)
{
	if (bufferW == width && bufferH == height)
		return;

	bufferW = width;
	bufferH = height;

	for (uint i = 0; i < numBuffers; ++i)
	{
		renderTargetArr[i]->Release();
	}

	ThrowFailedHR(swapChain->ResizeBuffers(numBuffers, bufferW, bufferH, bufferFormat, flags));

	for (UINT i = 0; i < numBuffers; ++i)
	{
		ThrowFailedHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargetArr[i])));

		rtvHeap[i].assignRTV(renderTargetArr[i]);
	}

	currentBufferIdx = swapChain->GetCurrentBackBufferIndex();
}
	
void SwapChain::create(
	ID3D12CommandQueue* cmdQueue,
	HWND hwnd,
	uint numBackBuffers,
	DXGI_FORMAT format,
	uint width,
	uint height,
	bool allowTearing)
{
	if(swapChain != nullptr)
		throw Error("destroy() must be called before re-creating.");

	numBuffers = numBackBuffers;
	bufferW = width;
	bufferH = height;
	bufferFormat = format;
	flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = numBuffers;
	swapChainDesc.Width = bufferW;
	swapChainDesc.Height = bufferH;
	swapChainDesc.Format = bufferFormat;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Flags = flags;
	swapChainDesc.SampleDesc.Count = 1;

	ThrowFailedHR(getFactory()->CreateSwapChainForHwnd(
		cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr,
		(IDXGISwapChain1**)(IDXGISwapChain3**)&swapChain));

	renderTargetArr.resize(numBuffers, nullptr);
	rtvHeap.create(numBuffers, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (uint i = 0; i < numBuffers; ++i)
	{
		ThrowFailedHR(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargetArr[i])));

		rtvHeap[i].assignRTV(renderTargetArr[i]);
	}

	currentBufferIdx = swapChain->GetCurrentBackBufferIndex();
}


// [dxShader]
const LPCWSTR dxShader::csoFolder = L"bin/cso/";
const LPCWSTR dxShader::hlslFolder = L"";

static inline uint length(const wchar* wstr)
{
	uint idx = 0;
	for(const wchar* ch = wstr; *ch != L'\0'; ++ch)
		++idx;
	return idx;
}

static inline void copy(wchar* dst, const wchar* src, uint num)
{
	for (uint i = 0; i < num; ++i)
	{
		dst[i] = src[i];
	}
}

static inline uint addStr(wchar* dst, const wchar* src1, const wchar* src2)
{
	uint l1 = length(src1);
	uint l2 = length(src2);
	
	copy(dst, src1, l1);
	copy(dst + l1, src2, l2);
	dst[l1 + l2] = L'\0';

	return l1 + l2;
}

void dxShader::flush()
{
	SAFE_RELEASE(code);
}

void dxShader::load(LPCWSTR csoFile)
{
	wchar filePath[256];
	addStr(filePath, csoFolder, csoFile);
	ThrowFailedHR(D3DReadFileToBlob(filePath, &code));
}

void dxShader::load(LPCWSTR hlslFile, const char* entryFtn, const char* target)
{
	wchar filePath[256];
	addStr(filePath, hlslFolder, hlslFile);

	ID3DBlob* error = nullptr;
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ThrowFailedHR(D3DCompileFromFile(filePath, nullptr, nullptr, entryFtn, target, compileFlags, 0, &code, &error));
	if (error)
	{
		throw Error((char*)error->GetBufferPointer());
	}
}




//---------------------From here, DXR related helper---------------------------------//

ID3D12Resource* createAS(
	ID3D12GraphicsCommandList4* cmdList,
	const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& buildInput,
	ID3D12Resource** scrach)
{
	ID3D12Device5* device;
	gDevice->QueryInterface(&device);

	ID3D12Resource* AS;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
    device->GetRaytracingAccelerationStructurePrebuildInfo(&buildInput, &info);

	*scrach = createCommittedBuffer(info.ScratchDataSizeInBytes, 
		D3D12_HEAP_TYPE_DEFAULT, 
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	AS = createCommittedBuffer(info.ResultDataMaxSizeInBytes, 
		D3D12_HEAP_TYPE_DEFAULT, 
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
    asDesc.Inputs = buildInput;
    asDesc.DestAccelerationStructureData = AS->GetGPUVirtualAddress();
    asDesc.ScratchAccelerationStructureData = (*scrach)->GetGPUVirtualAddress();
	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = AS;
    cmdList->ResourceBarrier(1, &uavBarrier);

	device->Release();

	return AS;
}


void buildBLAS(ID3D12GraphicsCommandList4* cmdList,
	ID3D12Resource** blas,
	ID3D12Resource** scrach,
	const GPUMesh gpuMeshArr[],
	const D3D12_GPU_VIRTUAL_ADDRESS gpuTransformAddressArr[],
	uint numMeshes,
	uint vertexStride,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
	D3D12_RAYTRACING_GEOMETRY_DESC meshDesc = {};
	meshDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	meshDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	meshDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	meshDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
	meshDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

	Array<D3D12_RAYTRACING_GEOMETRY_DESC> geoDesc(numMeshes);
	for (uint i = 0; i < numMeshes; ++i)
	{
		geoDesc[i] = meshDesc;
		geoDesc[i].Triangles.VertexCount = gpuMeshArr[i].numVertices;
		geoDesc[i].Triangles.VertexBuffer.StartAddress = gpuMeshArr[i].vertexBufferVA;
		geoDesc[i].Triangles.IndexCount = gpuMeshArr[i].numTridices * 3;
		geoDesc[i].Triangles.IndexBuffer = gpuMeshArr[i].tridexBufferVA;
		geoDesc[i].Triangles.Transform3x4 = gpuTransformAddressArr[i];
	}
	
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInput = {};
	buildInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildInput.NumDescs = numMeshes;
    buildInput.Flags = buildFlags;
	// Below is necessary at this point. It must be placed before getting prebuild info.
	buildInput.pGeometryDescs = geoDesc.data();

	*blas = createAS(cmdList, buildInput, scrach);

}


void buildTLAS(ID3D12GraphicsCommandList4* cmdList,
	ID3D12Resource** tlas,
	ID3D12Resource** scrach,
	ID3D12Resource** instanceDescArr,
	ID3D12Resource* const blasArr[],
	const dxTransform transformArr[],
	uint numBlas,
	uint instanceMultiplier,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
	*instanceDescArr = createCommittedBuffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numBlas);

	D3D12_RAYTRACING_INSTANCE_DESC* pInsDescArr; 
	(*instanceDescArr)->Map(0, &Range(0), (void**)&pInsDescArr);
	{
		memset(pInsDescArr, 0, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * numBlas);
		for (uint i = 0; i < numBlas; ++i)
		{
			pInsDescArr[i].InstanceMask = 0xFF;
			pInsDescArr[i].InstanceContributionToHitGroupIndex = i * instanceMultiplier;
			*(dxTransform*)(pInsDescArr[i].Transform) = transformArr[i];
			pInsDescArr[i].AccelerationStructure = const_cast<ID3D12Resource*>(blasArr[i])->GetGPUVirtualAddress();
		}
	}
	(*instanceDescArr)->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInput = {};
    buildInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	buildInput.NumDescs = numBlas;
    buildInput.Flags = buildFlags;
	// Below is not necessary at this point. It can be placed after getting prebuild info.
	buildInput.InstanceDescs = (*instanceDescArr)->GetGPUVirtualAddress();

	*tlas = createAS(cmdList, buildInput, scrach);
}


void dxAccelerationStructure::build(
	ID3D12GraphicsCommandList4* cmdList,
	const Array<GPUMesh>& gpuMeshArr, 
	const Array<dxTransform>& transformArr, 
	//const Array<const Transform*>& pTransformArr, 
	uint vertexStride,
	uint instanceMultiplier,
	AccelerationStructureBuildMode buildMode,
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags)
{
	assert(tlas == nullptr);
	assert(gpuMeshArr.size() == transformArr.size());

	uint numObjs = gpuMeshArr.size();
	
	Array<D3D12_GPU_VIRTUAL_ADDRESS> transformAddressArr(numObjs, 0);

	if (buildMode != BLAS_PER_OBJECT_AND_TOP_LEVEL_TRANSFORM)
	{
		uint transformSize = (uint) _align(sizeof(dxTransform), D3D12_RAYTRACING_TRANSFORM3X4_BYTE_ALIGNMENT);
		assert(transformSize==sizeof(dxTransform));

		transformBuff.create(transformSize * numObjs, (void*) transformArr.data());
		
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddr = transformBuff.getGpuAddress();
		for (uint objIdx = 0; objIdx < numObjs; ++objIdx)
		{
			transformAddressArr[objIdx] = gpuAddr;
			gpuAddr += transformSize;
		}
	}
	
	uint numObjsPerBlas = (buildMode == ONLY_ONE_BLAS) ? numObjs : 1;
	uint numBottomLevels = (buildMode == ONLY_ONE_BLAS) ? 1 : numObjs;
	blas.resize(numBottomLevels, nullptr);
	scratch.resize(numBottomLevels + 1, nullptr);

	for (uint i = 0; i < numBottomLevels; ++i)
	{
		buildBLAS(cmdList, &blas[i], &scratch[i], 
			&gpuMeshArr[i], &transformAddressArr[i], numObjsPerBlas, vertexStride, buildFlags);
	}
	
	const Array<dxTransform>* topLevelTransform;
	Array<dxTransform> identityArr;
	if (buildMode != BLAS_PER_OBJECT_AND_TOP_LEVEL_TRANSFORM)
	{
		identityArr.resize(numBottomLevels, dxTransform(1.0f));
		topLevelTransform = &identityArr;
	}
	else
	{
		topLevelTransform = &transformArr;
	}

	buildTLAS(cmdList, &tlas, &scratch[numBottomLevels], &instances, 
		&blas[0], &(*topLevelTransform)[0], numBottomLevels, instanceMultiplier, buildFlags);
}

void dxAccelerationStructure::destroy()
{
	flush();
	for (uint i = 0; i < scratch.size(); ++i)
	{
		SAFE_RELEASE(blas[i]);
	}
	SAFE_RELEASE(tlas);
	blas.clear();
}

void dxAccelerationStructure::flush()
{
	for (uint i = 0; i < scratch.size(); ++i)
	{
		SAFE_RELEASE(scratch[i]);
	}
	SAFE_RELEASE(instances);
	transformBuff.destroy();
	scratch.clear();
}


// [RaytracingPipeline]
void RaytracingPipeline::destroy()
{
	dxrLib = nullptr;
	globalRS = nullptr;
	maxPayloadSize = 0;
	maxAttributeSize = 8;
	maxRayDepth = 1;
	hitGroupArr.clear();
	localRSArr.clear();	
	SAFE_RELEASE(sop);
	SAFE_RELEASE(rtPipeline);
}

void RaytracingPipeline::build()
{
	if(rtPipeline != nullptr)
		throw Error("destroy() must be called before re-building.");

	ID3D12Device5* device;
	gDevice->QueryInterface(&device);

	assert(dxrLib != nullptr);
	assert(globalRS != nullptr);

	D3D12_STATE_SUBOBJECT subobj = {};
	Array<D3D12_STATE_SUBOBJECT> subobjArr(4 + hitGroupArr.size() + localRSArr.size()*2);
	uint subobjIdx = 0;

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	dxilLibDesc.DXILLibrary = dxrLib->getCode();
	subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobj.pDesc = (void*) &dxilLibDesc;
	subobjArr[subobjIdx++] = subobj;

	D3D12_GLOBAL_ROOT_SIGNATURE grsDesc = {};
	grsDesc.pGlobalRootSignature = globalRS->get();
	subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobj.pDesc = (void*) &grsDesc;
	subobjArr[subobjIdx++] = subobj;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = maxRayDepth;
	subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobj.pDesc = (void*) &pipelineConfig;
	subobjArr[subobjIdx++] = subobj;

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxPayloadSizeInBytes = maxPayloadSize;
	shaderConfig.MaxAttributeSizeInBytes = maxAttributeSize;
	subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobj.pDesc = (void*) &shaderConfig;
	subobjArr[subobjIdx++] = subobj;

	Array<D3D12_HIT_GROUP_DESC> hitGroupDesc(hitGroupArr.size());
	for (uint i = 0; i < hitGroupArr.size(); ++i)
	{
		hitGroupDesc[i].Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
        hitGroupDesc[i].ClosestHitShaderImport = hitGroupArr[i].closestHit;
        hitGroupDesc[i].AnyHitShaderImport = hitGroupArr[i].anyHit;
        hitGroupDesc[i].HitGroupExport = hitGroupArr[i].groupName;
		hitGroupDesc[i].IntersectionShaderImport = nullptr;
		subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subobj.pDesc = (void*) &hitGroupDesc[i];
		subobjArr[subobjIdx++] = subobj;
	}

	Array<D3D12_LOCAL_ROOT_SIGNATURE> lrsDesc(localRSArr.size());
	Array<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> accociationDesc(localRSArr.size());
	for (uint i = 0; i < localRSArr.size(); ++i)
	{
		lrsDesc[i].pLocalRootSignature = localRSArr[i].rootSig->get();
		subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		subobj.pDesc = (void*) &lrsDesc[i];
		subobjArr[subobjIdx++] = subobj;

		accociationDesc[i].pSubobjectToAssociate = &subobjArr[subobjIdx-1];
		accociationDesc[i].NumExports = localRSArr[i].exportNames.size();
		accociationDesc[i].pExports = localRSArr[i].exportNames.data();
		subobj.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		subobj.pDesc = (void*) &accociationDesc[i];
		subobjArr[subobjIdx++] = subobj;
	}

	assert(subobjIdx==subobjArr.size());

	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = subobjArr.size();
	desc.pSubobjects = subobjArr.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	ThrowFailedHR(device->CreateStateObject(&desc, IID_PPV_ARGS(&rtPipeline)));

	rtPipeline->QueryInterface(IID_PPV_ARGS(&sop));

	device->Release();
	const_cast<dxShader*>(dxrLib)->flush();
}

void RaytracingPipeline::bind(ID3D12GraphicsCommandList4* cmdList)
{
	cmdList->SetPipelineState1(rtPipeline);
}

ShaderIdentifier* RaytracingPipeline::getIdentifier(LPCWSTR exportName)
{
	return (ShaderIdentifier*) sop->GetShaderIdentifier(exportName);
}


// [ShaderTable]
void ShaderTable::create(uint recordSize, uint maxRecords, uint uploaderSizeFactor) 
{
	this->recordSize = recordSize;
	this->maxRecords = maxRecords;
	
	D3D12_RESOURCE_STATES state = 
		D3D12_RESOURCE_STATE_COMMON;
		//D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	SelfBuffer::create(recordSize * maxRecords, uploaderSizeFactor, state);
}
	
D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ShaderTable::getTable() const 
{
	return getSubTable(0, maxRecords);
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ShaderTable::getSubTable(uint recordOffset, uint numRecords) const 
{
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE ret = {};
	ret.StartAddress = getGpuAddress() + recordOffset * recordSize;
	ret.StrideInBytes = recordSize;
	ret.SizeInBytes = recordSize * numRecords;
	return ret;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE ShaderTable::getRecord(uint recordOffset) const 
{
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE ret = {};
	ret.StartAddress = getGpuAddress() + recordOffset * recordSize;
	ret.SizeInBytes = recordSize;
	return ret;
}