#include "GPGPUFractalGeneratorPCH.h"

#include "OpenCL.h"
#include "Globals.h"

using namespace std;

//Initialise the OpenCL/DirectX sharing extension functions
clGetDeviceIDsFromD3D11KHR_fn		clGetDeviceIDsFromD3D11KHR = NULL;
clCreateFromD3D11BufferKHR_fn		clCreateFromD3D11BufferKHR = NULL;
clCreateFromD3D11Texture2DKHR_fn	clCreateFromD3D11Texture2DKHR = NULL;
clCreateFromD3D11Texture3DKHR_fn	clCreateFromD3D11Texture3DKHR = NULL;
clEnqueueAcquireD3D11ObjectsKHR_fn	clEnqueueAcquireD3D11ObjectsKHR = NULL;
clEnqueueReleaseD3D11ObjectsKHR_fn	clEnqueueReleaseD3D11ObjectsKHR = NULL;

//Macro used to instantiate the sharing functions
#define INITPFN(y,x) \
    x = (x ## _fn)clGetExtensionFunctionAddressForPlatform(y,#x);\
        if(!x) { printf("failed getting %s" #x); }

//Source https://code.google.com/p/fractalterraingeneration/downloads/detail?name=PerlinNoise.1.0.cpp&can=2&q=
float OpenCL::random(float max){
	int r;
	float s;

	r = rand();
	s = (float)(r & 0x7fff) / (float)0x7fff;

	return (s * max);
}

//
//Initialise deafult values for member variables
//
OpenCL::OpenCL()
{
	//m_platformID = NULL;
	m_platforms = nullptr;
	m_numPlatforms = 0;
	m_context = NULL;
	m_commandQueue = NULL;
	m_program = NULL;
	m_device = NULL;

	m_initVertsKernel = NULL;
	m_initIndsKernel = NULL;
	m_perlinKernel = NULL;
	m_applyHeightKernel = NULL;

	m_VertexBuffer = NULL;
	m_IndexBuffer = NULL;
	m_perlinData = NULL;

	m_errorCode = NULL;

	m_vertKernelTime = m_IndKernelTime = m_noiseKernelTime = m_heightMapKernelTime = 0;

	//m_noiseData = new cl_float[TOTAL_GRID_POINTS];
}

//
// Displays information to the console window about the given platform 
//
bool OpenCL::DisplayPlatformInfo(cl_platform_id p_id, cl_platform_info p_name, string p_string)
{
	size_t paramValueSize;

	//Ask how much space is needed
	m_errorCode = clGetPlatformInfo(p_id, p_name, 0, NULL, &paramValueSize);
	if (m_errorCode != CL_SUCCESS)
	{
		cout << "Failed to get paramater data for OpenCL platform: " << p_string << endl;
		return false;
	}

	//Allocate that much space
	char* platformInfo = (char*)alloca(sizeof(char) * paramValueSize);

	//Fill that space with the data
	m_errorCode = clGetPlatformInfo(p_id, p_name, paramValueSize, platformInfo, NULL);
	if (m_errorCode != CL_SUCCESS)
	{
		cout << "Failed to get OpenCL platform: " << p_string << endl;
		return false;
	}

	cout << "\t" << p_string << ":\t" << platformInfo << endl;
}

//
// Select the platform to run on
//
bool OpenCL::SelectPlatform()
{
	//cl_platform_id* platformIDs;

	//Get the total number of platforms available
	m_errorCode = clGetPlatformIDs(0, NULL, &m_numPlatforms);
	if (m_errorCode != CL_SUCCESS || m_numPlatforms <= 0)
	{
		CleanUp("Failed to find any OpenCL platforms.");
		return false;
	}

	//Make space
	m_platforms = (cl_platform_id*)alloca(sizeof(cl_platform_id)*m_numPlatforms);

	//Get the platform IDs
	m_errorCode = clGetPlatformIDs(m_numPlatforms, m_platforms, NULL);
	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Failed to find any OpenCL platforms.");
		return false;
	}

	cout << "OpenCL platforms: " << m_numPlatforms << endl;

	for (int i = 0; i < m_numPlatforms; i++)
	{
		DisplayPlatformInfo(m_platforms[i], CL_PLATFORM_PROFILE, "CL_PLATFORM_PROFILE");
		DisplayPlatformInfo(m_platforms[i], CL_PLATFORM_VERSION, "CL_PLATFORM_VERSION");
		DisplayPlatformInfo(m_platforms[i], CL_PLATFORM_VENDOR, "CL_PLATFORM_VENDOR");
		DisplayPlatformInfo(m_platforms[i], CL_PLATFORM_EXTENSIONS, "CL_PLATFORM_EXTENSIONS");

		cout << endl;
	}

	return true;
}

//
// Select the platform to run on and create a context for it
//
bool OpenCL::CreateContext(ID3D11Device* p_3DDevice)
{
	cl_uint numDevices;
	cl_device_id deviceID;
	int found = -1;

	if (!SelectPlatform())
		return false;
	
	//Try to create context with CL_PREFERRED_DEVICES_FOR_D3D11_KHR
	for (int i = 0; i < m_numPlatforms; i++)
	{
		cl_platform_id platform = m_platforms[i];

		//Initialise extension API function with platform
		INITPFN(platform, clGetDeviceIDsFromD3D11KHR);
		if (!clGetDeviceIDsFromD3D11KHR)
			//Failed to create function pointer, try the next platform
			continue;

		INITPFN(platform, clCreateFromD3D11BufferKHR);
		INITPFN(platform, clEnqueueAcquireD3D11ObjectsKHR);
		INITPFN(platform, clEnqueueReleaseD3D11ObjectsKHR);

		deviceID = NULL;
		numDevices = 0;

		m_errorCode = clGetDeviceIDsFromD3D11KHR(platform, CL_D3D11_DEVICE_KHR, (void*)p_3DDevice, CL_PREFERRED_DEVICES_FOR_D3D11_KHR, 1, &deviceID, &numDevices);

		//If unsuccessful, stop this loop and try the next one.
		if (m_errorCode != CL_SUCCESS)
			continue;

		if (numDevices > 0)
		{
			//Create a context on the platform
			cl_context_properties contextProperties[] =
			{
				CL_CONTEXT_D3D11_DEVICE_KHR,
				(cl_context_properties)p_3DDevice,
				CL_CONTEXT_PLATFORM,
				(cl_context_properties)platform,
				0
			};

			m_context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, NULL, NULL, &m_errorCode);

			if (m_errorCode != CL_SUCCESS)
			{
				cout << "Failed to create context." << endl;
				clReleaseDevice(deviceID);
			}
			else
			{
				found = i;
				return true;
			}
		}
	}

	if (found == -1)
	{
		//Now try with CL_ALL_DEVICES_FOR_D3D11_KHR

		for (int i = 0; i < m_numPlatforms; i++)
		{
			cl_platform_id platform = m_platforms[i];

			//Initialise extension API function with platform
			INITPFN(platform, clGetDeviceIDsFromD3D11KHR);
			if (!clGetDeviceIDsFromD3D11KHR)
				//Failed to create function pointer, try the next platform
				continue;

			INITPFN(platform, clCreateFromD3D11BufferKHR);
			INITPFN(platform, clEnqueueAcquireD3D11ObjectsKHR);
			INITPFN(platform, clEnqueueReleaseD3D11ObjectsKHR);

			deviceID = NULL;
			numDevices = 0;

				m_errorCode = clGetDeviceIDsFromD3D11KHR(platform, CL_D3D11_DEVICE_KHR, (void*)p_3DDevice, CL_ALL_DEVICES_FOR_D3D11_KHR, 1, &deviceID, &numDevices);

			//If unsuccessful, stop this loop and try the next one.
			if (m_errorCode != CL_SUCCESS)
				continue;

			if (numDevices > 0)
			{
				//Create a context on the platform
				cl_context_properties contextProperties[] =
				{
					CL_CONTEXT_D3D11_DEVICE_KHR,
					(cl_context_properties)p_3DDevice,
					CL_CONTEXT_PLATFORM,
					(cl_context_properties)platform,
					0
				};

				m_context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, NULL, NULL, &m_errorCode);

				if (m_errorCode != CL_SUCCESS)
				{
					cout << "Failed to create context." << endl;
					clReleaseDevice(deviceID);
				}
				else
				{
					found = i;
					return true;
				}
			}
		}

		if (found == -1)
		{
			ShowError("Can't create context for DirectX interop");
		}
	}

	return false;
}

