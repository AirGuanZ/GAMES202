// VS

cbuffer VSTransform
{
    float4x4 WVP;
};

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float  depth    : DEPTH;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1), WVP);
    output.depth    = output.position.z;
    return output;
}

// PS

float4 PSMain(VSOutput input) : SV_TARGET
{
    return float4(input.depth, input.depth * input.depth, 0, 0);
}
