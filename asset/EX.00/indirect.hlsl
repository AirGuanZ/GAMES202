#ifndef INDIRECT_HLSL
#define INDIRECT_HLSL

cbuffer IndirectParams
{
    float4x4 RSMLightVP;

    int   PoissonDiskSampleCount;
    float IndirectSampleRadius;
    float LightProjWorldArea;
    int   EnableIndirect;
}

Texture2D<float4> RSMBufferA;
Texture2D<float4> RSMBufferB;
Texture2D<float4> RSMBufferC;

SamplerState RSMSampler;

StructuredBuffer<float3> PoissonDiskSamples;

float4 worldToRSMClip(float3 worldPos)
{
    return mul(float4(worldPos, 1), RSMLightVP);
}

float getViewDepthFromRSMBuffer(float4 rsmClipPos)
{
    float2 rsmPos = 0.5 + 0.5 * rsmClipPos.xy / rsmClipPos.w;
    if(any(saturate(rsmPos) != rsmPos))
        return 1;
    return RSMBufferA.SampleLevel(
        RSMSampler, float2(rsmPos.x, 1 - rsmPos.y), 0).w;
}

float3 estimateIndirect(
    float4 rsmClipPos, float3 position, float3 normal)
{
    float2 rsmCenter = 0.5 + 0.5 * rsmClipPos.xy / rsmClipPos.w;
    rsmCenter.y = 1 - rsmCenter.y;
    
    float3 sum = float3(0, 0, 0);

    for(int i = 0; i < PoissonDiskSampleCount; ++i)
    {
        float3 sam    = PoissonDiskSamples[i];
        float2 rsmPos = rsmCenter + sam.xy;
        float  weight = sam.z;

        if(all(saturate(rsmPos) == rsmPos))
        {
            float3 worldPos = RSMBufferA.SampleLevel(RSMSampler, rsmPos, 0).xyz;
            float3 worldNor = RSMBufferB.SampleLevel(RSMSampler, rsmPos, 0).xyz;
            float3 flux     = RSMBufferC.SampleLevel(RSMSampler, rsmPos, 0).xyz;

            float3 diff = worldPos - position;
            float  dis2 = dot(diff, diff);

            sum += weight * flux / max(dis2 * dis2, 0.001) *
                   max(0.0, dot(normal, diff)) *
                   max(0.0, dot(worldNor, -diff));
        }
    }

    float areaRatio = 3.14159265 * IndirectSampleRadius * IndirectSampleRadius;
    return areaRatio * LightProjWorldArea * sum;
}

#endif // #ifndef INDIRECT_HLSL
