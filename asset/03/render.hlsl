#include "brdf.hlsl"

cbuffer VSTransform
{
    float4x4 WVP;
    float4x4 World;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal   : WORLD_NORMAL;
};

VSOutput VSMain(VSInput input)
{
    float4 pos = float4(input.position, 1);

    VSOutput output;
    output.position      = mul(pos, WVP);
    output.worldPosition = mul(pos, World).xyz;
    output.worldNormal   = normalize(mul(float4(input.normal, 0), World).xyz);
    return output;
}

cbuffer PSParams
{
    float3 R0;             float Roughness;
    float3 Eye;            float EdgeTintR;
    float3 LightDirection; float EdgeTintG;
    float3 LightIntensity; float EdgeTintB;
}

Texture2D<float> EMiu;
Texture2D<float> EAvg;

SamplerState ESampler;

float3 averageFresnel(float3 r, float3 g)
{
    return 0.087237 + 0.0230685 * g - 0.0864902 * g * g + 0.0774594 * g * g * g
           + 0.782654 * r - 0.136432 * r * r + 0.278708 * r * r * r
           + 0.19744 * g * r + 0.0360605 * g * g * r - 0.2586 * g * r * r;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 normal = normalize(input.worldNormal);

    float3 wi = -LightDirection;
    float3 wo = normalize(Eye - input.worldPosition);

    float cosThetaI = dot(wi, normal);
    float cosThetaO = dot(wo, normal);
    if(cosThetaI <= 0 || cosThetaO <= 0)
        return float4(0, 0, 0, 1);

    float3 wh = normalize(wi + wo);
    float cosThetaIH = dot(wh, wi);
    float3 F = fresnel(R0, cosThetaIH);

    float cosThetaH = dot(wh, normal);
    float D = ggx(cosThetaH, Roughness);

    float G = ggxSmith(cosThetaI, cosThetaO, Roughness);

    float3 brdf = F * D * G / (4 * cosThetaI * cosThetaO);

    float EMiuI = EMiu.SampleLevel(ESampler, float2(cosThetaI, Roughness), 0);
    float EMiuO = EMiu.SampleLevel(ESampler, float2(cosThetaO, Roughness), 0);
    float EavgV = EAvg.SampleLevel(ESampler, float2(Roughness, 0.5f), 0);

    float Fms = (1 - EMiuI) * (1 - EMiuO) / (3.14159265f * (1 - EavgV));

    float3 edgeTint = float3(EdgeTintR, EdgeTintG, EdgeTintB);
    float3 Favg = averageFresnel(R0, edgeTint);
    float3 Fadd = Favg * EavgV / (1 - Favg * (1 - EavgV));
    if(EdgeTintR < 0)
        Fadd = float3(0, 0, 0);

    float3 result = (brdf + Fadd * Fms) * cosThetaI * LightIntensity;
    return float4(pow(result, 1 / 2.2f), 1);
}
