float3 heightToNormal(
    float            diffStep,
    float            heightScale,
    Texture2D<float> Height,
    SamplerState     pointSampler,
    float3           normal,
    float3           binormal,
    float3           tangent,
    float2           texSize,
    float2           dxy,
    float2           uv)
{
    float2 uvDiff = diffStep / texSize;

    float n01 = Height.SampleLevel(pointSampler, uv - float2(0, uvDiff.y), 0);
    float n10 = Height.SampleLevel(pointSampler, uv - float2(uvDiff.x, 0), 0);
    float p01 = Height.SampleLevel(pointSampler, uv + float2(0, uvDiff.y), 0);
    float p10 = Height.SampleLevel(pointSampler, uv + float2(uvDiff.x, 0), 0);

    return normalize(cross(
        2 * diffStep * dxy.y * binormal + heightScale * (p01 - n01) * normal,
        2 * diffStep * dxy.x * tangent  + heightScale * (p10 - n10) * normal));
}