//
// Create the Command Queue
//
bool OpenCL::CreateCommandQueue()
{
	cl_device_id* devices;
	size_t deviceBufferSize = -1;

	//Get the size of the devices buffer
	m_errorCode = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Failed to get context info.");
		return false;
	}

	if (deviceBufferSize <= 0)
	{
		CleanUp("No devices available.");
		return false;
	}

	//Allocate memory for devices buffer
	devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];

	m_errorCode = clGetContextInfo(m_context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
	if (m_errorCode != CL_SUCCESS)
	{
		delete[] devices;
		CleanUp("Failed to get device IDs.");
		return false;
	}

	cl_queue_properties profilingEnable[] = { CL_QUEUE_PROPERTIES,
		CL_QUEUE_PROFILING_ENABLE, 0 };

	m_commandQueue = clCreateCommandQueueWithProperties(m_context, devices[0], profilingEnable, &m_errorCode);
	if (m_commandQueue == NULL)
	{
		CleanUp("Failed to create Command Queue.");
		return false;
	}

	//Set the first device
	m_device = devices[0];
	delete[] devices;

	return true;
}

//
// Create the Program (ie a library of kernel functions)
//
bool OpenCL::CreateProgram(const char* p_fileName)
{
	ifstream kernelFile(p_fileName, ios::in);
	if (!kernelFile.is_open())
	{
		CleanUp("Failed to open kernel file");
		return false;
	}

	ostringstream oss;
	oss << kernelFile.rdbuf();

	string srcStdStr = oss.str();
	const char* srcStr = srcStdStr.c_str();

	m_program = clCreateProgramWithSource(m_context, 1, (const char**)&srcStr, NULL, &m_errorCode);
	if (m_program == NULL)
	{
		CleanUp("Failed to create kernel from source.");
		return false;
	}

	m_errorCode = clBuildProgram(m_program, 0, NULL, NULL, NULL, NULL);
	if (m_errorCode != CL_SUCCESS)
	{
		char buildLog[16384];
		clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);

		m_errorOutput << "Error in kernel: " << endl;
		m_errorOutput << buildLog;
		ShowError(&m_errorOutput);

		CleanUp();
		return false;
	}

	return true;
}

