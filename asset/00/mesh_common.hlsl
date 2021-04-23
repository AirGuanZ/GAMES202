#ifndef MESH_COMMON_HLSL
#define MESH_COMMON_HLSL

// vs

cbuffer VSTransform
{
    float4x4 World;
    float4x4 WVP;
    float4x4 LightWVP;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct VSOutput
{
    float4 position      : SV_POSITION;
    float4 lightPosition : LIGHT_POSITION;
    float3 worldPosition : POSITION;
    float3 worldNormal   : NORMAL;
};

VSOutput VSMain(VSInput input)
{
    float3 offseted_position = input.position + 0.02 * input.normal;

    VSOutput output;
    output.position      = mul(float4(input.position, 1), WVP);
    output.lightPosition = mul(float4(offseted_position, 1), LightWVP);
    output.worldPosition = mul(float4(input.position, 1), World).xyz;
    output.worldNormal   = normalize(mul(float4(input.normal, 0), World).xyz);
    return output;
}

// ps

struct PSInput
{
    float4 position      : SV_POSITION;
    float4 lightPosition : LIGHT_POSITION;
    float3 worldPosition : POSITION;
    float3 worldNormal   : NORMAL;
};

cbuffer PSLight
{
    float3 LightPosition;
    float  LightFadeCosBegin;
    float3 LightDirection;
    float  LightFadeCosEnd;
    float3 LightIncidence;
    float  LightAmbient;
};

float3 calcLightFactor(PSInput input)
{
    float3 position   = input.worldPosition;
    float3 lightToPos = position - LightPosition;
    float distance2   = dot(lightToPos, lightToPos);
    float3 attenFlux  = LightIncidence / distance2;
    lightToPos        = normalize(lightToPos);

    float cosTheta = dot(lightToPos, LightDirection);
    float fadeFactor =
        (cosTheta - LightFadeCosEnd) / (LightFadeCosBegin - LightFadeCosEnd);
    fadeFactor = pow(saturate(fadeFactor), 3);

    return fadeFactor * attenFlux * max(0, dot(input.worldNormal, -lightToPos));
}

float calcShadowFactor(PSInput input);

float3 calcShadowCoord(float4 lightClipPos)
{
    float2 lightNDCPos = lightClipPos.xy / lightClipPos.w;
    float2 shadowUV    = float2(0.5, -0.5) * lightNDCPos.xy + 0.5;
    return float3(shadowUV, lightClipPos.z);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    input.worldNormal   = normalize(input.worldNormal);
    float3 lightFactor  = calcLightFactor(input);
    float  shadowFactor = calcShadowFactor(input);
    float3 color        = shadowFactor * lightFactor + LightAmbient;
    return float4(pow(color, 1 / 2.2), 1);
}

#endif // #ifndef MESH_COMMON_HLSL
