#pragma once

#include "./common.h"

class IndirectRenderer
{
public:

    void initialize(const Int2 &res);

    void resize(const Int2 &res);

    void setCamera(const Mat4 &view, const Mat4 &proj, float nearZ);

    void setSampleCount(int sampleCount);

    void setDepthThreshold(float threshold);

    void setTracer(int maxSteps, int initialMipLevel, float initialTraceStep);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

    void render(
        ComPtr<ID3D11ShaderResourceView> gbufferA,
        ComPtr<ID3D11ShaderResourceView> gbufferB,
        ComPtr<ID3D11ShaderResourceView> viewZMipmap,
        ComPtr<ID3D11ShaderResourceView> direct);

private:

    void updateRawSamples();

    struct IndirectParams
    {
        Mat4 view;
        Mat4 proj;

        uint32_t sampleCount;
        uint32_t maxTraceSteps;
        float    projNearZ;
        float    FrameIndex;

        float outputWidth;
        float outputHeight;
        int   initialMipLevel;
        float initialTraceStep;

        float depthThreshold;
        float pad0;
        float pad1;
        float pad2;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *gbufferASlot_    = nullptr;
    ShaderResourceViewSlot<PS> *gbufferBSlot_    = nullptr;
    ShaderResourceViewSlot<PS> *viewZMipmapSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *directSlot_      = nullptr;
    ShaderResourceViewSlot<PS> *rawSamplesSlot_  = nullptr;

    RenderTarget renderTarget_;

    IndirectParams                 indirectParamsData_ = {};
    ConstantBuffer<IndirectParams> indirectParams_;
};
