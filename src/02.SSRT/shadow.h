#pragma once

#include "./common.h"

// worldPosition, normalA
// albedo / PI,   normalB
class ShadowMapRenderer : public agz::misc::uncopyable_t
{
public:

    void initialize(const Int2 &res);
    
    void setLight(const Mat4 &lightViewProj);

    ComPtr<ID3D11ShaderResourceView> getShadowMap() const;

    void begin();

    void end();

    void render(
        const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world);

private:
    
    struct VSTransform
    {
        Mat4 WVP;
    };

    ComPtr<ID3D11InputLayout> inputLayout_;
    RenderTarget              renderTarget_;

    ComPtr<ID3D11RasterizerState> rasState_;

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;
    
    ConstantBuffer<VSTransform> vsTransform_;

    Mat4 lightViewProj_;
};
