#pragma once

#include <common/common.h>

class Renderer : public agz::misc::uncopyable_t
{
public:

    struct Vertex
    {
        Float3   position;
        uint32_t id;
    };

    void initialize();

    void setSH(int SHCount);

    void setVertices(
        const Vertex *vertices,
        const float  *vertexSHCoefs,
        int           vertexCount);

    void setLight(const float *coefs);

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
        Float4 envSH[16];
    };

    static_assert(sizeof(VSEnvSH) == sizeof(Float4) * 17);

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;
    VertexBuffer<Vertex>      vertexBuffer_;
    
    ShaderResourceViewSlot<VS> *vertexSHCoefsSlot_ = nullptr;

    ConstantBuffer<VSTransform> vsTransform_;

    VSEnvSH                 vsEnvSHData_ = {};
    ConstantBuffer<VSEnvSH> vsEnvSH_;
};
