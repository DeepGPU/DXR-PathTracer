#pragma once
#include "pch.h"


class InputEngine;
class PinholeCamera
{
	// Camera Intrinsic Parameters
	float fovY = 60.0f;
	float screenWidth = 1.0f;
	float screenHeight = 1.0f;
	
	// Camera Extrinsic Parameters
	float3 cameraPos;
	float3 cameraX;
	float3 cameraY;
	float3 cameraZ;

	float2 cameraAspect;
	void updateCameraAspect() {
		cameraAspect.y = tanf( (fovY*0.5f) * DEGREE );
		cameraAspect.x = cameraAspect.y * screenWidth / screenHeight;
	}

	bool changed = false;

public:
	const float2& getCameraAspect() const	{ return cameraAspect; }
	const float3& getCameraPos() const		{ return cameraPos; }
	const float3& getCameraX  () const		{ return cameraX; }
	const float3& getCameraY  () const		{ return cameraY; }
	const float3& getCameraZ  () const		{ return cameraZ; }
	const float getScreenWidth() const		{ return screenWidth; }
	const float getScreenHeight() const		{ return screenHeight; }
	const float getFovY() const				{ return fovY; }

	void setCameraPos(const float3& pos ) { cameraPos = pos; changed = true; }
	void setCameraX  (const float3& xDir) { cameraX = xDir;  changed = true; }
	void setCameraY  (const float3& yDir) { cameraY = yDir;  changed = true; }
	void setCameraZ  (const float3& zDir) { cameraZ = zDir;  changed = true; }
	void setScreenSize(float w, float h) {
		assert(w>0.f && h>0.f);
		screenWidth = w;
		screenHeight = h;
		updateCameraAspect();
		changed = true;
	}
	void setFovY(float fovY) {
		this->fovY = fovY;
		updateCameraAspect();
		changed = true;
	}

	bool notifyChanged() {
		if (changed) {
			changed = false;
			return true;
		}
		return false;
	}

	virtual void update(const InputEngine& input) {}
};


class OrbitCamera : public PinholeCamera
{
	static const float minOrbitRadius;
	float3 orbitCenter;
	float orbitRadius;
	float azimuth;		// [    0,  2Pi] -> [ 0.0, 1.0] mapped angle measured from positive z-axis in zx-plane with origin at target.
	float altitude;		// [-Pi/2, Pi/2] -> [-1.0, 1.0] mapped angle measured from azimuth-rotated z-axis. 

	float speedRatio = 80.0f;

	void updateCameraOrientation();
	void updateCameraPosition();

public:
	float getSpeedRatio() const { return speedRatio; }
	void setSpeedRatio(float speedRatio) { this->speedRatio = speedRatio; }
	void initOrbit(const float3& target, float distanceToTarget, float azimuth = 0.0f, float altitude = 0.1f);
	void orbit(float deltaAzimuth, float deltaAltitude);
	void truck(float deltaX, float deltaY);
	void dolly(float deltaRadius);
	virtual void update(const InputEngine& input);
};