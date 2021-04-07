// VS

cbuffer VSTransform
{
    float4x4 WVP;
}

struct VSInput
{
    float3 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float  depth    : DEPTH;
    float  w        : W;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1), WVP);
    output.depth    = output.position.z;
    output.w        = output.position.w;
    return output;
}

// PS

cbuffer PSParam
{
    float ShadowC;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return float4(exp(ShadowC * input.depth / input.w), 0, 0, 0);
}
