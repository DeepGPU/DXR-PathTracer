#pragma once
#include "IGRTScreen.h"
#include "dxHelpers.h"


class D3D12Screen : public IGRTScreen
{
	static const DXGI_FORMAT		tracerOutFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
	static const DXGI_FORMAT		screenOutFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const uint				backBufferCount = 2;

	ID3D12Device*					mDevice;
	ID3D12CommandQueue*				mCmdQueue;
	ID3D12GraphicsCommandList*		mCmdList;
	ID3D12CommandAllocator*			mCmdAllocator;
	BinaryFence						mFence;
	void initD3D12();

	SwapChain						mSwapChain;
	
	DescriptorHeap					mSrvUavHeap;

	RootSignature					mRootSig;
	void declareRootSignature();

	ID3D12PipelineState*			mPipeline;
	void createPipeline();

	DefaultTexture					mTracerOutTexture;
	UploadBuffer					mTextureUploader;
	void initializeResources();

	void fillCommandLists();

public:
	~D3D12Screen();
	D3D12Screen(HWND hwnd, uint width, uint height);
	virtual void onSizeChanged(uint width, uint height);
	virtual void display(const TracedResult& trResult);
};