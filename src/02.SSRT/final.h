#pragma once

#include "./common.h"

class FinalRenderer
{
public:

    void initialize();

    void render(
        ComPtr<ID3D11ShaderResourceView> direct,
        ComPtr<ID3D11ShaderResourceView> indirect);

private:

    struct PSParams
    {
        int   direct;
        int   indirect;
        float pad0;
        float pad1;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *directSlot_   = nullptr;
    ShaderResourceViewSlot<PS> *indirectSlot_ = nullptr;

    ConstantBuffer<PSParams> psParams_;
};
