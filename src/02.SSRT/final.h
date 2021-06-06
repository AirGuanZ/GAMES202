#pragma once

#include "./common.h"

class FinalRenderer
{
public:

    void initialize();

    void render(
        ComPtr<ID3D11ShaderResourceView> gbufferB,
        ComPtr<ID3D11ShaderResourceView> direct,
        ComPtr<ID3D11ShaderResourceView> indirect,
        bool                             indirectColor,
        float                            exposure);

private:

    struct PSParams
    {
        int   direct;
        int   indirect;
        int   indirectColor;
        float exposure;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *directSlot_   = nullptr;
    ShaderResourceViewSlot<PS> *indirectSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *gbufferBSlot_ = nullptr;

    ConstantBuffer<PSParams> psParams_;
};
