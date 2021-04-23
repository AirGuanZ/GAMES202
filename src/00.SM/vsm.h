#pragma once

#include "./common.h"

class VarianceShadowMapRenderer : public agz::misc::uncopyable_t
{
public:

    void initialize(const Int2 &resolution);

    ComPtr<ID3D11ShaderResourceView> getSRV() const;

    const Mat4 &getLightViewProj() const;
    
    void setLight(const Light &light);

    void begin();

    void end();

    void render(
        const VertexBuffer<MeshVertex> &vertexBuffer,
        const Mat4                     &world);

private:

    void updateLightViewProj();

    struct VSTransform
    {
        Mat4 WVP;
    };

    ComPtr<ID3D11InputLayout> inputLayout_;
    RenderTarget              renderTarget_;

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;
    
    ConstantBuffer<VSTransform> vsTransform_;

    Light light_ = {};
    Mat4  lightViewProj_;
};

class MeshRendererVSM : public agz::misc::uncopyable_t
{
public:

    void initialize();

    void setCamera(const Mat4 &viewProj);

    void setLight(const Light &light);

    void setShadowMap(
        ComPtr<ID3D11ShaderResourceView> sm,
        const Mat4                      &viewProj);

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

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;

    ConstantBuffer<VSTransform> vsTransform_;
    ConstantBuffer<Light>       psLight_;

    ShaderResourceViewSlot<PS> *psShadowMapSlot_ = nullptr;

    Mat4 lightViewProj_;
    Mat4 viewProj_;
};
