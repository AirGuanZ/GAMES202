#pragma once

#include "./common.h"

// bufferA: worldPosition, 1
// bufferB: worldNormal, 1
// bufferC: flux, 1
class RSMGenerator : public agz::misc::uncopyable_t
{
public:

    void initialize(const Int2 &res);

    void setLight(
        const DirectionalLight &light,
        const Mat4             &lightViewProj);

    ComPtr<ID3D11ShaderResourceView> getRSM(int index) const;

    void begin();

    void end();

    void render(const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world);

private:

    struct VSTransform
    {
        Mat4 WVP;
        Mat4 world;
    };

    struct PSLight
    {
        DirectionalLight light;
    };
    
    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;
    RenderTarget              renderTarget_;

    Mat4                        lightViewProj_;
    ConstantBuffer<VSTransform> vsTransform_;

    PSLight                 psLightData_ = {};
    ConstantBuffer<PSLight> psLight_;
};
