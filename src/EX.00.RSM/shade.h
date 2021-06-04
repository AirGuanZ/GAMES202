#pragma once

#include "./indirect.h"

class Renderer : public agz::misc::uncopyable_t
{
public:

    void initialize();

    void setGBuffer(
        ComPtr<ID3D11ShaderResourceView> gbufferA,
        ComPtr<ID3D11ShaderResourceView> gbufferB);

    void setRSM(
        ComPtr<ID3D11ShaderResourceView> rsmBufferA,
        ComPtr<ID3D11ShaderResourceView> rsmBufferB,
        ComPtr<ID3D11ShaderResourceView> rsmBufferC);

    void setLowResIndirect(
        ComPtr<ID3D11ShaderResourceView> lowResGBufferA,
        ComPtr<ID3D11ShaderResourceView> lowResGBufferB,
        ComPtr<ID3D11ShaderResourceView> lowResIndirect);

    void setSampleParams(int sampleCount, float sampleRadius);

    void setLight(
        const DirectionalLight &light,
        const Mat4             &lightViewProj,
        float                   lightProjWorldArea);

    void enableIndirect(bool enabled);
    
    void render();

private:

    struct PSLight
    {
        DirectionalLight light;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *gbufferA_ = nullptr;
    ShaderResourceViewSlot<PS> *gbufferB_ = nullptr;
    
    ShaderResourceViewSlot<PS> *rsmBufferA_ = nullptr;
    ShaderResourceViewSlot<PS> *rsmBufferB_ = nullptr;
    ShaderResourceViewSlot<PS> *rsmBufferC_ = nullptr;

    ShaderResourceViewSlot<PS> *lowResGBufferA_ = nullptr;
    ShaderResourceViewSlot<PS> *lowResGBufferB_ = nullptr;
    ShaderResourceViewSlot<PS> *lowResIndirect_ = nullptr;

    ShaderResourceViewSlot<PS> *poissonDiskSamplesSlot_ = nullptr;

    PSLight                 psLightData_ = {};
    ConstantBuffer<PSLight> psLight_;

    IndirectRenderer::IndirectParams                 psIndirectParamsData_ = {};
    ConstantBuffer<IndirectRenderer::IndirectParams> psIndirectParams_;
};
