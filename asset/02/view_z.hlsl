#define THREAD_GROUP_SIZE_X 16
#define THREAD_GROUP_SIZE_Y 16

cbuffer CSParams
{
    float LastWidth;
    float LastHeight;
    float ThisWidth;
    float ThisHeight;
}

Texture2D<float4> Src;

RWTexture2D<float> Dst;

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    if(threadIdx.x < ThisWidth && threadIdx.y < ThisHeight)
    {
        float4 src = Src[threadIdx.xy];
        Dst[threadIdx.xy] = src.z > 0 ? src.z : 1.0f / 0.0f;
    }
}
