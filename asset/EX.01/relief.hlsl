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
    float3 worldPosition : WORLD_POSITION;
    float3 worldNormal   : WORLD_NORMAL;
    float3 worldTangent  : WORLD_TANGENT;
    float2 texCoord      : TEXCOORD;
    float  dTangent      : DTANGENT;
    float  dBinormal     : DBINORMAL;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position      = mul(float4(input.position, 1), WVP);
    output.worldPosition = mul(float4(input.position, 1), World).xyz;
    output.worldNormal   = normalize(mul(float4(input.normal, 0), World).xyz);
    output.worldTangent  = normalize(mul(float4(input.tangent, 0), World).xyz);
    output.texCoord      = input.texCoord;
    output.dTangent      = input.dTangent;
    output.dBinormal     = input.dBinormal;
    return output;
}

cbuffer PSPerFrame
{
    float3 LightRadiance;
    float3 LightDirection;
    float3 LightAmbient;

    float3 Eye;
    float  DiffStep;

    float LinearStep;
    int   MaxLinearStepCount;
    int   BinaryStepCount;
}

cbuffer PSHeight
{
    float HeightScale;
}

Texture2D<float>  Height;
Texture2D<float3> Albedo;

SamplerState PointSampler;
SamplerState LinearSampler;

float2 trace(VSOutput input, float3 ori, float3 dir)
{
    float  y  = ori.y;
    float2 uv = ori.xz / float2(input.dTangent, input.dBinormal);
    float2 p  = input.texCoord + uv;

    float uvLen = length(uv);
    if(uvLen < 0.0001)
        return p;

    // linear search

    float  lastY = y;
    float2 lastP = p;

    float  dY = -LinearStep * HeightScale / uvLen;
    float2 dP = LinearStep * normalize(dir.xz);

    for(int i = 0; i < MaxLinearStepCount; ++i)
    {
        if(all(saturate(p) == p))
        {
            float height = HeightScale * Height.SampleLevel(LinearSampler, p, 0);
            if(y <= height)
                break;
        }
        lastY = y;
        lastP = p;
        y += dY;
        p += dP;
    }

    // binary search

    for(int i = 0; i < BinaryStepCount; ++i)
    {
        float  midY = 0.5 * (lastY + y);
        float2 midP = 0.5 * (lastP + p);

        if(any(saturate(midP) != midP))
        {
            y = midY;
            p = midP;
            continue;
        }

        float height = HeightScale * Height.SampleLevel(LinearSampler, midP, 0);
        if(midY <= height)
        {
            y = midY;
            p = midP;
        }
        else
        {
            lastY = midY;
            lastP = midP;
        }
    }

    return 0.5 * (lastP + p);
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    int hWidth = 0, hHeight = 0;
    Height.GetDimensions(hWidth, hHeight);

    float2 hSize = float2(hWidth, hHeight);
    float2 dxy   = float2(input.dTangent, input.dBinormal) / hSize;

    float3 position = input.worldPosition;

    float3 normal   = normalize(input.worldNormal);
    float3 tangent  = normalize(input.worldTangent);
    float3 binormal = normalize(cross(tangent, normal));

    // adjust uv

    float3x3 globalToLocal = transpose(float3x3(tangent, normal, binormal));

    float3 dir      = normalize(mul(position - Eye, globalToLocal));
    float3 ori      = float3(HeightScale / dir.y * dir.xz, HeightScale).xzy;
    float2 tracedUV = trace(input, ori, dir);

    // adjust normal

    normal = heightToNormal(
            DiffStep, HeightScale, Height, PointSampler,
            normal, binormal, tangent, hSize, dxy, tracedUV);

    // read diffuse coefficient

    float3 albedo = pow(Albedo.SampleLevel(LinearSampler, tracedUV, 0), 2.2);
    float3 color  = albedo / 3.14159;

    // final lighting

    float lightFactor = max(dot(-LightDirection, normal), 0);
    float3 result = color * (LightRadiance * lightFactor + LightAmbient);
    return float4(pow(result, 1 / 2.2), 1);
}
