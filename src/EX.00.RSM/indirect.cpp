#include "./indirect.h"
#include "./poisson.h"

void IndirectRenderer::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<VS>(
        "./asset/EX.00/lowres_indirect.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/EX.00/lowres_indirect.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    gbufferA_ = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferA");
    gbufferB_ = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferB");

    rsmBufferA_ = shaderRscs_.getShaderResourceViewSlot<PS>("RSMBufferA");
    rsmBufferB_ = shaderRscs_.getShaderResourceViewSlot<PS>("RSMBufferB");
    rsmBufferC_ = shaderRscs_.getShaderResourceViewSlot<PS>("RSMBufferC");

    poissonDiskSamplesSlot_ =
        shaderRscs_.getShaderResourceViewSlot<PS>("PoissonDiskSamples");

    psIndirectParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("IndirectParams")
        ->setBuffer(psIndirectParams_);

    auto nearestSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("RSMSampler")->setSampler(nearestSampler);

    resize(res);

    psIndirectParamsData_.enableIndirect = 1;
}

void IndirectRenderer::resize(const Int2 &res)
{
    renderTarget_ = RenderTarget(res);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void IndirectRenderer::setGBuffer(
    ComPtr<ID3D11ShaderResourceView> gbufferA,
    ComPtr<ID3D11ShaderResourceView> gbufferB)
{
    gbufferA_->setShaderResourceView(std::move(gbufferA));
    gbufferB_->setShaderResourceView(std::move(gbufferB));
}

void IndirectRenderer::setRSM(
    ComPtr<ID3D11ShaderResourceView> rsmBufferA,
    ComPtr<ID3D11ShaderResourceView> rsmBufferB,
    ComPtr<ID3D11ShaderResourceView> rsmBufferC)
{
    rsmBufferA_->setShaderResourceView(std::move(rsmBufferA));
    rsmBufferB_->setShaderResourceView(std::move(rsmBufferB));
    rsmBufferC_->setShaderResourceView(std::move(rsmBufferC));
}

ComPtr<ID3D11ShaderResourceView> IndirectRenderer::getOutput() const
{
    return renderTarget_.getColorShaderResourceView(0);
}

void IndirectRenderer::setSampleParams(
    int sampleCount, float sampleRadius)
{
    psIndirectParamsData_.sampleCount = sampleCount;
    psIndirectParamsData_.sampleRadius = sampleRadius;

    auto srv = createPoissionDiskSamplesSRV(
        psIndirectParamsData_.sampleCount, psIndirectParamsData_.sampleRadius);
    poissonDiskSamplesSlot_->setShaderResourceView(srv);
}

void IndirectRenderer::setLight(
    const Mat4 &lightViewProj, float lightProjWorldArea)
{
    psIndirectParamsData_.lightViewProj      = lightViewProj;
    psIndirectParamsData_.lightProjWorldArea = lightProjWorldArea;
}

void IndirectRenderer::render()
{
    psIndirectParams_.update(psIndirectParamsData_);

    renderTarget_.clearColorBuffer(0, { 0, 0, 0, 0 });
    renderTarget_.bind();
    renderTarget_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext.draw(6, 0);

    shaderRscs_.unbind();
    shader_.unbind();

    renderTarget_.unbind();
}
