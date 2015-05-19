#pragma once

#include "GPGPUFractalGeneratorPCH.h"
#include "Globals.h"

//
// This class is used to time the code as it is ru on the CPU.
//
class ReferenceCode
{
private:

	struct TerrainVertex
	{
		cl_float4 Position;
		cl_float4 Normal;
	};

	TerrainVertex* m_VBuffer;
	int* m_IBuffer;
	float* m_NoiseBuffer;

	void VertexCode();
	void IndicesCode();
	void PerlinCode();
	void HeightMapCode();

public:
	ReferenceCode();
	void Run();
	void Cleanup();
};