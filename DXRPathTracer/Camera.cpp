#include "pch.h"
#include "Camera.h"
#include "Input.h"


const float OrbitCamera::minOrbitRadius = 0.01f;

inline void OrbitCamera::updateCameraOrientation() 
{
	float A	= azimuth * 2.0f * PI;	
	float B = altitude * 0.5f * PI;	
	float cA  = cosf(A), sA = sinf(A), cB = cosf(B), sB = sinf(B);

	setCameraX(float3(    cA,  0.0f,	-sA ));
	setCameraY(float3( sB*sA,   -cB,  sB*cA ));
	setCameraZ(float3(-cB*sA,   -sB, -cB*cA ));
}

inline void OrbitCamera::updateCameraPosition() 
{
	setCameraPos(orbitCenter - orbitRadius * getCameraZ());
}

void OrbitCamera::initOrbit(const float3& target, float distanceToTarget, float azimuth, float altitude)
{
	orbitCenter = target;
	orbitRadius = _max(distanceToTarget, minOrbitRadius);
	this->azimuth = azimuth;
	this->altitude = altitude;

	updateCameraOrientation();
	updateCameraPosition();
}

inline void OrbitCamera::orbit(float deltaAzimuth, float deltaAltitude)
{
	azimuth = azimuth + deltaAzimuth;
	if (azimuth < 0.0f)			azimuth += 1.0f; 
	else if (azimuth > 1.0f)	azimuth -= 1.0f; 

	altitude = _clamp(altitude + deltaAltitude, -0.95f, 0.95f);

	updateCameraOrientation();
	updateCameraPosition();
}

inline void OrbitCamera::truck(float stepX, float stepY)
{
	orbitCenter = orbitCenter + stepX * getCameraX() + stepY * getCameraY();

	updateCameraPosition();
}

inline void OrbitCamera::dolly(float deltaRadius)
{
	orbitRadius = _max(orbitRadius + deltaRadius, minOrbitRadius);

	updateCameraPosition();
}

void OrbitCamera::update(const InputEngine& input)
{
	int2 screenDelta;

	screenDelta = input.getMouseDragged(LButton);
	if (screenDelta.x != 0 || screenDelta.y != 0) {
		orbit( float(-screenDelta.x) / getScreenWidth(),
			   float( screenDelta.y) / (0.5f*getScreenHeight()) );
	}
	
	screenDelta = input.getMouseDragged(MButton);
	if (screenDelta.x != 0 || screenDelta.y != 0) {
		truck( float(-screenDelta.x) / speedRatio,
			   float(-screenDelta.y) / speedRatio );
	}

	screenDelta = input.getMouseDragged(RButton);
	if (screenDelta.y != 0) {
		dolly( float(-screenDelta.y) / speedRatio );
	}
}
