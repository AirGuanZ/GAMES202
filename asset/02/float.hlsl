#ifndef FLOAT_HLSL
#define FLOAT_HLSL

float packFloats(float a, float b)
{
    uint a16 = f32tof16(a);
    uint b16 = f32tof16(b);
    uint abPacked = (a16 << 16) | b16;
    return asfloat(abPacked);
}

void unpackFloat(float input, out float a, out float b)
{
    uint uintInput = asuint(input);
    a = f16tof32(uintInput >> 16);
    b = f16tof32(uintInput);
}

#endif // #ifndef FLOAT_HLSL
