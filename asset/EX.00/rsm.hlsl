#include "./light.hlsl"

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
    float3 albedo        : ALBEDO;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position      = mul(float4(input.position, 1), WVP);
    output.worldPosition = mul(float4(input.position, 1), World).xyz;
    output.worldNormal   = normalize(mul(float4(input.normal, 0), World).xyz);
    output.albedo        = input.albedo;
    return output;
}

struct PSOutput
{
    float4 output0 : SV_TARGET0;
    float4 output1 : SV_TARGET1;
    float4 output2 : SV_TARGET2;
};

PSOutput PSMain(VSOutput input)
{
    PSOutput output;
    output.output0 = float4(input.worldPosition, 1);
    output.output1 = float4(normalize(input.worldNormal), 1);
    output.output2 = float4(LightRadiance * input.albedo, 1);
    return output;
}
