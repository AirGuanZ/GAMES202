#include "./poisson.h"
#include "./shade.h"

void Renderer::initialize()
{
    shader_.initializeStageFromFile<VS>(
        "./asset/EX.00/shade.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/EX.00/shade.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    gbufferA_ = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferA");
    gbufferB_ = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferB");

    rsmBufferA_ = shaderRscs_.getShaderResourceViewSlot<PS>("RSMBufferA");
    rsmBufferB_ = shaderRscs_.getShaderResourceViewSlot<PS>("RSMBufferB");
    rsmBufferC_ = shaderRscs_.getShaderResourceViewSlot<PS>("RSMBufferC");

    lowResGBufferA_ = shaderRscs_.getShaderResourceViewSlot<PS>("LowResGBufferA");
    lowResGBufferB_ = shaderRscs_.getShaderResourceViewSlot<PS>("LowResGBufferB");
    lowResIndirect_ = shaderRscs_.getShaderResourceViewSlot<PS>("LowResIndirect");

    poissonDiskSamplesSlot_ =
        shaderRscs_.getShaderResourceViewSlot<PS>("PoissonDiskSamples");

    psLight_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSLight")->setBuffer(psLight_);

    psIndirectParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("IndirectParams")
        ->setBuffer(psIndirectParams_);

    psIndirectParamsData_.enableIndirect = 1;

    auto nearestSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("RSMSampler")->setSampler(nearestSampler);
}

void Renderer::setGBuffer(
    ComPtr<ID3D11ShaderResourceView> gbufferA,
    ComPtr<ID3D11ShaderResourceView> gbufferB)
{
    gbufferA_->setShaderResourceView(std::move(gbufferA));
    gbufferB_->setShaderResourceView(std::move(gbufferB));
}

void Renderer::setRSM(
    ComPtr<ID3D11ShaderResourceView> rsmBufferA,
    ComPtr<ID3D11ShaderResourceView> rsmBufferB,
    ComPtr<ID3D11ShaderResourceView> rsmBufferC)
{
    rsmBufferA_->setShaderResourceView(std::move(rsmBufferA));
    rsmBufferB_->setShaderResourceView(std::move(rsmBufferB));
    rsmBufferC_->setShaderResourceView(std::move(rsmBufferC));
}

void Renderer::setLowResIndirect(
    ComPtr<ID3D11ShaderResourceView> lowResGBufferA,
    ComPtr<ID3D11ShaderResourceView> lowResGBufferB,
    ComPtr<ID3D11ShaderResourceView> lowresIndirect)
{
    lowResGBufferA_->setShaderResourceView(std::move(lowResGBufferA));
    lowResGBufferB_->setShaderResourceView(std::move(lowResGBufferB));
    lowResIndirect_->setShaderResourceView(std::move(lowresIndirect));
}

void Renderer::setSampleParams(int sampleCount, float sampleRadius)
{
    psIndirectParamsData_.sampleCount  = sampleCount;
    psIndirectParamsData_.sampleRadius = sampleRadius;

    auto srv = createPoissionDiskSamplesSRV(
        psIndirectParamsData_.sampleCount, psIndirectParamsData_.sampleRadius);
    poissonDiskSamplesSlot_->setShaderResourceView(srv);
}

void Renderer::setLight(
    const DirectionalLight &light,
    const Mat4             &lightViewProj,
    float                   lightProjWorldArea)
{
    psLightData_.light = light;

    psIndirectParamsData_.lightViewProj      = lightViewProj;
    psIndirectParamsData_.lightProjWorldArea = lightProjWorldArea;
}

void Renderer::enableIndirect(bool enabled)
{
    psIndirectParamsData_.enableIndirect = enabled;
}

void Renderer::render()
{
    psLight_.update(psLightData_);
    psIndirectParams_.update(psIndirectParamsData_);

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext.draw(6, 0);

    shaderRscs_.unbind();
    shader_.unbind();
}
