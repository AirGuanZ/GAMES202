#include "./float.hlsl"
#include "./octnor.hlsl"

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

cbuffer PSLight
{
    float3   LightDirection; float LightPad0;
    float3   LightRadiance;  float LightPad1;
    float4x4 LightViewProj;
}

Texture2D<float> ShadowMap;
Texture2D<float4> GBufferA;
Texture2D<float4> GBufferB;

SamplerState PointSampler;

float4 PSMain(VSOutput input) : SV_TARGET
{
    // read gbuffer

    float4 gbufferA = GBufferA.SampleLevel(PointSampler, input.texCoord, 0);
    if(gbufferA.w > 50)
        discard;
    float4 gbufferB = GBufferB.SampleLevel(PointSampler, input.texCoord, 0);

    float3 position = gbufferA.xyz;
    float2 color12  = gbufferB.xy;
    float3 normal   = decodeNormal(float2(gbufferA.w, gbufferB.w));

    float colorR = 0, colorB = 0;
    unpackFloat(color12.x, colorR, colorB);
    float3 color = float3(colorR, color12.y, colorB);

    // shadow

    float4 lightClip   = mul(float4(position + 0.02 * normal, 1), LightViewProj);
    float  lightDepth  = lightClip.z;
    float2 shadowNDC   = lightClip.xy / lightClip.w;
    float2 shadowMapUV = float2(0.5, 0.5) + float2(0.5, -0.5) * shadowNDC;

    float shadowMapDepth = ShadowMap.SampleLevel(PointSampler, shadowMapUV, 0);
    float shadowFactor   = shadowMapDepth >= lightDepth ? 1 : 0;

    // light

    float lightFactor = max(0, dot(normal, -LightDirection));

    // result

    float3 result =
        LightRadiance * color * shadowFactor * lightFactor;
    return float4(result, 1);
}
