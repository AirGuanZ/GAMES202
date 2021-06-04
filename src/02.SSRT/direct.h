#pragma once

#include "./common.h"

class DirectRenderer
{
public:

    void initialize(const Int2 &res);

    void resize(const Int2 &res);

    void setLight(const DirectionalLight &light, const Mat4 &lightViewProj);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;
    
    void render(
        const ComPtr<ID3D11ShaderResourceView> &shadowMap,
        const ComPtr<ID3D11ShaderResourceView> &gbufferA,
        const ComPtr<ID3D11ShaderResourceView> &gbufferB);

private:

    struct PSLight
    {
        DirectionalLight light;
        Mat4             lightViewProj;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *shadowMapSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *gbufferASlot_  = nullptr;
    ShaderResourceViewSlot<PS> *gbufferBSlot_  = nullptr;

    RenderTarget renderTarget_;

    PSLight                 psLightData_ = {};
    ConstantBuffer<PSLight> psLight_;
};
