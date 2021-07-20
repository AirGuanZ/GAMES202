#ifndef BRDF_HLSL
#define BRDF_HLSL

float square(float x)
{
    return x * x;
}

float3 fresnel(float3 r0, float cosTheta)
{
    float x  = 1 - cosTheta;
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return r0 + (float3(1, 1, 1) - r0) * x5;
}

float ggx(float cosThetaH, float roughness)
{
    float a  = square(roughness);
    float a2 = square(a);
    float c2 = square(cosThetaH);
    return a2 / (3.14159265f * square(c2 * (a2 - 1) + 1));
}

float ggxSmith(float cosThetaI, float cosThetaO, float roughness)
{
    float k = square(roughness) / 2;
    float gi = cosThetaI / (cosThetaI * (1 - k) + k);
    float go = cosThetaO / (cosThetaO * (1 - k) + k);
    return gi * go;
}

#endif // #ifndef BRDF_HLSL
