#pragma once
#include "pch.h"


inline double getCurrentTime()
{
	static double inv_freq;
	static bool first = true;
	
	if (first)
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency( &freq );
		inv_freq = 1.0 / freq.QuadPart;
		first = false;
	}

	LARGE_INTEGER c_time;
	QueryPerformanceCounter( &c_time );
		
	return c_time.QuadPart * inv_freq;
}

inline double updateFPS(double delta = 1.0)
{
	static int frameCount = 0;
	static double lastTime = getCurrentTime();
	static double old_fps = 0.0;
	
	double fps;
	double curTime = getCurrentTime();

	++frameCount;

	if( curTime - lastTime >= delta )
	{
		fps = (double) frameCount / (curTime - lastTime);

		frameCount = 0;
		lastTime = curTime;
		old_fps = fps;
	}
	else
	{
		fps = old_fps;
	}

	return fps;
}
