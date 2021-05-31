#ifndef LIGHT_HLSL
#define LIGHT_HLSL

cbuffer PSLight
{
    float3 LightDirection; float LightPad0;
    float3 LightRadiance;  float LightPad1;
}

#endif // #ifndef LIGHT_HLSL
