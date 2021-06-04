#include "./final.h"

void FinalRenderer::initialize()
{
    shader_.initializeStageFromFile<VS>(
        "./asset/02/final.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/02/final.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    directSlot_   = shaderRscs_.getShaderResourceViewSlot<PS>("Direct");
    indirectSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("Indirect");

    psParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSParams")->setBuffer(psParams_);

    auto sampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("PointSampler")->setSampler(sampler);
}

void FinalRenderer::render(
    ComPtr<ID3D11ShaderResourceView> direct,
    ComPtr<ID3D11ShaderResourceView> indirect)
{
    assert(direct || indirect);

    PSParams psParamsData = {};
    psParamsData.direct   = direct != nullptr;
    psParamsData.indirect = indirect != nullptr;

    psParams_.update(psParamsData);

    if(!direct)
        direct = indirect;
    if(!indirect)
        indirect = direct;

    directSlot_->setShaderResourceView(direct);
    indirectSlot_->setShaderResourceView(indirect);

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext.draw(6, 0);

    shaderRscs_.unbind();
    shader_.unbind();

    directSlot_->setShaderResourceView(nullptr);
    indirectSlot_->setShaderResourceView(nullptr);
}
