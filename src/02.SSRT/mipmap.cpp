#include "./mipmap.h"

namespace
{

    int computeMipLevels(int w, int h)
    {
        int result = 1;
        while(w > 1 || h > 1)
        {
            ++result;
            w /= 2;
            h /= 2;
        }
        return result;
    }

} // namespace anonymous

void MipmapsGenerator::initialize(const Int2 &rootRes)
{
    copyShader_.initializeStageFromFile<CS>(
        "./asset/02/view_z.hlsl", nullptr, "CSMain");
    copyShaderRscs_ = copyShader_.createResourceManager();

    genShader_.initializeStageFromFile<CS>(
        "./asset/02/mipmap.hlsl", nullptr, "CSMain");
    genShaderRscs_ = genShader_.createResourceManager();

    copySrcSlot_ = copyShaderRscs_.getShaderResourceViewSlot<CS>("Src");
    copyDstSlot_ = copyShaderRscs_.getUnorderedAccessViewSlot<CS>("Dst");

    genSrcSlot_ = genShaderRscs_.getShaderResourceViewSlot<CS>("LastLevel");
    genDstSlot_ = genShaderRscs_.getUnorderedAccessViewSlot<CS>("ThisLevel");

    csParams_.initialize();
    copyShaderRscs_.getConstantBufferSlot<CS>("CSParams")->setBuffer(csParams_);
    genShaderRscs_.getConstantBufferSlot<CS>("CSParams")->setBuffer(csParams_);

    resize(rootRes);
}

void MipmapsGenerator::resize(const Int2 &rootRes)
{
    const int mipLevels = computeMipLevels(rootRes.x, rootRes.y);

    level0Res_ = rootRes;

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = static_cast<UINT>(rootRes.x);
    texDesc.Height         = static_cast<UINT>(rootRes.y);
    texDesc.MipLevels      = static_cast<UINT>(mipLevels);
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_SHADER_RESOURCE |
                             D3D11_BIND_UNORDERED_ACCESS;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;
    texture_ = device.createTex2D(texDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = mipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srv_ = device.createSRV(texture_, srvDesc);

    levelViews_.resize(mipLevels);
    for(int i = 0; i < mipLevels; ++i)
    {
        srvDesc.Texture2D.MipLevels       = 1;
        srvDesc.Texture2D.MostDetailedMip = static_cast<UINT>(i);
        levelViews_[i].srv = device.createSRV(texture_, srvDesc);

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
        uavDesc.Format             = DXGI_FORMAT_R32_FLOAT;
        uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = static_cast<UINT>(i);
        levelViews_[i].uav = device.createUAV(texture_, uavDesc);
    }
}

ComPtr<ID3D11ShaderResourceView> MipmapsGenerator::getOutput() const
{
    return srv_;
}

void MipmapsGenerator::generate(ComPtr<ID3D11ShaderResourceView> gbufferB)
{
    constexpr int GROUP_SIZE_X = 16, GROUP_SIZE_Y = 16;

    auto getGroupCount = [](const Int2 &res)
    {
        return Int2(
            (res.x + GROUP_SIZE_X - 1) / GROUP_SIZE_X,
            (res.x + GROUP_SIZE_Y - 1) / GROUP_SIZE_Y);
    };

    // copy view-space z from gbuffer to level 0

    copySrcSlot_->setShaderResourceView(gbufferB);
    copyDstSlot_->setUnorderedAccessView(levelViews_[0].uav);

    csParams_.update({
        static_cast<float>(level0Res_.x),
        static_cast<float>(level0Res_.y),
        static_cast<float>(level0Res_.x),
        static_cast<float>(level0Res_.y) });

    copyShader_.bind();
    copyShaderRscs_.bind();

    Int2 groupCount = getGroupCount(level0Res_);
    deviceContext.dispatch(groupCount.x, groupCount.y);

    copyShaderRscs_.unbind();
    copyShader_.unbind();

    // generate miplevels

    Int2 res = level0Res_;
    for(size_t i = 1; i < levelViews_.size(); ++i)
    {
        const Int2 newRes = {
            (std::max)(res.x / 2, 1),
            (std::max)(res.y / 2, 1)
        };
        csParams_.update({
            static_cast<float>(res.x),
            static_cast<float>(res.y),
            static_cast<float>(newRes.x),
            static_cast<float>(newRes.y) });
        res = newRes;

        genSrcSlot_->setShaderResourceView(levelViews_[i - 1].srv);
        genDstSlot_->setUnorderedAccessView(levelViews_[i].uav);

        genShader_.bind();
        genShaderRscs_.bind();

        groupCount = getGroupCount(newRes);
        deviceContext.dispatch(groupCount.x, groupCount.y);

        genShaderRscs_.unbind();
        genShader_.unbind();
    }
}