//
// Creates all the kernels that are to be executed.
//
bool OpenCL::CreateKernels()
{
	if (!CreateKernel("InitVertBuffer", &m_initVertsKernel)) return false;
	if (!CreateKernel("InitIndexBuffer", &m_initIndsKernel)) return false;
	if (!CreateKernel("PerlinNoise", &m_perlinKernel)) return false;
	if (!CreateKernel("ApplyHeightMap", &m_applyHeightKernel)) return false;

	return true;
}

//
// Creates a kernel object using its name in the Program object
//
bool OpenCL::CreateKernel(string p_kernelName, cl_kernel* p_kernel)
{
	*p_kernel = clCreateKernel(m_program, p_kernelName.c_str(), &m_errorCode);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Failed to create kernel: " + p_kernelName);
		return false;
	}

	return true;
}

//
//Create the necessary Memory Objects
//
bool OpenCL::CreateMemoryObjects(ID3D11Buffer* p_vertexBuffer, ID3D11Buffer* p_indexBuffer)
{
	m_VertexBuffer = clCreateFromD3D11BufferKHR(m_context, CL_MEM_READ_WRITE, p_vertexBuffer, &m_errorCode);
	m_IndexBuffer = clCreateFromD3D11BufferKHR(m_context, CL_MEM_READ_WRITE, p_indexBuffer, &m_errorCode);
	m_perlinData = clCreateBuffer(m_context, CL_MEM_READ_WRITE, sizeof(cl_float) * TOTAL_GRID_POINTS, NULL, &m_errorCode);

	if (m_errorCode != CL_SUCCESS)
	{
		ShowError("Failed to create memory objects");
		return false;
	}

	return true;
}

