#define THREAD_GROUP_SIZE_X 16
#define THREAD_GROUP_SIZE_Y 16

cbuffer CSParams
{
    float LastWidth;
    float LastHeight;
    float ThisWidth;
    float ThisHeight;
}

Texture2D<float> LastLevel;

RWTexture2D<float> ThisLevel;

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    if(threadIdx.x >= ThisWidth || threadIdx.y >= ThisHeight)
        return;

    float2 uvMin = float2(
        float(threadIdx.x) / ThisWidth,
        float(threadIdx.y) / ThisHeight);

    float2 uvMax = float2(
        float(threadIdx.x + 1) / ThisWidth,
        float(threadIdx.y + 1) / ThisHeight);

    int xBeg = int(floor(uvMin.x * LastWidth));
    int yBeg = int(floor(uvMin.y * LastHeight));

    int xEnd = int(ceil(uvMax.x * LastWidth));
    int yEnd = int(ceil(uvMax.y * LastHeight));

    float depth = 1.0f / 0.0f;
    for(int x = xBeg; x < xEnd; ++x)
    {
        for(int y = yBeg; y < yEnd; ++y)
        {
            float newDepth = LastLevel.Load(int3(x, y, 0));
            if(newDepth > 0)
                depth = min(depth, newDepth);
        }
    }

    ThisLevel[threadIdx.xy] = depth;
}
