#include "./indirect.hlsl"
#include "./light.hlsl"
#include "./octnor.hlsl"

Texture2D<float4> GBufferA;
Texture2D<float4> GBufferB;

Texture2D<float4> LowResIndirect;
Texture2D<float4> LowResGBufferA;
Texture2D<float4> LowResGBufferB;

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * float2(2, -2) + float2(-1, 1), 0.5, 1);
    return output;
}

bool isLowResPixelGood(
    int x, int y, float3 idealNor, float3 idealColor, out float3 indirect)
{
    int width, height;
    LowResGBufferA.GetDimensions(width, height);

    float2 uv = float2(
        (x + 0.5) / width,
        (y + 0.5) / height);

    float4 sam = LowResIndirect.SampleLevel(RSMSampler, uv, 0);
    if(sam.w == 0)
        return false;

    float4 gbufferA = LowResGBufferA.SampleLevel(RSMSampler, uv, 0);
    float4 gbufferB = LowResGBufferB.SampleLevel(RSMSampler, uv, 0);

    float3 nor   = decodeNormal(float2(gbufferA.w, gbufferB.w));
    float3 color = gbufferB.xyz;
    
    if(dot(nor, idealNor) < 0.8)
        return false;

    if(any(color - idealColor > 0.1))
        return false;

    indirect = sam.xyz;
    return true;
}

float3 getIndirect(float2 uv, float3 color, float3 worldPos, float3 worldNor)
{
    if(!EnableIndirect)
        return float3(0, 0, 0);

    int width, height;
    LowResGBufferA.GetDimensions(width, height);

    int bx = int(ceil(uv.x * width  - 0.5));
    int by = int(ceil(uv.y * height - 0.5));
    int ax = bx - 1;
    int ay = by - 1;

    float tbx = uv.x * width  - (ax + 0.5);
    float tby = uv.y * height - (ay + 0.5);
    float tax = 1 - tbx;
    float tay = 1 - tby;

    float3 sum = float3(0, 0, 0), indirect;
    float sumWeight = 0;
    int sumCount = 0;

    if(isLowResPixelGood(ax, ay, worldNor, color, indirect))
    {
        sumWeight += tax * tay;
        sum       += indirect * tax * tay;
        sumCount  += 1;
    }

    if(isLowResPixelGood(ax, by, worldNor, color, indirect))
    {
        sumWeight += tax * tby;
        sum       += indirect * tax * tby;
        sumCount  += 1;
    }

    if(isLowResPixelGood(bx, ay, worldNor, color, indirect))
    {
        sumWeight += tbx * tay;
        sum       += indirect * tbx * tay;
        sumCount  += 1;
    }

    if(isLowResPixelGood(bx, by, worldNor, color, indirect))
    {
        sumWeight += tbx * tby;
        sum       += indirect * tbx * tby;
        sumCount  += 1;
    }

    if(sumCount >= 3)
        return sum / sumWeight;
    
    return estimateIndirect(worldToRSMClip(worldPos), worldPos, worldNor);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 gbufferA = GBufferA.SampleLevel(RSMSampler, input.texCoord, 0);
    if(gbufferA.w > 50)
        discard;
    float4 gbufferB = GBufferB.SampleLevel(RSMSampler, input.texCoord, 0);

    float3 worldPos = gbufferA.xyz;
    float3 worldNor = decodeNormal(float2(gbufferA.w, gbufferB.w));
    float3 color    = gbufferB.xyz;

    // diffuse

    float lightFactor = max(0.0, dot(-worldNor, LightDirection));

    // indirect

    float3 indirect = getIndirect(input.texCoord, color, worldPos, worldNor);

    float3 result =
        LightRadiance * color * (lightFactor + indirect);
    return float4(pow(result, 1 / 2.2), 1);
}
