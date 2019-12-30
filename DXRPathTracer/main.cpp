#include "pch.h"
#include "DXRPathTracer.h"
#include "D3D12Screen.h"
#include "SceneLoader.h"
#include "Input.h"
#include "timer.h"


HWND createWindow(const char* winTitle, uint width, uint height);

IGRTTracer* tracer;
IGRTScreen* screen;
uint width  = 1200;
uint height = 900;
bool minimized = false;

int main()
{
	HWND hwnd = createWindow("Integrated GPU Path Tracer", width, height);
	ShowWindow(hwnd, SW_SHOW);
	InputEngine input(hwnd);

	tracer = new DXRPathTracer(width, height);
	screen = new D3D12Screen(hwnd, width, height);

	SceneLoader sceneLoader;
	//Scene* scene = sceneLoader.push_testScene1();
	Scene* scene = sceneLoader.push_hyperionTestScene();
	tracer->setupScene(scene);
	
	double fps, old_fps = 0;
	while (IsWindow(hwnd))
	{
		input.update();
		
		if (!minimized)
		{
			tracer->update(input);
			TracedResult trResult = tracer->shootRays();
			screen->display(trResult);
		}

		MSG msg;
		while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		fps = updateFPS(1.0);
		if (fps != old_fps)
		{
			printf("FPS: %f\n", fps);
			old_fps = fps;
		}
	}

	return 0;
}

LRESULT CALLBACK msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND createWindow(const char* winTitle, uint width, uint height)
{
	WNDCLASSA wc = {};
	wc.lpfnWndProc = msgProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = "anything";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wc);

	RECT r{ 0, 0, (LONG)width, (LONG)height };
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);

	HWND hWnd = CreateWindowA(
		wc.lpszClassName,
		winTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		r.right - r.left,
		r.bottom - r.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);

	return hWnd;
}

LRESULT CALLBACK msgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (tracer && screen)
		{
			uint width = (uint) LOWORD(lParam);
			uint height = (uint) HIWORD(lParam);
			if (width == 0 || height == 0)
			{
				minimized = true;
				return 0;
			}
			else if (minimized)
			{
				minimized = false;
			}

			tracer->onSizeChanged(width, height);
			screen->onSizeChanged(width, height);
		}
		return 0;

	/*case WM_PAINT:
		if(tracer && screenOutFormat)
		{
			TracedResult trResult = tracer->shootRays();
			screen->display(trResult);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;*/
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
