#include "GPGPUFractalGeneratorPCH.h"

#include "Window.h"

Window::Window()
{
	m_WindowWidth = 1900;
	m_WindowHeight = 1040;
	m_WindowClassName = "Fractal Generator";
	m_WindowName = "Fractal Generator";
	m_WindowHandle = 0;
}

HWND Window::getHandle() const
{
	return m_WindowHandle;
};

bool Window::Init(HINSTANCE hInstance, int cmdShow)
{
	//Init console window
	//https://justcheckingonall.wordpress.com/2008/08/29/console-window-win32-app/ for console window code
	AllocConsole();

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)hOut, _O_TEXT);
	FILE* hf_out = _fdopen(hCrt, "w");
	setvbuf(hf_out, NULL, _IONBF, 1);
	*stdout = *hf_out;

	HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
	hCrt = _open_osfhandle((long)handle_in, _O_TEXT);
	FILE* hf_in = _fdopen(hCrt, "r");
	setvbuf(hf_in, NULL, _IONBF, 128);
	*stdin = *hf_in;

	//Create regular window
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = &WndProc;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = m_WindowClassName;

	if (!RegisterClassEx(&wndClass))
		return false;

	RECT windowRect = { 0, 0, m_WindowWidth, m_WindowHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_WindowHandle = CreateWindowA(m_WindowClassName, m_WindowName,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, hInstance, nullptr);

	if (!m_WindowHandle)
		return false;

	ShowWindow(m_WindowHandle, cmdShow);
	UpdateWindow(m_WindowHandle);

	return true;
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}