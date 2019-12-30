#pragma once
#include "IGRTCommon.h"


class IGRTScreen
{
protected:
	HWND	targetWindow;
	uint	screenW;
	uint	screenH;

	uint	tracerOutW;
	uint	tracerOutH;

public:
	IGRTScreen(HWND hwnd, uint w, uint h) : targetWindow(hwnd), screenW(w), screenH(h) {}
	virtual void onSizeChanged(uint width, uint height) = 0;
	virtual void display(const TracedResult& trResult) = 0;
};