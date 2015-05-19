#include "GPGPUFractalGeneratorPCH.h"

#include "ReferenceCode.h"

ReferenceCode::ReferenceCode()
{
	m_VBuffer = new TerrainVertex[TOTAL_GRID_POINTS];
	m_IBuffer = new int[NUM_INDICES];
	m_NoiseBuffer = new float[TOTAL_GRID_POINTS];
}

//
// Runs the various functions and times their executions.
//
void ReferenceCode::Run()
{
	using namespace std;
	using namespace std::chrono;

	auto beggining = high_resolution_clock::now();

	//Time Vertex Code
	auto start = high_resolution_clock::now();
	//VertexCode();
	auto finish = high_resolution_clock::now();
	//cout << "CPU Vertex Code: \t";
	//cout << duration_cast<microseconds>(finish - start).count() << " microseconds" << endl;
	//
	////Time Indices Code
	//start = high_resolution_clock::now();
	//IndicesCode();
	//finish = high_resolution_clock::now();
	//cout << "CPU Indices Code: \t";
	//cout << duration_cast<microseconds>(finish - start).count() << " microseconds" << endl;

	//Time Perlin Code
	start = high_resolution_clock::now();
	PerlinCode();
	finish = high_resolution_clock::now();
	cout << "CPU Perlin Noise Code: \t";
	cout << duration_cast<microseconds>(finish - start).count() << " microseconds" << endl;

	//Time Apply Height Map Code
	//start = high_resolution_clock::now();
	//HeightMapCode();
	//finish = high_resolution_clock::now();
	//cout << "CPU Height Map Code: \t";
	//cout << duration_cast<microseconds>(finish - start).count() << " microseconds" << endl;

	auto end = high_resolution_clock::now();

	//cout << "Total execution time: \t";
	//cout << duration_cast<microseconds>(end - beggining).count() << " microseconds" << endl;
}

//
// Initialises a flat grid of points.
//
void ReferenceCode::VertexCode()
{
	float cellSpacing = 1.0f;

	for (int i = 0; i < GRID_WIDTH; i++)
	{
		for (int j = 0; j < GRID_HEIGHT; j++)
		{
			int arrayIndex = (i * GRID_WIDTH) + j;

			m_VBuffer[arrayIndex].Position.x = (j * cellSpacing) + ((GRID_WIDTH - 1) * -0.5f);
			m_VBuffer[arrayIndex].Position.y = 0.0f;
			m_VBuffer[arrayIndex].Position.z = (-i * cellSpacing) + ((GRID_HEIGHT - 1) * 0.5f);
			m_VBuffer[arrayIndex].Position.w = 0.0f;

			m_VBuffer[arrayIndex].Normal.x = 0.0f;
			m_VBuffer[arrayIndex].Normal.y = 1.0f;
			m_VBuffer[arrayIndex].Normal.z = 0.0f;
			m_VBuffer[arrayIndex].Normal.w = 0.0f;
		}
	}
}

//
// Calculates the indices order for use when drawing the points to screen.
//
void ReferenceCode::IndicesCode()
{
	int k = 0;
	int m = GRID_WIDTH;
	int n = GRID_HEIGHT;

	for (int i = 0; i < m - 1; i++)
	{
		for (int j = 0; j < n - 1; j++)
		{
			m_IBuffer[k]	= i*n + j;
			m_IBuffer[k + 1] = i*n + j + 1;
			m_IBuffer[k + 2] = (i + 1)*n + j;
			m_IBuffer[k + 3] = (i + 1)*n + j;
			m_IBuffer[k + 4] = i*n + j + 1;
			m_IBuffer[k + 5] = (i + 1)*n + j + 1;

			k += 6;
		}
	}
}

//
// Applies the height map data to the grid points
//
void ReferenceCode::HeightMapCode()
{
	for (int i = 0; i < GRID_WIDTH; i++)
	{
		for (int j = 0; j < GRID_HEIGHT; j++)
		{
			int arrayIndex = (i * GRID_WIDTH) + j;

			m_VBuffer[arrayIndex].Position.y = m_NoiseBuffer[arrayIndex];
		}
	}
}


//
// The following functions and the function that computes the Perlin noise came from: https://code.google.com/p/fractalterraingeneration/downloads/list
//
float random(float max){
	int r;
	float s;

	r = rand();
	s = (float)(r & 0x7fff) / (float)0x7fff;

	return (s * max);
}

