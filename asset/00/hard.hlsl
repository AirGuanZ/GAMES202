#include "./mesh_common.hlsl"

cbuffer PSShadowMap
{
    float4x4 ShadowMapViewProj;
}

SamplerState ShadowMapSampler;

Texture2D<float> ShadowMap;

float calcShadowFactor(PSInput input)
{
    float3 shadowCoord = calcShadowCoord(ShadowMapViewProj, input);
    float shadowDepth =
        ShadowMap.SampleLevel(ShadowMapSampler, shadowCoord.xy, 0);
    return shadowCoord.z <= shadowDepth ? 1 : 0;
}
