#include <cyPoint.h>
#include <cySampleElim.h>

#include <random>

#include "./indirect.h"

void IndirectRenderer::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<VS>(
        "./asset/02/indirect.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/02/indirect.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    gbufferASlot_    = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferA");
    gbufferBSlot_    = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferB");
    viewZMipmapSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("ViewZMipmap");
    directSlot_      = shaderRscs_.getShaderResourceViewSlot<PS>("Direct");
    rawSamplesSlot_  = shaderRscs_.getShaderResourceViewSlot<PS>("RawSamples");
    
    indirectParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("IndirectParams")
        ->setBuffer(indirectParams_);

    resize(res);

    auto sampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("PointSampler")->setSampler(sampler);
}

void IndirectRenderer::resize(const Int2 &res)
{
    indirectParamsData_.outputWidth  = static_cast<float>(res.x);
    indirectParamsData_.outputHeight = static_cast<float>(res.y);

    renderTarget_ = RenderTarget(res);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void IndirectRenderer::setCamera(
    const Mat4 &view, const Mat4 &proj, float nearZ)
{
    indirectParamsData_.view      = view;
    indirectParamsData_.proj      = proj;
    indirectParamsData_.projNearZ = nearZ;
}

void IndirectRenderer::setSampleCount(int sampleCount)
{
    indirectParamsData_.sampleCount = sampleCount;
    updateRawSamples();
}

void IndirectRenderer::setDepthThreshold(float threshold)
{
    indirectParamsData_.depthThreshold = threshold;
}

void IndirectRenderer::setTracer(
    int maxSteps, int initialMipLevel, float initialTraceStep)
{
    indirectParamsData_.maxTraceSteps    = maxSteps;
    indirectParamsData_.initialMipLevel  = initialMipLevel;
    indirectParamsData_.initialTraceStep = initialTraceStep;
}

ComPtr<ID3D11ShaderResourceView> IndirectRenderer::getOutput() const
{
    return renderTarget_.getColorShaderResourceView(0);
}

void IndirectRenderer::render(
    ComPtr<ID3D11ShaderResourceView> gbufferA,
    ComPtr<ID3D11ShaderResourceView> gbufferB,
    ComPtr<ID3D11ShaderResourceView> viewZMipmap,
    ComPtr<ID3D11ShaderResourceView> direct)
{
    static int frameIndex = 0;
    indirectParamsData_.FrameIndex = static_cast<float>(frameIndex++);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    viewZMipmap->GetDesc(&srvDesc);
    const int totalLevels = srvDesc.Texture2D.MipLevels;

    indirectParamsData_.initialMipLevel = (std::min)(
        indirectParamsData_.initialMipLevel, totalLevels - 1);
    indirectParamsData_.initialMipLevel = (std::min)(
        indirectParamsData_.initialMipLevel, 4);
    if(indirectParamsData_.initialMipLevel % 2 == 1)
        --indirectParamsData_.initialMipLevel;

    gbufferASlot_   ->setShaderResourceView(std::move(gbufferA));
    gbufferBSlot_   ->setShaderResourceView(std::move(gbufferB));
    viewZMipmapSlot_->setShaderResourceView(std::move(viewZMipmap));
    directSlot_     ->setShaderResourceView(std::move(direct));

    indirectParams_.update(indirectParamsData_);

    renderTarget_.bind();
    renderTarget_.clearColorBuffer(0, { 0, 0, 0, 0 });
    renderTarget_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext.draw(6, 0);

    shaderRscs_.unbind();
    shader_.unbind();

    renderTarget_.unbind();

    gbufferASlot_->setShaderResourceView(nullptr);
    gbufferBSlot_->setShaderResourceView(nullptr);
    directSlot_->setShaderResourceView(nullptr);
}

void IndirectRenderer::updateRawSamples()
{
    static std::default_random_engine::result_type seed = 0;

    const int N = indirectParamsData_.sampleCount;

    std::default_random_engine rng{ seed + 1 };
    std::uniform_real_distribution<float> dis(0, 1);

    std::vector<cy::Point2f> candidateSamples;
    for(int i = 0; i < N; ++i)
    {
        for(int j = 0; j < 10; ++j)
        {
            const float x = dis(rng);
            const float y = dis(rng);
            candidateSamples.push_back({ x, y });
        }
    }

    std::vector<cy::Point2f> resultSamples(N);
    cy::WeightedSampleElimination<cy::Point2f, float, 2> wse;
    wse.SetTiling(true);
    wse.Eliminate(
        candidateSamples.data(), candidateSamples.size(),
        resultSamples.data(), resultSamples.size());

    static_assert(sizeof(cy::Point2f) == sizeof(float) * 2);
    const size_t bufBytes = sizeof(Float2) * N;

    D3D11_BUFFER_DESC bufDesc;
    bufDesc.ByteWidth           = static_cast<UINT>(bufBytes);
    bufDesc.Usage               = D3D11_USAGE_IMMUTABLE;
    bufDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    bufDesc.CPUAccessFlags      = 0;
    bufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufDesc.StructureByteStride = sizeof(Float2);

    D3D11_SUBRESOURCE_DATA bufData;
    bufData.pSysMem          = resultSamples.data();
    bufData.SysMemPitch      = 0;
    bufData.SysMemSlicePitch = 0;
    auto buf = device.createBuffer(bufDesc, &bufData);
    
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format               = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.ElementOffset = 0;
    srvDesc.Buffer.NumElements   = N;
    auto srv = device.createSRV(buf, srvDesc);

    rawSamplesSlot_->setShaderResourceView(srv);
}
