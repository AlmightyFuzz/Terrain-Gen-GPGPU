#include "GPGPUFractalGeneratorPCH.h"

#include "DirectXClass.h"
#include "Window.h"
#include "OpenCL.h"
#include "ReferenceCode.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPWSTR cmdLine, int cmdShow)
{
	UNREFERENCED_PARAMETER(prevInstance);
	UNREFERENCED_PARAMETER(cmdLine);

	Window window = Window();
	DirectXClass directX = DirectXClass();
	OpenCL openCL = OpenCL();
	ReferenceCode refCode = ReferenceCode(); //Used to time the terrain algorithms being executed on the CPU

	if (!window.Init(hInstance, cmdShow))
		return 0;
	
	if (!directX.Initialise(window.getHandle(), hInstance))
	{
		directX.CleanUp();
		return 0;
	}

	if (!openCL.Initialise(directX.Device(), directX.VertexBuffer(), directX.IndexBuffer()))
	{
		openCL.CleanUp();
		return 0;
	}

	refCode.Run();

	static DWORD previousTime = timeGetTime();
	static const float targetFramerate = 30.0f;
	static const float maxTimeStep = 1.0f / targetFramerate;
	
	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DWORD currentTime = timeGetTime();
			float deltaTime = (currentTime - previousTime) / 1000.0f;
			previousTime = currentTime;
	
			directX.Update(deltaTime);
			directX.Render();
		}
	}

	// Clean up
	directX.CleanUp();
	openCL.CleanUp();
	refCode.Cleanup();
}