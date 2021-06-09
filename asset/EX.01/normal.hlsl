#include "./dis2nor.hlsl"

cbuffer VSTransform
{
    float4x4 WVP;
    float4x4 World;
}

struct VSInput
{
    float3 position  : POSITION;
    float3 normal    : NORMAL;
    float3 tangent   : TANGENT;
    float2 texCoord  : TEXCOORD;
    float  dTangent  : DTANGENT;
    float  dBinormal : DBINORMAL;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    float3 worldNormal   : WORLD_NORMAL;
    float3 worldTangent  : WORLD_TANGENT;
    float2 texCoord      : TEXCOORD;
    float  dTangent      : DTANGENT;
    float  dBinormal     : DBINORMAL;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position     = mul(float4(input.position, 1), WVP);
    output.worldNormal  = normalize(mul(float4(input.normal, 0), World).xyz);
    output.worldTangent = normalize(mul(float4(input.tangent, 0), World).xyz);
    output.texCoord     = input.texCoord;
    output.dTangent     = input.dTangent;
    output.dBinormal    = input.dBinormal;
    return output;
}

cbuffer PSLight
{
    float3 LightRadiance;
    float3 LightDirection;

    float3 LightAmbient;
    float  PSLightPad0;

    float DiffStep;
}

cbuffer PSHeight
{
    float HeightScale;
}

Texture2D<float>  Height;
Texture2D<float3> Albedo;

SamplerState PointSampler;
SamplerState LinearSampler;

float3 computeNormal(
    float3 normal, float3 binormal, float3 tangent,
    float2 texSize, float2 dxy, float2 uv)
{
    return heightToNormal(
        DiffStep, HeightScale, Height, PointSampler,
        normal, binormal, tangent, texSize, dxy, uv);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    int heightWidth = 0, heightHeight = 0;
    Height.GetDimensions(heightWidth, heightHeight);
    
    float3 normal   = normalize(input.worldNormal);
    float3 tangent  = normalize(input.worldTangent);
    float2 texCoord = input.texCoord;
    float3 binormal = normalize(cross(tangent, normal));

    // adjust normal.
    // usually we generate and sample normal map seperately.
    // however, to bilinearly sample the normal with only
    // displacement map, we must do soft interpolation here.
    // NOTE: this can be optimized by using a precomputed normal map

    float dx    = input.dTangent  / heightWidth;
    float dy    = input.dBinormal / heightHeight;
    float2 size = float2(heightWidth, heightHeight);

    int2 b = int2(ceil(texCoord * size - 0.5));
    int2 a = b - 1;

    float2 tb = texCoord * size - (a + 0.5);
    float2 ta = 1 - tb;

    float2 uva = (a + 0.5) / size;
    float2 uvb = (b + 0.5) / size;

    float3 sum = float3(0, 0, 0);
    sum += ta.x * ta.y * computeNormal(
        normal, binormal, tangent, size, float2(dx, dy), uva);
    sum += ta.x * tb.y * computeNormal(
        normal, binormal, tangent, size, float2(dx, dy), float2(uva.x, uvb.y));
    sum += tb.x * ta.y * computeNormal(
        normal, binormal, tangent, size, float2(dx, dy), float2(uvb.x, uva.y));
    sum += tb.x * tb.y * computeNormal(
        normal, binormal, tangent, size, float2(dx, dy), uvb);

    normal = normalize(sum);

    // read diffuse coefficient

    float3 albedo = pow(Albedo.SampleLevel(LinearSampler, texCoord, 0), 2.2);
    float3 color  = albedo / 3.14159;

    // final lighting

    float lightFactor = max(dot(-LightDirection, normal), 0);
    float3 result = color * (LightRadiance * lightFactor + LightAmbient);
    return float4(pow(result, 1 / 2.2), 1);
}