int myfloor(float value)
{
	return (value >= 0 ? (int)value : (int)value - 1);
}

float dotproduct(float grad[], float x, float y)
{
	return (grad[0] * x + grad[1] * y);
}

float lerp(float left, float right, float amount)
{
	return ((1 - amount) * left + amount * right);
}

float fade(float x)
{
	return (x*x*x*(x*(6 * x - 15) + 10)); //equatres to 6x^5 - 15x^4 + 10x^3
}

void ReferenceCode::PerlinCode()
{
	//set up some variables
	int i, j, k, x, y,
		grad11, grad12, grad21, grad22,
		octaves = 16;

	float pixel_value,
		fracX, fracY,
		noise11, noise12, noise21, noise22,
		interpolatedx1, interpolatedx2, interpolatedxy,
		amplitude, frequency,
		gain = 0.65f, lacunarity = 2.0f;//physics terms. gain affects the amplitude each octave, lacunarity affects the frequency
	
	//set up the gradient table with 8 equally distributed angles around the unit circle
	float gradients[8][2];
	for (i = 0; i<8; ++i)
	{
		gradients[i][0] = cos(0.785398163f * (float)i);// 0.785398163 is PI/4.
		gradients[i][1] = sin(0.785398163f * (float)i);
	}

	//set up the random numbers table
	int permutations[(GRID_HEIGHT >= GRID_WIDTH ? GRID_HEIGHT : GRID_WIDTH)]; //make it as long as the largest dimension
	
	int num = GRID_HEIGHT >= GRID_WIDTH ? GRID_HEIGHT : GRID_WIDTH;
	for (i = 0; i < num; ++i)
		permutations[i] = i;//put each number in once

	//randomize the random numbers table
	for (i = 0; i<num; ++i)
	{
		j = (int)random(num);
		k = permutations[i];
		permutations[i] = permutations[j];
		permutations[j] = k;
	}

	//for each pixel...
	for (i = 0; i<GRID_WIDTH; ++i)
	{
		for (j = 0; j<GRID_WIDTH; ++j)
		{
			//get the value for this pixel by adding successive layers
			amplitude = 10.0f;
			frequency = 1.0f / (float)GRID_WIDTH;
			pixel_value = 0.0f;

			for (k = 0; k < octaves; ++k)
			{
				//	first, get the surrounding grid-points
				x = myfloor((float)j * frequency);
				y = myfloor((float)i * frequency);

				//	get the fractional parts
				fracX = (float)j * frequency - (float)x;
				fracY = (float)i * frequency - (float)y;

				//	get the gradients for the four surrounding points (or at least indexes for them)
				grad11 = permutations[(x + permutations[y & 255]) & 255] & 7;
				grad12 = permutations[(x + 1 + permutations[y & 255]) & 255] & 7;
				grad21 = permutations[(x + permutations[(y + 1) & 255]) & 255] & 7;
				grad22 = permutations[(x + 1 + permutations[(y + 1) & 255]) & 255] & 7;//&'ing caps the numbers inclusively

				//	get the noise from each corner
				noise11 = dotproduct(gradients[grad11], fracX, fracY);
				noise12 = dotproduct(gradients[grad12], fracX - 1.0f, fracY);
				noise21 = dotproduct(gradients[grad21], fracX, fracY - 1.0f);
				noise22 = dotproduct(gradients[grad22], fracX - 1.0f, fracY - 1.0f);

				//	get the fade or interpolation values
				fracX = fade(fracX);
				fracY = fade(fracY);

				//	now interpolate on the x axis
				interpolatedx1 = lerp(noise11, noise12, fracX);
				interpolatedx2 = lerp(noise21, noise22, fracX);

				//	now interpolate along the y axis
				interpolatedxy = lerp(interpolatedx1, interpolatedx2, fracY);

				//	finally, add it in and adjust for the next layer
				pixel_value += interpolatedxy * amplitude;
				amplitude *= gain;
				frequency *= lacunarity;
			}

			int arrayIndex = (i * GRID_WIDTH) + j;

			//put it in the map
			m_NoiseBuffer[arrayIndex] = pixel_value;
		}
	}
}

void ReferenceCode::Cleanup()
{
	if (m_IBuffer) delete m_IBuffer;
	if (m_NoiseBuffer) delete m_NoiseBuffer;
	if (m_VBuffer) delete m_VBuffer;
}