#include "./mesh_common.hlsl"

#define PI2 (3.1415926 * 3.1415926)

cbuffer PSShadowMap
{
    float ShadowMapResolution;
    float ShadowMapFilterSize;
    int   ShadowSampleCount;
    int   pad0;
}

SamplerComparisonState ShadowMapSampler;

Texture2D<float> ShadowMap;

float rand(float3 co)
{
    return frac(sin(dot(co.xy * co.z, float2(12.9898, 78.233))) * 43758.5453);
}

float calcShadowFactor(PSInput input)
{
    float3 shadowCoord = calcShadowCoord(input.lightPosition);
    
    float offsetScale    = ShadowMapFilterSize / ShadowMapResolution;

    float poissonAngleStep  = PI2 * 10 / float(ShadowSampleCount);
    float poissonAngle      = rand(input.worldPosition) * PI2;
    float poissonRadius     = 1.0 / float(ShadowSampleCount);
    float poissonRadiusStep = poissonRadius;

    float sum = 0;
    for(int i = 0; i < ShadowSampleCount; ++i)
    {
        float2 offset = float2(cos(poissonAngle), sin(poissonAngle))
                      * pow(poissonRadius, 0.75);
        poissonRadius += poissonRadiusStep;
        poissonAngle  += poissonAngleStep;
        
        sum += ShadowMap.SampleCmpLevelZero(
            ShadowMapSampler, shadowCoord.xy + offset * offsetScale, shadowCoord.z);
    }
    
    return sum / ShadowSampleCount;
}
