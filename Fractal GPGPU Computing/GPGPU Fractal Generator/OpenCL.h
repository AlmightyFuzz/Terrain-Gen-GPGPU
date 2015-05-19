#pragma once

#include "GPGPUFractalGeneratorPCH.h"
#include "Globals.h"

//
// This is the Host side of OpenCL and is responsible for setting everything up and then executing the kernels.
//
class OpenCL
{
private:
	struct TerrainVertex
	{
		cl_float4 Position;
		cl_float4 Normal;
	};

	//cl_platform_id m_platformID;
	cl_platform_id* m_platforms;
	cl_uint m_numPlatforms;
	cl_context m_context;
	cl_command_queue m_commandQueue;
	cl_program m_program;
	cl_device_id m_device;

	cl_kernel m_initVertsKernel;
	cl_kernel m_initIndsKernel;
	cl_kernel m_perlinKernel;
	cl_kernel m_applyHeightKernel;

	cl_mem m_VertexBuffer;
	cl_mem m_IndexBuffer;
	cl_mem m_perlinData;

	cl_event m_profEvent;
	double m_vertKernelTime;
	double m_IndKernelTime;
	double m_noiseKernelTime;
	double m_heightMapKernelTime;

	//cl_float* m_noiseData;// [TOTAL_GRID_POINTS]; //Used during debugging.
	
	cl_int m_errorCode;
	std::stringstream m_errorOutput;

	bool SelectPlatform();
	bool OpenCL::DisplayPlatformInfo(cl_platform_id p_id, cl_platform_info p_name, std::string p_string);
	bool CreateContext(ID3D11Device* p_3DDevice);
	bool CreateCommandQueue();
	bool CreateProgram(const char* p_fileName);
	bool CreateKernels();
	bool CreateKernel(std::string p_kernelName, cl_kernel* p_kernel);
	bool CreateMemoryObjects(ID3D11Buffer* p_vertexBuffer, ID3D11Buffer* p_indexBuffer);

	bool InitGrid();
	float random(float max);
	bool GeneratePerlinNoise();
	bool ApplyHeightMap();

	void CleanUp(std::string p_errorString);
	void ShowError(std::stringstream* p_outout);
	void ShowError(std::string p_outout);
	void Profile(cl_event p_event, std::string p_output, double* p_time);

public:
	OpenCL();
	bool Initialise(ID3D11Device* p_3DDevice, ID3D11Buffer* p_vertexBuffer, ID3D11Buffer* p_indexBuffer);
	void CleanUp();
};