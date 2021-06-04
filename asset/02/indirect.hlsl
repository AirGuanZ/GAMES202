#include "./float.hlsl"
#include "./octnor.hlsl"

#define PI 3.14159265

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

cbuffer IndirectParams
{
    float4x4 View;
    float4x4 Proj;

    uint  SampleCount;
    uint  MaxTraceSteps;
    float MaxTraceLength;
    float ProjNearZ;

    float OutputWidth;
    float OutputHeight;
    float DepthThreshold;
    float IndirectParamsPad0;
}

Texture2D<float4> GBufferA;
Texture2D<float4> GBufferB;
Texture2D<float3> Direct;

SamplerState PointSampler;

StructuredBuffer<float2> RawSamples;

// ray cast in view space
bool trace(float3 ori, float3 dir, out float2 posNDC)
{
    float rayLen = (ori.z + dir.z * MaxTraceLength < ProjNearZ) ?
                   (ProjNearZ - ori.z) / dir.z : MaxTraceLength;
    float3 end = ori + rayLen * dir;

    float4 oriClip = mul(float4(ori, 1), Proj);
    float4 endClip = mul(float4(end, 1), Proj);

    float oriInvW = 1 / oriClip.w;
    float endInvW = 1 / endClip.w;

    // NDC of origin
    float2 oriNDC = oriClip.xy * oriInvW;
    // NDC extent of ray
    float2 dtaNDC = endClip.xy * endInvW - oriNDC;
    // pixel extent of ray
    float2 dtaPxl = (float2(0.5, 0.5) + float2(0.5, -0.5) * dtaNDC)
                  * float2(OutputWidth, OutputHeight);

    // step along x or y
    bool swapXY = false;
    if(abs(dtaPxl.x) < abs(dtaPxl.y))
    {
        swapXY = true;
        oriNDC = oriNDC.yx;
        dtaNDC = dtaNDC.yx;
        dtaPxl = dtaPxl.yx;
    }

    // NDC step
    float  dx = sign(dtaPxl.x) * 2 / (swapXY ? OutputHeight : OutputWidth);
    float  dy = dtaNDC.y / dtaNDC.x * dx;
    float2 dP = float2(dx, dy);

    float oriZOverW = ori.z * oriInvW;
    float endZOverW = end.z * endInvW;

    // viewZ/w step
    float dZOverW = (endZOverW - oriZOverW) / dtaNDC.x * dx;

    // 1/w step
    float dInvW   = (endInvW - oriInvW)     / dtaNDC.x * dx;

    float2 P     = oriNDC + dP;
    float zOverW = oriZOverW + dZOverW;
    float invW   = oriInvW + dInvW;

    uint maxSteps = min(MaxTraceSteps, uint(floor(abs(dtaPxl.x))));
    for(uint i = 0; i < maxSteps; ++i)
    {
        float2 NDC = swapXY ? P.yx : P;
        float2 uv  = float2(0.5, 0.5) + float2(0.5, -0.5) * NDC;
        if(any(saturate(uv) != uv))
            return false;

        float rayDepth     = zOverW / invW;
        float sampledDepth = GBufferB.SampleLevel(PointSampler, uv, 0).z;

        if(sampledDepth <= rayDepth && rayDepth <= sampledDepth + DepthThreshold)
        {
            posNDC = NDC;
            return true;
        }

        P      += dP;
        zOverW += dZOverW;
        invW   += dInvW;
    }

    return false;
}

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

    // build local frame

    float3 ori = mul(float4(position, 1), View).xyz;

    float3 local_z = normal;

    float3 local_y;
    if(abs(dot(local_z, float3(1, 0, 0))) < 0.9)
        local_y = cross(local_z, float3(1, 0, 0));
    else
        local_y = cross(local_z, float3(0, 1, 0));
    local_y = normalize(local_y);

    float3 local_x = cross(local_y, local_z);

    // estimate indirect

    float2 sampleOffset = frac(sin(dot(
        input.texCoord, float2(12.9898, 78.233) * 2.0)) * 43758.5453);

    float3 sum = float3(0, 0, 0);
    for(uint i = 0; i < SampleCount; ++i)
    {
        float2 raw = RawSamples[i];
        raw = frac(raw + sampleOffset);
        
        float z   = sqrt(1 - raw.x);
        float phi = 2 * PI * raw.y;
        float r   = sqrt(raw.x);

        float3 dir = float3(r * cos(phi), r * sin(phi), z);
        dir = dir.x * local_x + dir.y * local_y + dir.z * local_z;
        dir = normalize(mul(float4(dir, 0), View).xyz);

        float2 ndc;
        if(trace(ori, dir, ndc))
        {
            float2 uv     = float2(0.5, 0.5) + float2(0.5, -0.5) * ndc;
            float3 direct = Direct.SampleLevel(PointSampler, uv, 0);
            sum += direct * PI; // cos / pdf ==  PI
        }
    }

    return float4(color * sum / SampleCount, 1);
}
