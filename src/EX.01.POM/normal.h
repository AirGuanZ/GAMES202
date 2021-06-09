#pragma once

#include "./common.h"

class NormalMappingRenderer
{
public:

    void initialize();

    void setCamera(const Mat4 &viewProj);

    void setLight(const DirectionalLight &light);

    void setDiffStep(int step);

    void begin();

    void end();

    void render(const Mesh &mesh);

private:

    struct VSTransform
    {
        Mat4 WVP;
        Mat4 world;
    };

    struct PSLight
    {
        DirectionalLight light;

        float diffStep;
        float pad0;
        float pad1;
        float pad2;
    };

    struct PSHeight
    {
        float heightScale;
        float pad0;
        float pad1;
        float pad2;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *heightSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *albedoSlot_ = nullptr;

    ComPtr<ID3D11InputLayout> inputLayout_;

    Mat4                        viewProj_;
    ConstantBuffer<VSTransform> vsTransform_;

    PSLight                 psLightData_ = {};
    ConstantBuffer<PSLight> psLight_;
    
    ConstantBuffer<PSHeight> psHeight_;
};
