#define THREAD_GROUP_SIZE_X 16
#define THREAD_GROUP_SIZE_Y 16

cbuffer CSParams
{
    float4x4 LastViewProj;
    float Alpha;
    float CSParamsPad0;
    float CSParamsPad1;
    float CSParamsPad2;
}

Texture2D<float4> LastGBuffer;
Texture2D<float4> ThisGBuffer;

Texture2D<float4> AccuSrc;
Texture2D<float4> NewIndirect;

RWTexture2D<float4> AccuDst;

SamplerState LinearSampler;

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    AccuDst.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    float4 newIndirect = NewIndirect[threadIdx.xy];
    if(!newIndirect.w)
    {
        AccuDst[threadIdx.xy] = float4(0, 0, 0, 0);
        return;
    }
    
    float3 worldPos = ThisGBuffer[threadIdx.xy].xyz;
    float4 lastClip = mul(float4(worldPos, 1), LastViewProj);
    float2 lastNDC  = lastClip.xy / lastClip.w;
    float2 lastUV   = float2(0.5, 0.5) + float2(0.5, -0.5) * lastNDC;

    if(any(saturate(lastUV) != lastUV))
    {
        AccuDst[threadIdx.xy] = newIndirect;
        return;
    }

    float4 accuSrc = AccuSrc    .SampleLevel(LinearSampler, lastUV, 0);
    float4 lastG   = LastGBuffer.SampleLevel(LinearSampler, lastUV, 0);

    float alpha = Alpha;
    alpha *= exp(min(distance(lastG.xyz, worldPos), 1));
    alpha = saturate(alpha);
    
    AccuDst[threadIdx.xy] = alpha * newIndirect + (1 - alpha) * accuSrc;
}
