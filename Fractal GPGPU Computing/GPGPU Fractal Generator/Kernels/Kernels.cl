//
// Vertex description for use in kernels
//
typedef struct TerrainVertex
{
	float4 Position;
	float4 Normal;
} Vertex;

//
// Initialises the flat grid
//
__kernel void InitVertBuffer(
	global Vertex* p_VBuffer,
	private int p_gridWidth,
	private int p_gridLength
	//, global Vertex* debugBuff
	)
{
	float i = get_global_id(0);
	float j = get_global_id(1);
	float cellSpacing = 1.0f;

	int arrayIndex = (i * p_gridWidth) + j;

	//Calculate vertex's x and z postion
	p_VBuffer[arrayIndex].Position.x = (j * cellSpacing) + ((p_gridWidth - 1) * -0.5f);
	p_VBuffer[arrayIndex].Position.y = 0.0f;
	p_VBuffer[arrayIndex].Position.z = (-i * cellSpacing) + ((p_gridLength - 1) * 0.5f);
	p_VBuffer[arrayIndex].Position.w = 0.0f;
	
	p_VBuffer[arrayIndex].Normal = (float4)(0.0f, 1.0f, 0.0f, 0.0f);

	//debugBuff[arrayIndex] = p_VBuffer[arrayIndex];
}

//
// Calculates the index order.
//
__kernel void InitIndexBuffer(
	global int* p_IBuffer,
	private int p_width
	//, global int* debug
	)
{
	int i = get_global_id(0);
	int j = get_global_id(1);
	int k = get_global_id(2);

	//Calculate array index
	int arrayIndex = ((i * (p_width-1) + j) *6) + k;

	if (k == 0)
		p_IBuffer[arrayIndex] = i * p_width + j;

	if (k == 1 || k == 4)
		p_IBuffer[arrayIndex] = i * p_width + j + 1;

	if (k == 2 || k == 3)
		p_IBuffer[arrayIndex] = (i + 1) * p_width + j;
	
	if (k == 5)
		p_IBuffer[arrayIndex] = (i + 1) * p_width + j + 1;

	//debug[arrayIndex] = p_IBuffer[arrayIndex];
}

float dotproduct(float2 grad, float x, float y)
{
	return (grad.x * x + grad.y * y);
}

float lerp(float left, float right, float amount)
{
	return ((1 - amount) * left + amount * right);
}

float fade(float x)
{
	return (x*x*x*(x*(6 * x - 15) + 10)); //equatres to 6x^5 - 15x^4 + 10x^3
}

//
// Performs the Perlin Noise calculations
//
__kernel void PerlinNoise(
	global float* p_noiseData,
	global float2* p_gradients,
	global int* p_randomValues,
	private int p_gridWidth,
	private int p_octaves
	)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	int x, y, grad11, grad12, grad21, grad22;
	float fracX, fracY, noise11, noise12, noise21, noise22,
		interpolatedX1, interpolatedX2, interpolatedXY;
	float gain = 0.80f; // Alter the gain to affect how "bumpy" the final terrain will be.
	float lacunarity = 2.0f;

	float amplitude = 10.0f; // Alter amplitude to affect how large the "bumps" in the final terrain will be
	float frequency = 1.0f / (float)p_gridWidth;
	float data = 0.0f;

	for (int k = 0; k < p_octaves; k++)
	{
		x = floor((float)j * frequency);
		y = floor((float)i * frequency);
		
		fracX = (float)j * frequency - (float)x;
		fracY = (float)i * frequency - (float)y;
		
		grad11 = p_randomValues[(x + p_randomValues[y & 255]) & 255] & 7;
		grad12 = p_randomValues[(x + 1 + p_randomValues[y & 255]) & 255] & 7;
		grad21 = p_randomValues[(x + p_randomValues[(y + 1) & 255]) & 255] & 7;
		grad22 = p_randomValues[(x + 1 + p_randomValues[(y + 1) & 255]) & 255] & 7;
		
		noise11 = dotproduct(p_gradients[grad11], fracX, fracY);
		noise12 = dotproduct(p_gradients[grad12], fracX - 1.0f, fracY);
		noise21 = dotproduct(p_gradients[grad21], fracX, fracY - 1.0f);
		noise22 = dotproduct(p_gradients[grad22], fracX - 1.0f, fracY - 1.0f);
		
		fracX = fade(fracX);
		fracY = fade(fracY);
		
		interpolatedX1 = lerp(noise11, noise12, fracX);
		interpolatedX2 = lerp(noise21, noise22, fracX);
		
		interpolatedXY = lerp(interpolatedX1, interpolatedX2, fracY);
		
		data += interpolatedXY * amplitude;
		amplitude *= gain;
		frequency *= lacunarity;
	}

	int arrayIndex = (i * p_gridWidth) + j;

	p_noiseData[arrayIndex] = data;
}

//
// Applies the height map data to teh flat grid.
//
__kernel void ApplyHeightMap(
	global Vertex* p_VBuffer,
	global float* p_noiseData,
	private int p_gridWidth
	)
{
	int i = get_global_id(0);
	int j = get_global_id(1);

	int arrayIndex = (i * p_gridWidth) + j;

	p_VBuffer[arrayIndex].Position.y = p_noiseData[arrayIndex];
}