//
//Initialise the OpenCL object
//
bool OpenCL::Initialise(ID3D11Device* p_3DDevice, ID3D11Buffer* p_vertexBuffer, ID3D11Buffer* p_indexBuffer)
{
	
	if (!CreateContext(p_3DDevice))
		return false;

	if (!CreateCommandQueue())
		return false;

	string loc = "..\\Kernels\\";
	if (!CreateProgram((loc + "Kernels.cl").c_str()))
		return false;

	if (!CreateKernels())
		return false;

	if (!CreateMemoryObjects(p_vertexBuffer, p_indexBuffer))
		return false;

	cout << "Grid size: " << GRID_HEIGHT << " x " << GRID_WIDTH << endl << endl;

	if (!InitGrid())
		return false;

	if (!GeneratePerlinNoise())
		return false;

	if (!ApplyHeightMap())
		return false;

	//double totalTime = m_vertKernelTime + m_IndKernelTime + m_noiseKernelTime + m_heightMapKernelTime;
	//cout << "Total execution time: \t" << totalTime << " microseconds" << endl << endl;
	cout << endl;

	return true;
}

//
// Create the flat grid of points.
//
bool OpenCL::InitGrid()
{
	//DEBUGGING CODE
	//int const size = TOTAL_GRID_POINTS;
	//
	//cl_mem debugInds = clCreateBuffer(m_context, CL_MEM_READ_WRITE, sizeof(cl_int)*NUM_INDICES, NULL, &m_errorCode);
	//cl_mem debugVerts = clCreateBuffer(m_context, CL_MEM_READ_WRITE, sizeof(TerrainVertex) * size, NULL, &m_errorCode);

	//
	// Set the kernel arguments
	//
	m_errorCode = clSetKernelArg(m_initVertsKernel, 0, sizeof(cl_mem), &m_VertexBuffer);
	m_errorCode |= clSetKernelArg(m_initVertsKernel, 1, sizeof(cl_int), &GRID_WIDTH);
	m_errorCode |= clSetKernelArg(m_initVertsKernel, 2, sizeof(cl_int), &GRID_HEIGHT);
	//m_errorCode |= clSetKernelArg(m_initVertsKernel, 3, sizeof(cl_mem), &debugVerts);

	m_errorCode = clSetKernelArg(m_initIndsKernel, 0, sizeof(cl_mem), &m_IndexBuffer);
	m_errorCode |= clSetKernelArg(m_initIndsKernel, 1, sizeof(cl_int), &(GRID_WIDTH));
	//m_errorCode |= clSetKernelArg(m_initIndsKernel, 2, sizeof(cl_mem), &debugInds);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error setting kernel arguments");
		return false;
	}
	
	//
	// Execute the kernels
	//
	cl_mem D3D11MemObjects[2] = { m_VertexBuffer, m_IndexBuffer };

	//Aquire the D3D11 objects before manipulating them.
	m_errorCode = clEnqueueAcquireD3D11ObjectsKHR(m_commandQueue, 2, D3D11MemObjects, 0, NULL, NULL);

	//2 => 2D NDRange
	size_t globalWorkSizeV[2] = { GRID_WIDTH, GRID_HEIGHT };

	m_errorCode = clEnqueueNDRangeKernel(m_commandQueue, m_initVertsKernel, 2, NULL, globalWorkSizeV, NULL, 0, NULL, &m_profEvent);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error queueuing vertex kernel for execution");
		return false;
	}

	//Profile(m_profEvent, "Vertex Kernel ", &m_vertKernelTime);

	size_t globalWorkSizeI[3] = { (GRID_WIDTH - 1), (GRID_HEIGHT - 1), 6 };

	m_errorCode = clEnqueueNDRangeKernel(m_commandQueue, m_initIndsKernel, 3, NULL, globalWorkSizeI, NULL, 0, NULL, &m_profEvent);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error queueuing index kernel for execution");
		return false;
	}

	//Profile(m_profEvent, "Indices kernel", &m_IndKernelTime);

	//DEBUGGING CODE
	//cl_int debugIndsData[NUM_INDICES];
	//TerrainVertex* debugVertsData = new TerrainVertex[size];
	//
	//m_errorCode = clEnqueueReadBuffer(m_commandQueue, debugVerts, CL_TRUE, 0, sizeof(TerrainVertex) * size, debugVertsData, 0, NULL, NULL);
	//m_errorCode = clEnqueueReadBuffer(m_commandQueue, debugInds, CL_TRUE, 0, sizeof(cl_int) * NUM_INDICES, debugIndsData, 0, NULL, NULL);
	//
	//cout << "VERTICES:" << endl;
	//for (int i = 0; i < size; i++)
	//{
	//	cout << "(" << debugVertsData[i].Position.x << "," << debugVertsData[i].Position.y << "," << debugVertsData[i].Position.z << ") " << endl;
	//}
	//cout << endl;
	//
	//cout << "INDICES:" << endl;
	//for (int i = 0; i < NUM_INDICES; i++)
	//{
	//	cout << debugIndsData[i] << endl;
	//}
	
	m_errorCode = clEnqueueReleaseD3D11ObjectsKHR(m_commandQueue, 2, D3D11MemObjects, 0, NULL, NULL);

	return true;
}

