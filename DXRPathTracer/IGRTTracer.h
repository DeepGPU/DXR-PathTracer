#pragma once
#include "IGRTCommon.h"


class Scene;
class InputEngine;
class IGRTTracer
{
protected:
	uint	tracerOutW;
	uint	tracerOutH;

	Scene*	scene;

public:
	IGRTTracer(uint w, uint h) : tracerOutW(w), tracerOutH(h) {}
	virtual void onSizeChanged(uint width, uint height) = 0;
	virtual void update(const InputEngine& input) = 0;
	virtual TracedResult shootRays() = 0;
	virtual void setupScene(const Scene* scene) = 0;
};