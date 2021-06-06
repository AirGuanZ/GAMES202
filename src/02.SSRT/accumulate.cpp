#include "./accumulate.h"

void IndirectAccumulator::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<CS>(
        "./asset/02/accumulate.hlsl", nullptr, "CSMain");

    shaderRscs_ = shader_.createResourceManager();

    lastGBufferASlot_ = shaderRscs_.getShaderResourceViewSlot<CS>("LastGBuffer");
    thisGBufferASlot_ = shaderRscs_.getShaderResourceViewSlot<CS>("ThisGBuffer");

    accuIndirectSlot_ = shaderRscs_.getShaderResourceViewSlot<CS>("AccuSrc");
    newIndirectSlot_  = shaderRscs_.getShaderResourceViewSlot<CS>("NewIndirect");

    accuOutputSlot_ = shaderRscs_.getUnorderedAccessViewSlot<CS>("AccuDst");

    csParams_.initialize();
    shaderRscs_.getConstantBufferSlot<CS>("CSParams")->setBuffer(csParams_);

    auto sampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<CS>("LinearSampler")->setSampler(sampler);

    resize(res);
}

void IndirectAccumulator::resize(const Int2 &res)
{
    auto createSRVAndUAV = [res]
    {
        D3D11_TEXTURE2D_DESC texDesc;
        texDesc.Width          = static_cast<UINT>(res.x);
        texDesc.Height         = static_cast<UINT>(res.y);
        texDesc.MipLevels      = 1;
        texDesc.ArraySize      = 1;
        texDesc.Format         = DXGI_FORMAT_R32G32B32A32_FLOAT;
        texDesc.SampleDesc     = { 1, 0 };
        texDesc.Usage          = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE |
                                 D3D11_BIND_UNORDERED_ACCESS;
        texDesc.CPUAccessFlags = 0;
        texDesc.MiscFlags      = 0;
        auto tex = device.createTex2D(texDesc);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels       = 1;
        auto srv = device.createSRV(tex, srvDesc);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
        uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        auto uav = device.createUAV(tex, uavDesc);

        return std::make_pair(srv, uav);
    };

    res_ = res;
    std::tie(srvA_, uavA_) = createSRVAndUAV();
    std::tie(srvB_, uavB_) = createSRVAndUAV();
}

ComPtr<ID3D11ShaderResourceView> IndirectAccumulator::getOutput() const
{
    return srvB_;
}

void IndirectAccumulator::setFactor(float alpha)
{
    csParamsData_.alpha = alpha;
}

void IndirectAccumulator::setCamera(const Mat4 &lastViewProj)
{
    csParamsData_.lastViewProj = lastViewProj;
}

void IndirectAccumulator::accumulate(
    ComPtr<ID3D11ShaderResourceView> lastGBufferA,
    ComPtr<ID3D11ShaderResourceView> thisGBufferA,
    ComPtr<ID3D11ShaderResourceView> indirect)
{
    std::swap(srvA_, srvB_);
    std::swap(uavA_, uavB_);

    lastGBufferASlot_->setShaderResourceView(std::move(lastGBufferA));
    thisGBufferASlot_->setShaderResourceView(std::move(thisGBufferA));

    accuIndirectSlot_->setShaderResourceView(srvA_);
    newIndirectSlot_->setShaderResourceView(std::move(indirect));

    accuOutputSlot_->setUnorderedAccessView(uavB_);

    csParams_.update(csParamsData_);

    shader_.bind();
    shaderRscs_.bind();

    const int groupSizeX = 16;
    const int groupSizeY = 16;
    const int groupCountX = (res_.x + groupSizeX - 1) / groupSizeX;
    const int groupCountY = (res_.y + groupSizeY - 1) / groupSizeY;

    deviceContext.dispatch(groupCountX, groupCountY);

    shaderRscs_.unbind();
    shader_.unbind();
}