//
// Execute the kernel that generates the Perlin noise.
//
bool OpenCL::GeneratePerlinNoise()
{
	//
	// Set up the necessary resources before giving them to the kernels
	//
	cl_float2 gradients[8];
	for (int i = 0; i < 8; i++)
	{
		gradients[i].x = cos(0.785398163f * (float)i); //0.785398163f = PI/4
		gradients[i].y = sin(0.785398163f * (float)i);
	}

	//Create array of incrementing ints, then ranomise their positions in the array
	int randomValues[(GRID_HEIGHT >= GRID_WIDTH ? GRID_HEIGHT : GRID_WIDTH)]; //Choose larger value

	int num = GRID_HEIGHT >= GRID_WIDTH ? GRID_HEIGHT : GRID_WIDTH;
	for (int i = 0; i < num; i++)
		randomValues[i] = i;

	for (int i = 0; i < num; i++)
	{
		int j = (int)random(num);
		int k = randomValues[i];
		randomValues[i] = randomValues[j];
		randomValues[j] = k;
	}

	int octaves = 16;

	cl_mem gradientData = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float2) * 8, &gradients, &m_errorCode);
	cl_mem randomValuesData = clCreateBuffer(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * num, &randomValues, &m_errorCode);

	m_errorCode = clSetKernelArg(m_perlinKernel, 0, sizeof(cl_mem), &m_perlinData);
	m_errorCode |= clSetKernelArg(m_perlinKernel, 1, sizeof(cl_mem), &gradientData);
	m_errorCode |= clSetKernelArg(m_perlinKernel, 2, sizeof(cl_mem), &randomValuesData);
	m_errorCode |= clSetKernelArg(m_perlinKernel, 3, sizeof(int), &GRID_WIDTH);
	m_errorCode |= clSetKernelArg(m_perlinKernel, 4, sizeof(int), &octaves);

	if(m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error setting kernel arguments");
		return false;
	}

	//
	// Execute the kernel
	//
	size_t globalWorkSize[2] = { GRID_WIDTH, GRID_HEIGHT };

	m_errorCode = clEnqueueNDRangeKernel(m_commandQueue, m_perlinKernel, 2, NULL, globalWorkSize, NULL, 0, NULL, &m_profEvent);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error queueuing index kernel for execution");
		return false;
	}

	Profile(m_profEvent, "Perlin noise kernel", &m_noiseKernelTime);

	//DEBUGGING CODE
	//m_errorCode = clEnqueueReadBuffer(m_commandQueue, m_perlinData, CL_TRUE, 0, sizeof(cl_float) * TOTAL_GRID_POINTS, m_noiseData, 0, NULL, NULL);
	//
	//for (int i = 0; i < TOTAL_GRID_POINTS; i++)
	//{
	//	cout << m_noiseData[i] << endl;
	//}

	return true;
}

