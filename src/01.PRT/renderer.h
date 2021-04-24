#pragma once

#include <common/common.h>

class Renderer : public agz::misc::uncopyable_t
{
public:

    void initialize();

    void setSH(int SHCount);

    void setVertices(
        const Float3 *vertices,
        const float  *vertexSHCoefs,
        int           vertexCount);

    void setLight(const Float3 *coefs);

    void render(const Mat4 &viewProj);

private:

    struct VSTransform
    {
        Mat4 viewProj;
    };

    struct VSEnvSH
    {
        int    count;
        float  pad[3] = { 0, 0, 0 };
        Float4 envSH[25];
    };

    static_assert(sizeof(VSEnvSH) == sizeof(Float4) * 26);

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;
    VertexBuffer<Float3>      vertexBuffer_;
    
    ShaderResourceViewSlot<VS> *vertexSHCoefsSlot_ = nullptr;

    ConstantBuffer<VSTransform> vsTransform_;

    VSEnvSH                 vsEnvSHData_ = {};
    ConstantBuffer<VSEnvSH> vsEnvSH_;
};
