#pragma once

#include <common/common.h>

class GaussianBlur : public agz::misc::uncopyable_t
{
public:

    void initialize(const Int2 &resolution);

    void setFilter(int radius, float sigma);

    void blur(ComPtr<ID3D11ShaderResourceView> srv);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

private:

    struct BlurSetting
    {
        int   radius;
        float weights[31];
    };

    Shader<CS>         horiShader_;
    Shader<CS>::RscMgr horiShaderRscs_;

    ShaderResourceViewSlot<CS>  *horiInputSlot_  = nullptr;
    UnorderedAccessViewSlot<CS> *horiOutputSlot_ = nullptr;

    Shader<CS>         vertShader_;
    Shader<CS>::RscMgr vertShaderRscs_;
    
    ShaderResourceViewSlot<CS>  *vertInputSlot_  = nullptr;
    UnorderedAccessViewSlot<CS> *vertOutputSlot_ = nullptr;

    ConstantBuffer<BlurSetting> blurSetting_;

    ComPtr<ID3D11ShaderResourceView>  srvA_;
    ComPtr<ID3D11UnorderedAccessView> uavA_;

    ComPtr<ID3D11ShaderResourceView>  srvB_;
    ComPtr<ID3D11UnorderedAccessView> uavB_;

    Int2 resolution_;

    int   radius_ = 5;
    float sigma_  = 2;
};