//
// Execute the kernel that applies the height map data to the flat grid.
//
bool OpenCL::ApplyHeightMap()
{
	// Set the kernel arguments
	m_errorCode = clSetKernelArg(m_applyHeightKernel, 0, sizeof(cl_mem), &m_VertexBuffer);
	m_errorCode |= clSetKernelArg(m_applyHeightKernel, 1, sizeof(cl_mem), &m_perlinData);
	m_errorCode |= clSetKernelArg(m_applyHeightKernel, 2, sizeof(int), &GRID_WIDTH);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error setting Height Map kernel arguments");
		return false;
	}

	//
	// Acquire the DirectX resource, execute the kernel and then release the resource again
	//
	m_errorCode = clEnqueueAcquireD3D11ObjectsKHR(m_commandQueue, 1, &m_VertexBuffer, 0, NULL, NULL);

	size_t globalWorkSize[2] = { GRID_WIDTH, GRID_HEIGHT };

	m_errorCode = clEnqueueNDRangeKernel(m_commandQueue, m_applyHeightKernel, 2, NULL, globalWorkSize, NULL, 0, NULL, &m_profEvent);

	if (m_errorCode != CL_SUCCESS)
	{
		CleanUp("Error queueuing Height Map kernel for execution");
		return false;
	}

	//Profile(m_profEvent, "Height map kernel", &m_heightMapKernelTime);

	m_errorCode = clEnqueueReleaseD3D11ObjectsKHR(m_commandQueue, 2, &m_VertexBuffer, 0, NULL, NULL);

	return true;
}

//
// Clean up any resources OpenCL has used.
//
void OpenCL::CleanUp()
{
	if (m_commandQueue != 0) clReleaseCommandQueue(m_commandQueue);
	if (m_program != 0) clReleaseProgram(m_program);
	if (m_context != 0) clReleaseContext(m_context);
	if (m_device != 0) clReleaseDevice(m_device);

	if (m_initVertsKernel != 0) clReleaseKernel(m_initVertsKernel);
	if (m_initIndsKernel != 0) clReleaseKernel(m_initIndsKernel);
	if (m_perlinKernel != 0) clReleaseKernel(m_perlinKernel);
	if (m_applyHeightKernel != 0) clReleaseKernel(m_applyHeightKernel);

	if (m_VertexBuffer != 0) clReleaseMemObject(m_VertexBuffer);
	if (m_IndexBuffer != 0) clReleaseMemObject(m_IndexBuffer);
	if (m_perlinData != 0) clReleaseMemObject(m_perlinData);

	if (m_profEvent != 0) clReleaseEvent(m_profEvent);
}

//
// Output an error to the console, and then call clean up.
//
void OpenCL::CleanUp(string p_errorString)
{
	ShowError(p_errorString);

	CleanUp();
}


void OpenCL::ShowError(stringstream* p_output)
{
	MessageBox(NULL, (p_output->str()).c_str(), "Output", MB_OK);
}


void OpenCL::ShowError(string p_output)
{
	MessageBox(NULL, p_output.c_str(), "Output", MB_OK);
}

//
// Read and ouput the profiling data from given cl_event
//
void OpenCL::Profile(cl_event p_event, string p_output, double* p_time)
{
	cl_ulong start, end;

	m_errorCode = clWaitForEvents(1, &p_event);
	m_errorCode = clGetEventProfilingInfo(p_event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
	m_errorCode = clGetEventProfilingInfo(p_event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);

	*p_time = (end - start)/1000; //microseonds

	cout << p_output << ": \t" << *p_time << " microseconds" << endl;
}