#pragma once

#include "./common.h"

class IndirectRenderer : public agz::misc::uncopyable_t
{
public:

    struct IndirectParams
    {
        Mat4 lightViewProj;

        int   sampleCount;
        float sampleRadius;
        float lightProjWorldArea;
        int   enableIndirect;
    };

    void initialize(const Int2 &res);

    void resize(const Int2 &res);

    void setGBuffer(
        ComPtr<ID3D11ShaderResourceView> gbufferA,
        ComPtr<ID3D11ShaderResourceView> gbufferB);

    void setRSM(
        ComPtr<ID3D11ShaderResourceView> rsmBufferA,
        ComPtr<ID3D11ShaderResourceView> rsmBufferB,
        ComPtr<ID3D11ShaderResourceView> rsmBufferC);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

    void setSampleParams(int sampleCount, float maxSampleRadiusInTexel);

    void setLight(const Mat4 &lightViewProj, float lightProjWorldArea);

    void render();

private:
    
    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *gbufferA_ = nullptr;
    ShaderResourceViewSlot<PS> *gbufferB_ = nullptr;

    ShaderResourceViewSlot<PS> *rsmBufferA_ = nullptr;
    ShaderResourceViewSlot<PS> *rsmBufferB_ = nullptr;
    ShaderResourceViewSlot<PS> *rsmBufferC_ = nullptr;

    ShaderResourceViewSlot<PS> *poissonDiskSamplesSlot_ = nullptr;

    RenderTarget renderTarget_;
    
    IndirectParams                 psIndirectParamsData_ = {};
    ConstantBuffer<IndirectParams> psIndirectParams_;
};
