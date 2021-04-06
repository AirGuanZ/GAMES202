#pragma once

#include "./common.h"

class MeshRendererPCF : public agz::misc::uncopyable_t
{
public:

    void initialize();

    void setCamera(const Mat4 &viewProj);

    void setLight(const Light &light);

    void setShadowMap(ComPtr<ID3D11ShaderResourceView> sm, const Mat4 &viewProj);

    void setFilter(float filterRadius, int sampleCount);

    void begin();

    void end();

    void render(
        const VertexBuffer<MeshVertex> &vertexBuffer,
        const Mat4                     &world);

private:

    struct VSTransform
    {
        Mat4 world;
        Mat4 WVP;
        Mat4 lightWVP;
    };

    struct PSShadowMapConst
    {
        float textureResolution;
        float filterRadius;
        int   sampleCount;
        int   pad0;
    };

    ComPtr<ID3D11InputLayout> inputLayout_;

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ConstantBufferSlot<VS>     *vsTransformSlot_ = nullptr;
    ConstantBuffer<VSTransform> vsTransform_;

    ConstantBufferSlot<PS> *psLightSlot_ = nullptr;
    ConstantBuffer<Light>   psLight_;

    ConstantBufferSlot<PS>          *psShadowMapConstSlot_ = nullptr;
    ConstantBuffer<PSShadowMapConst> psShadowMapConst_;
    PSShadowMapConst                 psShadowMapConstData_ = {};

    ShaderResourceViewSlot<PS> *psShadowMapSlot_ = nullptr;

    Mat4 lightViewProj_;
    Mat4 viewProj_;
};
