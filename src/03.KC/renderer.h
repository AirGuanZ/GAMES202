#pragma once

#include <common/camera.h>

class Renderer : public agz::misc::uncopyable_t
{
public:

    struct Vertex
    {
        Float3 position;
        Float3 normal;
    };

    void initialize();

    void setCamera(const Camera &camera);

    void setLight(const Float3 &direction, const Float3 &intensity);

    void setKCModel(bool enabled);

    void begin();

    void end();

    void render(
        const VertexBuffer<Vertex> &vertexBuffer,
        const Mat4                 &world,
        const Float3               &R0,
        const Float3               &edgeTint,
        float                       roughness);

private:

    struct VSTransform
    {
        Mat4 WVP;
        Mat4 world;
    };

    struct PSParams
    {
        Float3 R0;             float roughness;
        Float3 eye;            float edgeTintR;
        Float3 lightDirection; float edgeTintG;
        Float3 lightIntensity; float edgeTintB;
    };

    bool enableKC_ = true;

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;

    Mat4                        viewProj_;
    ConstantBuffer<VSTransform> vsTransform_;

    PSParams                 psParamsData_ = {};
    ConstantBuffer<PSParams> psParams_;
};
