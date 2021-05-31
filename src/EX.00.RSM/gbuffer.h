#pragma once

#include "./common.h"

// bufferA: worldPosition, normal0
// bufferB: color,         normal1
class GBuffer : public agz::misc::uncopyable_t
{
public:

    void initialize(const Int2 &res);

    void resize(const Int2 &res);

    void setCamera(const Mat4 &viewProj);

    ComPtr<ID3D11ShaderResourceView> getGBuffer(int index) const;

    void begin();

    void end();

    void render(const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world);

private:

    struct VSTransform
    {
        Mat4 WVP;
        Mat4 world;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;
    RenderTarget              renderTarget_;

    Mat4                        viewProj_;
    ConstantBuffer<VSTransform> vsTransform_;
};
