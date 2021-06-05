#pragma once

#include "./common.h"

class MipmapsGenerator
{
public:

    void initialize(const Int2 &rootRes);

    void resize(const Int2 &rootRes);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

    void generate(ComPtr<ID3D11ShaderResourceView> gbufferB);

private:

    struct CSParams
    {
        float lastWidth;
        float lastHeight;
        float thisWidth;
        float thisHeight;
    };

    struct LevelView
    {
        ComPtr<ID3D11ShaderResourceView>  srv;
        ComPtr<ID3D11UnorderedAccessView> uav;
    };

    Shader<CS>         copyShader_;
    Shader<CS>::RscMgr copyShaderRscs_;

    Shader<CS>         genShader_;
    Shader<CS>::RscMgr genShaderRscs_;

    ShaderResourceViewSlot<CS>  *copySrcSlot_ = nullptr;
    UnorderedAccessViewSlot<CS> *copyDstSlot_ = nullptr;

    ShaderResourceViewSlot<CS>  *genSrcSlot_ = nullptr;
    UnorderedAccessViewSlot<CS> *genDstSlot_ = nullptr;

    ConstantBuffer<CSParams> csParams_;

    Int2 level0Res_;

    ComPtr<ID3D11Texture2D>          texture_;
    ComPtr<ID3D11ShaderResourceView> srv_;
    std::vector<LevelView>           levelViews_;
};
