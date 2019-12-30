#pragma once
#include "pch.h"


enum MouseButton 
{
	LButton = 0, MButton = 1, RButton = 2, numButtons = 3
};

class InputEngine
{
	const HWND hwnd;

	int2 mousePos;				
	int2 mousePosPrev;	
	bool mousePressed[numButtons];	
	bool mousePressedPrev[numButtons];
	int2 mouseLastClickedPos[numButtons];
	
public:
	InputEngine(const HWND hwnd) : hwnd(hwnd) {
		mousePosPrev = mousePos = int2(0.0f, 0.0f);
		for (uint i = 0; i < numButtons; ++i) {
			mousePressedPrev[i] = mousePressed[i] = false;
			mouseLastClickedPos[i] = int2(0.0f, 0.0f);
		}
	}

	int2 getMousePos() const {
		return mousePos;
	}

	bool getMousePressed(MouseButton btn) const {
		return mousePressed[btn];
	}

	bool getMouseJustPressed(MouseButton btn) const {
		return !mousePressedPrev[btn] && mousePressed[btn];
	}

	bool getMouseJustReleased(MouseButton btn) const {
		return mousePressedPrev[btn] && !mousePressed[btn];
	}

	int2 getMouseDragged(MouseButton btn) const { 
		if(mousePressedPrev[btn] && mousePressed[btn])
			return mousePos - mousePosPrev;
		else 
			return int2(0, 0);
	}

	int2 getMouseDraggedSum(MouseButton btn) const { 
		if(mousePressed[btn])
			return mousePos - mouseLastClickedPos[btn];
		else 
			return int2(0, 0);
	}

	void mouseUpdate() {
		mousePressedPrev[LButton] = mousePressed[LButton];
		mousePressedPrev[MButton] = mousePressed[MButton];
		mousePressedPrev[RButton] = mousePressed[RButton];
		mousePosPrev = mousePos;

		if(GetForegroundWindow() != hwnd)
			return;

		POINT pos;
		GetCursorPos(&pos); 
		ScreenToClient(hwnd, &pos);
		mousePos = int2(pos.x, pos.y);

		mousePressed[LButton] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) > 0;
		mousePressed[MButton] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) > 0;
		mousePressed[RButton] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) > 0;

		if(getMouseJustPressed(LButton))
			mouseLastClickedPos[LButton] = mousePos;
		if(getMouseJustPressed(MButton))
			mouseLastClickedPos[MButton] = mousePos;
		if(getMouseJustPressed(RButton))
			mouseLastClickedPos[RButton] = mousePos;
	}

	void update() {
		mouseUpdate();
	}
};
