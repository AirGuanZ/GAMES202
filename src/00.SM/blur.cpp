#include "./blur.h"

namespace
{
    std::vector<float> generateBlurWeights(int radius, float sigma)
    {
        const float fac = -0.5f / (sigma * sigma);
        const auto gaussian = [fac](int x) { return std::exp(fac * x * x); };

        std::vector<float> ret(2 * radius + 1);
        for(int r = -radius; r <= radius; ++r)
            ret[r + radius] = gaussian(r);

        const float sum = std::accumulate(
            ret.begin(), ret.end(), 0.0f, std::plus<float>());
        const float invSum = 1 / sum;

        for(auto &v : ret)
            v *= invSum;

        return ret;
    }
}

void GaussianBlur::initialize(const Int2 &resolution)
{
    // shader

    horiShader_.initializeStageFromFile<CS>("./asset/00/blur.hlsl", nullptr, "HoriMain");
    horiShaderRscs_ = horiShader_.createResourceManager();

    vertShader_.initializeStageFromFile<CS>("./asset/00/blur.hlsl", nullptr, "VertMain");
    vertShaderRscs_ = vertShader_.createResourceManager();

    horiInputSlot_  = horiShaderRscs_.getShaderResourceViewSlot<CS>("InputImage");
    horiOutputSlot_ = horiShaderRscs_.getUnorderedAccessViewSlot<CS>("OutputImage");
    
    vertInputSlot_  = vertShaderRscs_.getShaderResourceViewSlot<CS>("InputImage");
    vertOutputSlot_ = vertShaderRscs_.getUnorderedAccessViewSlot<CS>("OutputImage");

    // constant buffer

    blurSetting_.initialize();

    horiShaderRscs_.getConstantBufferSlot<CS>("BlurSetting")->setBuffer(blurSetting_);
    vertShaderRscs_.getConstantBufferSlot<CS>("BlurSetting")->setBuffer(blurSetting_);

    // intermediate result

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = resolution.x;
    texDesc.Height         = resolution.y;
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;

    auto texA = device.createTex2D(texDesc);
    auto texB = device.createTex2D(texDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    uavA_ = device.createUAV(texA, uavDesc);
    uavB_ = device.createUAV(texB, uavDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    srvA_ = device.createSRV(texA, srvDesc);
    srvB_ = device.createSRV(texB, srvDesc);

    resolution_ = resolution;
}

void GaussianBlur::setFilter(int radius, float sigma)
{
    radius_ = radius;
    sigma_  = sigma;

    auto weights = generateBlurWeights(radius_, sigma_);

    BlurSetting blurSetting;
    blurSetting.radius = radius_;
    for(size_t i = 0; i < weights.size(); ++i)
        blurSetting.weights[i] = weights[i];

    blurSetting_.update(blurSetting);
}

void GaussianBlur::blur(ComPtr<ID3D11ShaderResourceView> srv)
{
    // hori

    horiInputSlot_->setShaderResourceView(srv);
    horiOutputSlot_->setUnorderedAccessView(uavA_);

    horiShader_.bind();
    horiShaderRscs_.bind();

    const int groupXCount = agz::upalign_to(resolution_.x, 256) / 256;
    deviceContext.dispatch(groupXCount, resolution_.y);

    horiShaderRscs_.unbind();
    horiShader_.unbind();

    // vert

    vertInputSlot_->setShaderResourceView(srvA_);
    vertOutputSlot_->setUnorderedAccessView(uavB_);

    vertShader_.bind();
    vertShaderRscs_.bind();

    const int groupYCount = agz::upalign_to(resolution_.y, 256) / 256;
    deviceContext.dispatch(resolution_.x, groupYCount);

    vertShaderRscs_.unbind();
    vertShader_.unbind();
}

ComPtr<ID3D11ShaderResourceView> GaussianBlur::getOutput() const
{
    return srvB_;
}
