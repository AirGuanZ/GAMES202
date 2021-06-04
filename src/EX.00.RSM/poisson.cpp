#include <cyPoint.h>
#include <cySampleElim.h>

#include <random>

#include "./poisson.h"

// IMPROVE: use better sampling strategy: weighted sample elimination

ComPtr<ID3D11ShaderResourceView> createPoissionDiskSamplesSRV(
    int sampleCount, float radius)
{
    std::default_random_engine rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dis(0, 1);

    std::vector<cy::Point2f> candidateSamples;
    for(int i = 0; i < sampleCount; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            const float x = dis(rng);
            const float y = dis(rng);
            candidateSamples.push_back({ x, y });
        }
    }

    std::vector<cy::Point2f> resultSamples(sampleCount);

    cy::WeightedSampleElimination<cy::Point2f, float, 2> wse;
    wse.SetTiling(true);
    wse.Eliminate(candidateSamples.data(), candidateSamples.size(),
        resultSamples.data(), resultSamples.size());

    std::vector<Float3> data(sampleCount);
    float sumWeights = 0;
    for(int i = 0; i < sampleCount; ++i)
    {
        const auto &sam = resultSamples[i];

        Float3 point;
        point.x = radius * sam.x * std::cos(2 * 3.14159265f * sam.y);
        point.y = radius * sam.x * std::sin(2 * 3.14159265f * sam.y);
        point.z = sam.x * sam.x;

        sumWeights += point.z;
        data[i] = point;
    }

    float invSumWeights = 1 / (std::max)(sumWeights, 0.01f);
    for(auto &p : data)
        p.z *= invSumWeights;
    
    const size_t bufBytes = sizeof(Float3) * sampleCount;

    D3D11_BUFFER_DESC bufDesc;
    bufDesc.ByteWidth           = static_cast<UINT>(bufBytes);
    bufDesc.Usage               = D3D11_USAGE_IMMUTABLE;
    bufDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    bufDesc.CPUAccessFlags      = 0;
    bufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = sizeof(Float3);

    D3D11_SUBRESOURCE_DATA bufData;
    bufData.pSysMem          = data.data();
    bufData.SysMemPitch      = 0;
    bufData.SysMemSlicePitch = 0;
    auto buf = device.createBuffer(bufDesc, &bufData);
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format               = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements   = sampleCount;

    return device.createSRV(buf, srvDesc);
}
