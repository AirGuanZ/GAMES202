#include "./float.hlsl"

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

cbuffer PSParams
{
    int   EnableDirect;
    int   EnableIndirect;
    int   EnableIndirectColor;
    float Exposure;
}

Texture2D<float4> Direct;
Texture2D<float4> Indirect;

Texture2D<float4> GBufferB;

SamplerState PointSampler;

#define A 2.51
#define B 0.03
#define C 2.43
#define D 0.59
#define E 0.14

float3 tonemap(float3 x)
{
    float3 v = x * Exposure;
    return (v * (A * v + B)) / (v * (C * v + D) + E);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    if(EnableDirect && !EnableIndirect)
    {
        float4 direct = Direct.SampleLevel(PointSampler, input.texCoord, 0);
        if(direct.w == 0)
            discard;
        return float4(pow(tonemap(direct.xyz), 1 / 2.2), 1);
    }

    float4 gb = GBufferB.Sample(PointSampler, input.texCoord, 0);
    
    float colorR = 0, colorB = 0;
    unpackFloat(gb.x, colorR, colorB);
    float3 color = float3(colorR, gb.y, colorB);

    if(!EnableIndirectColor)
        color = float3(1, 1, 1);

    if(!EnableDirect && EnableIndirect)
    {
        float4 indirect = Indirect.SampleLevel(PointSampler, input.texCoord, 0);
        if(indirect.w == 0)
            discard;
        return float4(pow(tonemap(color * indirect.xyz), 1 / 2.2), 1);
    }

    float4 direct   = Direct  .SampleLevel(PointSampler, input.texCoord, 0);
    float4 indirect = Indirect.SampleLevel(PointSampler, input.texCoord, 0);
    if(direct.w == 0)
        discard;
    return float4(pow(tonemap(direct.xyz + color * indirect.xyz), 1 / 2.2), 1);
}
