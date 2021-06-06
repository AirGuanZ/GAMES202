#include "./float.hlsl"
#include "./octnor.hlsl"

cbuffer VSTransform
{
    float4x4 WVP;
    float4x4 WorldView;
    float4x4 World;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal   : WORLD_NORMAL;
    float3 worldTangent  : WORLD_TANGENT;
    float  viewDepth     : VIEW_DEPTH;
    float2 texCoord      : TEXCOORD;
};

VSOutput VSMain(VSInput input)
{
    float4 viewPos = mul(float4(input.position, 1), WorldView);

    VSOutput output;
    output.position      = mul(float4(input.position, 1), WVP);
    output.worldPosition = mul(float4(input.position, 1), World).xyz;
    output.worldNormal   = normalize(mul(float4(input.normal, 0), World).xyz);
    output.worldTangent  = normalize(mul(float4(input.tangent, 0), World).xyz);
    output.viewDepth     = viewPos.z / viewPos.w;
    output.texCoord      = input.texCoord;

    return output;
}

struct PSOutput
{
    float4 output0 : SV_TARGET0;
    float4 output1 : SV_TARGET1;
};

Texture2D<float3> Albedo;
Texture2D<float3> Normal;

SamplerState LinearSampler;

PSOutput PSMain(VSOutput input)
{
    float3 worldNor = normalize(input.worldNormal);
    float3 worldTgn = normalize(input.worldTangent);
    float3 worldBno = normalize(cross(worldNor, worldTgn));

    float3 albedo = pow(Albedo.Sample(LinearSampler, input.texCoord), 2.2);

    float3 ln = normalize(2 * Normal.Sample(LinearSampler, input.texCoord) - 1);
    float3 gn = ln.x * worldTgn + ln.y * worldBno + ln.z * worldNor;
    float2 octNor = encodeNormal(normalize(gn));
    
    float3 color  = albedo / 3.14159265;
    float  color1 = packFloats(color.r, color.b);
    float  color2 = color.g;

    PSOutput output;
    output.output0 = float4(input.worldPosition, octNor.x);
    output.output1 = float4(color1, color2, input.viewDepth, octNor.y);
    return output;
}
