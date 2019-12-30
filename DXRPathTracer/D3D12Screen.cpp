#include "pch.h"
#include "D3D12Screen.h"


namespace DescriptorID {
	enum {
		tracerOutTextureSRV = 0,	

		maxDesciptors = 1
	};
}


D3D12Screen::~D3D12Screen()
{
	SAFE_RELEASE(mCmdQueue);
	SAFE_RELEASE(mCmdList);
	SAFE_RELEASE(mCmdAllocator);
	SAFE_RELEASE(mPipeline);
	SAFE_RELEASE(mDevice);
}

D3D12Screen::D3D12Screen(HWND hwnd, uint width, uint height) 
: IGRTScreen(hwnd, width, height)
{
	tracerOutW = screenW;
	tracerOutH = screenH;

	initD3D12();

	mSwapChain.create(mCmdQueue, targetWindow, backBufferCount, screenOutFormat, screenW, screenH, false);

	mSrvUavHeap.create(DescriptorID::maxDesciptors);

	declareRootSignature();

	createPipeline();

	initializeResources();

	mFence.waitCommandQueue(mCmdQueue);
}

void D3D12Screen::initD3D12()
{
	mDevice = createDX12Device(getRTXAdapter());

	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowFailedHR(mDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&mCmdQueue)));

	ThrowFailedHR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCmdAllocator)));

	ThrowFailedHR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCmdAllocator, nullptr, IID_PPV_ARGS(&mCmdList)));
	ThrowFailedHR(mCmdList->Close());

	mFence.create(mDevice);
}

void D3D12Screen::declareRootSignature()
{
	assert(mSrvUavHeap.get() != nullptr);

	mRootSig.resize(1);
	mRootSig[0] = new RootTable("t0", mSrvUavHeap[DescriptorID::tracerOutTextureSRV].getGpuHandle());
	mRootSig.build(StaticSampler());
}

void D3D12Screen::createPipeline()
{	
	D3D12_INPUT_LAYOUT_DESC inputLayout = { nullptr, 0 };
	dxShader vertexShader(L"D3D12Screen.hlsl", "VSMain", "vs_5_0");
	dxShader pixelShader(L"D3D12Screen.hlsl", "PSMain", "ps_5_0");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = 
		Graphics_Pipeline_Desc(inputLayout, mRootSig.get(), vertexShader.getCode(), pixelShader.getCode(), mSwapChain.getBufferFormat());
		
	ThrowFailedHR(mDevice->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&mPipeline)));
}

void D3D12Screen::initializeResources()
{
	mTextureUploader.create(_textureDataSize(tracerOutFormat, 1920, 1080));
	mTracerOutTexture.create(tracerOutFormat, tracerOutW, tracerOutH);

	mSrvUavHeap[DescriptorID::tracerOutTextureSRV].assignSRV(mTracerOutTexture, nullptr);
}

void D3D12Screen::fillCommandLists()
{
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)screenW, (float)screenH, 0.0f, 1.0f};
	D3D12_RECT scissorRect = { 0, 0, (LONG)screenW, (LONG)screenH };
	
	mCmdList->SetPipelineState(mPipeline);

	mSrvUavHeap.bind(mCmdList);
	mRootSig.bindGraphics(mCmdList);

	mCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	mCmdList->RSSetViewports(1, &viewport);
	mCmdList->RSSetScissorRects(1, &scissorRect);
	mCmdList->OMSetRenderTargets(1, &mSwapChain.getRtv(), FALSE, nullptr);

	D3D12_RESOURCE_BARRIER barrier = Transition(mSwapChain.getRenderTatget(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCmdList->ResourceBarrier(1, &barrier);

	mCmdList->DrawInstanced(4, 1, 0, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	mCmdList->ResourceBarrier(1, &barrier);
}

void D3D12Screen::display(const TracedResult& trResult)
{
	ThrowFailedHR(mCmdAllocator->Reset());
	ThrowFailedHR(mCmdList->Reset(mCmdAllocator, nullptr));

	uint64 trResultPitch = trResult.width * trResult.pixelSize;
	assert(trResultPitch == mTracerOutTexture.getRowDataSize());

	memcpyPitch(mTextureUploader.map(), mTracerOutTexture.getRowPitch(),
		trResult.data, trResultPitch, trResultPitch, trResult.height);

	mTracerOutTexture.uploadData(mCmdList, mTextureUploader);

	fillCommandLists();

	ThrowFailedHR(mCmdList->Close());

	ID3D12CommandList* pCommandLists[] = { mCmdList };
	mCmdQueue->ExecuteCommandLists(1, pCommandLists);

	mSwapChain.present();

	mFence.waitCommandQueue(mCmdQueue);
}

void D3D12Screen::onSizeChanged(uint width, uint height)
{
	if (width == screenW && height == screenH)
		return;

	if (width == 0 || height == 0)
		return;

	tracerOutW = screenW = width;
	tracerOutH = screenH = height;

	mSwapChain.resize(screenW, screenH);

	mTracerOutTexture.destroy();
	mTracerOutTexture.create(tracerOutFormat, tracerOutW, tracerOutH);

	mSrvUavHeap[DescriptorID::tracerOutTextureSRV].assignSRV(mTracerOutTexture, nullptr);

	mFence.waitCommandQueue(mCmdQueue);
}