#include "./indirect.hlsl"
#include "./octnor.hlsl"

Texture2D<float4> GBufferA;
Texture2D<float4> GBufferB;

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

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 gbufferA = GBufferA.SampleLevel(RSMSampler, input.texCoord, 0);
    if(gbufferA.w > 50)
        discard;
    float4 gbufferB = GBufferB.SampleLevel(RSMSampler, input.texCoord, 0);

    float3 worldPos = gbufferA.xyz;
    float3 worldNor = decodeNormal(float2(gbufferA.w, gbufferB.w));
    
    float4 rsmClipPos = worldToRSMClip(worldPos);
    float3 indirect = estimateIndirect(rsmClipPos, worldPos, worldNor);
    
    return float4(indirect, 1);
}
