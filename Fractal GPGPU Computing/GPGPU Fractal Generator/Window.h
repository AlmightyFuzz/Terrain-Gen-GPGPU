#pragma once

#include "GPGPUFractalGeneratorPCH.h"

//
// This class is responsible for creating a window DirectX to use, as well as a console window.
//
class Window
{
private:
	LONG m_WindowWidth;
	LONG m_WindowHeight;
	LPCSTR m_WindowClassName;
	LPCSTR m_WindowName;
	HWND m_WindowHandle;

	static LRESULT CALLBACK	WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
	Window();
	bool Init(HINSTANCE hInstance, int cmdShow);

	HWND getHandle() const;
};