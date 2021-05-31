#include "./octnor.hlsl"

cbuffer VSTransform
{
    float4x4 WVP;
    float4x4 World;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 albedo   : ALBEDO;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal   : WORLD_NORMAL;
    float3 color         : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position      = mul(float4(input.position, 1), WVP);
    output.worldPosition = mul(float4(input.position, 1), World).xyz;
    output.worldNormal   = normalize(mul(float4(input.normal, 0), World).xyz);
    output.color         = input.albedo / 3.14159265;
    return output;
}

struct PSOutput
{
    float4 output0 : SV_TARGET0;
    float4 output1 : SV_TARGET1;
};

PSOutput PSMain(VSOutput input)
{
    float2 octNormal = encodeNormal(normalize(input.worldNormal));

    PSOutput output;
    output.output0 = float4(input.worldPosition, octNormal.x);
    output.output1 = float4(input.color,         octNormal.y);
    return output;
}
