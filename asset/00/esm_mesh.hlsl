#include "./mesh_common.hlsl"

cbuffer PSParam
{
    float ShadowC;
}

SamplerState ShadowMapSampler;

Texture2D<float4> ShadowMap;

float calcShadowFactor(PSInput input)
{
    float3 shadowCoord = calcShadowCoord(input.lightPosition);
    float NDCz = shadowCoord.z / input.lightPosition.w;
    float smValue =
        ShadowMap.SampleLevel(ShadowMapSampler, shadowCoord.xy, 0).x;
    float shadow = smValue * exp(-ShadowC * NDCz);
    return saturate(shadow);
}
