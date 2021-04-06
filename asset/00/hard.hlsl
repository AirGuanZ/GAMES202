#include "./mesh_common.hlsl"

SamplerState ShadowMapSampler;

Texture2D<float> ShadowMap;

float calcShadowFactor(PSInput input)
{
    float3 shadowCoord = calcShadowCoord(input.lightPosition);
    float shadowDepth =
        ShadowMap.SampleLevel(ShadowMapSampler, shadowCoord.xy, 0);
    return shadowCoord.z <= shadowDepth ? 1 : 0;
}
