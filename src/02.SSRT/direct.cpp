#include "./direct.h"

void DirectRenderer::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<VS>(
        "./asset/02/direct.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/02/direct.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    shadowMapSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("ShadowMap");
    gbufferASlot_  = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferA");
    gbufferBSlot_  = shaderRscs_.getShaderResourceViewSlot<PS>("GBufferB");

    resize(res);

    psLight_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSLight")->setBuffer(psLight_);

    auto sampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("PointSampler")->setSampler(sampler);
}

void DirectRenderer::resize(const Int2 &res)
{
    renderTarget_ = RenderTarget(res);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void DirectRenderer::setLight(
    const DirectionalLight &light, const Mat4 &lightViewProj)
{
    psLightData_ = { light, lightViewProj };
}

ComPtr<ID3D11ShaderResourceView> DirectRenderer::getOutput() const
{
    return renderTarget_.getColorShaderResourceView(0);
}

void DirectRenderer::render(
    const ComPtr<ID3D11ShaderResourceView> &shadowMap,
    const ComPtr<ID3D11ShaderResourceView> &gbufferA,
    const ComPtr<ID3D11ShaderResourceView> &gbufferB)
{
    shadowMapSlot_->setShaderResourceView(shadowMap);
    gbufferASlot_->setShaderResourceView(gbufferA);
    gbufferBSlot_->setShaderResourceView(gbufferB);

    psLight_.update(psLightData_);

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
}
