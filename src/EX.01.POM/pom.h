#pragma once

#include "./common.h"

class ParallaxOcclusionMappingRenderer
{
public:

    void initialize();

    void setCamera(const Float3 &eye, const Mat4 &viewProj);

    void setLight(const DirectionalLight &light);

    void setTracer(float linearStep, int maxLinearStepCount);

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

    struct PSPerFrame
    {
        DirectionalLight light;

        Float3 eye;
        float  diffStep;

        float linearStep;
        int   maxLinearStepCount;
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

    PSPerFrame                 psPerFrameData_ = {};
    ConstantBuffer<PSPerFrame> psPerFrame_;
    
    ConstantBuffer<PSHeight> psHeight_;
};
