#include "./mesh_common.hlsl"

#define PI2 (3.1415926 * 3.1415926)

cbuffer PSShadowMap
{
    int   ShadowSampleCount;
    int   BlockSearchSampleCount;
    float LightNearPlane;
    float LightRadius;
}

SamplerState ShadowMapSampler;

Texture2D<float> ShadowMap;

float rand(float3 co)
{
    return frac(sin(dot(co.xy * co.z, float2(12.9898, 78.233))) * 43758.5453);
}

float findBlockerDepth(float2 shadowUV, float zReceiver, float rnd)
{
    float searchRadius = LightRadius * (zReceiver - LightNearPlane) / zReceiver;

    float poissonAngleStep  = PI2 * 10 / float(BlockSearchSampleCount);
    float poissonAngle      = rnd * PI2;
    float poissonRadius     = 1.0 / float(BlockSearchSampleCount);
    float poissonRadiusStep = poissonRadius;

    float sumBlockDepth = 0; int blockCnt = 0;
    for(int i = 0; i < BlockSearchSampleCount; ++i)
    {
        float2 offset = float2(cos(poissonAngle), sin(poissonAngle))
                      * pow(poissonRadius, 0.75);
        poissonRadius += poissonRadiusStep;
        poissonAngle  += poissonAngleStep;

        float2 uv = shadowUV + offset * searchRadius;
        float smZ = ShadowMap.SampleLevel(ShadowMapSampler, uv, 0);
        if(smZ < zReceiver)
        {
            sumBlockDepth += smZ;
            ++blockCnt;
        }
    }
    
    return blockCnt ? sumBlockDepth / blockCnt : -1;
}

float calcShadowFactor(PSInput input)
{
    float3 shadowCoord = calcShadowCoord(input.lightPosition);
    
    float zBlocker = findBlockerDepth(
        shadowCoord.xy, shadowCoord.z, rand(input.worldPosition + 1));
    if(zBlocker < 0)
        return 1;

    float PCFRadiusRatio = (shadowCoord.z - zBlocker) / zBlocker;
    float PCFRadius = LightRadius * PCFRadiusRatio
                    * LightNearPlane / shadowCoord.z;

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

        float smZ = ShadowMap.SampleLevel(
            ShadowMapSampler, shadowCoord.xy + offset * PCFRadius, 0);
        if(smZ >= shadowCoord.z)
            sum = sum + 1;
    }

    return sum / ShadowSampleCount;
}
