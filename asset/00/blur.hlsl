cbuffer BlurSetting
{
    int Radius;
    float Weight0;
    float Weight1;
    float Weight2;
    float Weight3;
    float Weight4;
    float Weight5;
    float Weight6;
    float Weight7;
    float Weight8;
    float Weight9;
    float Weight10;
    float Weight11;
    float Weight12;
    float Weight13;
    float Weight14;
    float Weight15;
    float Weight16;
    float Weight17;
    float Weight18;
    float Weight19;
    float Weight20;
    float Weight21;
    float Weight22;
    float Weight23;
    float Weight24;
    float Weight25;
    float Weight26;
    float Weight27;
    float Weight28;
    float Weight29;
    float Weight30;
};

Texture2D<float4>   InputImage;
RWTexture2D<float4> OutputImage;

#define MAX_RADIUS 15
#define GROUP_SIZE 256

groupshared float4 ImageCache[GROUP_SIZE + 2 * MAX_RADIUS];

struct CSInput
{
    int3 threadInGroup : SV_GroupThreadID;
    int3 thread        : SV_DispatchThreadID;
};

[numthreads(GROUP_SIZE, 1, 1)]
void HoriMain(CSInput input)
{
    // fill imageLineCache

    uint2 imageSize;
    InputImage.GetDimensions(imageSize.x, imageSize.y);

    if(input.threadInGroup.x < Radius)
    {
        int  texelX = max(0, input.thread.x - Radius);
        int2 texelXY = int2(texelX, input.thread.y);
        ImageCache[input.threadInGroup.x] = InputImage[texelXY];
    }

    if(input.threadInGroup.x >= GROUP_SIZE - Radius)
    {
        int  texelX = min(imageSize.x - 1, input.thread.x + Radius);
        int2 texelXY = int2(texelX, input.thread.y);
        ImageCache[input.threadInGroup.x + 2 * Radius] = InputImage[texelXY];
    }

    ImageCache[input.threadInGroup.x + Radius] =
        InputImage[min(input.thread.xy, imageSize.xy - 1)];

    // sync thread group

    GroupMemoryBarrierWithGroupSync();

    // blur

    float weights[31] = {
        Weight0, Weight1, Weight2, Weight3,
        Weight4, Weight5, Weight6, Weight7,
        Weight8, Weight9, Weight10, Weight11,
        Weight12, Weight13, Weight14, Weight15,
        Weight16, Weight17, Weight18, Weight19,
        Weight20, Weight21, Weight22, Weight23,
        Weight24, Weight25, Weight26, Weight27,
        Weight28, Weight29, Weight30,
    };

    float4 weightedSum = float4(0, 0, 0, 0);

    for(int offset = -Radius; offset <= Radius; ++offset)
    {
        int weightIdx = offset + Radius;
        int cacheIdx = input.threadInGroup.x + weightIdx;
        weightedSum += weights[weightIdx] * ImageCache[cacheIdx];
    }

    OutputImage[input.thread.xy] = weightedSum;
}

[numthreads(1, GROUP_SIZE, 1)]
void VertMain(CSInput input)
{
    // fill imageLineCache

    uint2 imageSize;
    InputImage.GetDimensions(imageSize.x, imageSize.y);

    if(input.threadInGroup.y < Radius)
    {
        int  texelY = max(0, input.thread.y - Radius);
        int2 texelXY = int2(input.thread.x, texelY);
        ImageCache[input.threadInGroup.y] = InputImage[texelXY];
    }

    if(input.threadInGroup.y >= GROUP_SIZE - Radius)
    {
        int  texelY = min(imageSize.y - 1, input.thread.y + Radius);
        int2 texelXY = int2(input.thread.x, texelY);
        ImageCache[input.threadInGroup.y + 2 * Radius] = InputImage[texelXY];
    }

    ImageCache[input.threadInGroup.y + Radius] =
        InputImage[min(input.thread.xy, imageSize.xy - 1)];

    // sync thread group

    GroupMemoryBarrierWithGroupSync();

    // blur

    float weights[31] = {
        Weight0, Weight1, Weight2, Weight3,
        Weight4, Weight5, Weight6, Weight7,
        Weight8, Weight9, Weight10, Weight11,
        Weight12, Weight13, Weight14, Weight15,
        Weight16, Weight17, Weight18, Weight19,
        Weight20, Weight21, Weight22, Weight23,
        Weight24, Weight25, Weight26, Weight27,
        Weight28, Weight29, Weight30,
    };

    float4 weightedSum = float4(0, 0, 0, 0);

    for(int offset = -Radius; offset <= Radius; ++offset)
    {
        int weightIdx = offset + Radius;
        int cacheIdx = input.threadInGroup.y + weightIdx;
        weightedSum += weights[weightIdx] * ImageCache[cacheIdx];
    }

    OutputImage[input.thread.xy] = weightedSum;
}
