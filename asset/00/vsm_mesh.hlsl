#include "./mesh_common.hlsl"

SamplerState ShadowMapSampler;

Texture2D<float4> ShadowMap;

float calcShadowFactor(PSInput input)
{
    float3 shadowCoord = calcShadowCoord(input.lightPosition);
    float2 smValue =
        ShadowMap.SampleLevel(ShadowMapSampler, shadowCoord.xy, 0).xy;
    
    float miu    = smValue.x;
    float sigma2 = smValue.y - miu * miu;

    float zDiff = shadowCoord.z - miu;
    float slf   = zDiff <= 0 ? 1 : 0;
    float vsf   = sigma2 / (sigma2 + zDiff * zDiff);

    return max(slf, vsf);
}
