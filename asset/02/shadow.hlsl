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
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = mul(float4(input.position, 1), WVP);
    return output;
}

// PS

void PSMain(VSOutput input)
{

}
