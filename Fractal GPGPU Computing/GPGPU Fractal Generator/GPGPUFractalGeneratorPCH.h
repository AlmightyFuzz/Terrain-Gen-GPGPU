//Only headers that don't frequently change should be here. So no project headers.

#include <Windows.h>

//DirectX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h> //?

//OpenCL
#include <CL\cl.h>
#include <CL\cl_ext.h>
#include <CL\cl_d3d11.h>

//Enable the DirectX 11 interop extension
#pragma OPENCL EXTENSION cl_khr_d3d11_sharing : enable

//STL
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

//Link library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")

#ifdef _WIN64
//#pragma comment(lib, "Libraries\\x86_64\\OpenCL.lib")
#pragma comment(lib, "Libraries\\x86_64\\libOpenCL.a")
#else
//#pragma comment(lib, "Libraries\\x86\\OpenCL.lib")
#pragma comment(lib, "Libraries\\x86\\libOpenCL.a")
#endif