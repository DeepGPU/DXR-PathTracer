/*
At the very beginning, this Integrated GPU Ray Tracing (IGRT) framework has been designed to support many GPU 
raytracing libraries(DirectX Raytracing, VulKan Raytracing, nVidia OptiX), and it consists of two stages.
Firstly, Ray tracing stage which shoots several rays per each pixel to render a HDR image, and secondly 
the hdr image displays on an output window using any kinds of graphic libraries(DX12, DX11, OpenGL, Vulkan and etc) 
irregardless of the choice for the raytracing library.
For example,
							[DXRPathTracer]						[OpenGLScreen]
[IGRTTracer]	<--------	[VulkanPathTracer]	=============>	[VulkanScreen]	-------->	[IGRTScreen]
				(inherit)	[OptiXPathTracer]	(TracedResult)	[D3D12Screen]	(inherit)

*/

#pragma once
#include "pch.h"


struct TracedResult
{
	void* data;
	uint width;
	uint height;
	uint pixelSize;
};