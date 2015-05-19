struct PixelShaderInput
{
    float4 colour : COLOR;
};

float4 PxlShader( PixelShaderInput IN ) : SV_TARGET
{
    return IN.colour;
}