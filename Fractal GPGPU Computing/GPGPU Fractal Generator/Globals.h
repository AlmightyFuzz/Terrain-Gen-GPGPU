#pragma once


const int GRID_WIDTH	= 128*4;
const int GRID_HEIGHT	= 128*4;
const int TOTAL_GRID_POINTS = GRID_WIDTH * GRID_HEIGHT;
const int NUM_INDICES	= (2 * (GRID_WIDTH - 1) * (GRID_HEIGHT - 1)) * 3; //sizeof * numTriangles * (3 indices per triangles)

enum ConstantBuffer
{
	CB_Application,
	CB_Frame,
	CB_Object,
	NumConstantBuffers
};