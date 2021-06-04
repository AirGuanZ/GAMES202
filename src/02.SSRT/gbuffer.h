#pragma once

#include "./common.h"

// 3: position, 1: normal1
// 2: color, 1: viewDepth, 1: normal2
class GBufferGenerator
{
public:

    void initialize(const Int2 &res);

    void resize(const Int2 &res);

    void setCamera(const Mat4 &view, const Mat4 &viewProj);

    ComPtr<ID3D11ShaderResourceView> getGBuffer(int index) const;

    void begin();

    void end();

    void render(const Mesh &mesh);

private:

    struct VSTransform
    {
        Mat4 WVP;
        Mat4 WV;
        Mat4 W;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *albedoSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *normalSlot_ = nullptr;

    ComPtr<ID3D11RasterizerState> rasState_;

    ComPtr<ID3D11InputLayout> inputLayout_;
    RenderTarget              renderTarget_;

    Mat4                        view_;
    Mat4                        viewProj_;
    ConstantBuffer<VSTransform> vsTransform_;
};
