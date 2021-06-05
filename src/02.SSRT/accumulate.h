#pragma once

#include "./common.h"

class IndirectAccumulator
{
public:

    void initialize(const Int2 &res);

    void resize(const Int2 &res);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

    // accu = alpha * new + (1 - alpha) * accu
    void setFactor(float alpha);

    void setCamera(const Mat4 &lastViewProj);

    void accumulate(
        ComPtr<ID3D11ShaderResourceView> lastGBufferA,
        ComPtr<ID3D11ShaderResourceView> thisGBufferA,
        ComPtr<ID3D11ShaderResourceView> indirect);

private:

    struct CSParams
    {
        Mat4  lastViewProj;
        float alpha;
        float pad0;
        float pad1;
        float pad2;
    };

    Shader<CS>         shader_;
    Shader<CS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<CS> *lastGBufferASlot_ = nullptr;
    ShaderResourceViewSlot<CS> *thisGBufferASlot_ = nullptr;

    ShaderResourceViewSlot<CS> *accuIndirectSlot_ = nullptr;
    ShaderResourceViewSlot<CS> *newIndirectSlot_  = nullptr;

    UnorderedAccessViewSlot<CS> *accuOutputSlot_ = nullptr;

    CSParams                 csParamsData_ = {};
    ConstantBuffer<CSParams> csParams_;

    // swap a and b just before rendering
    // render: read A, write B

    Int2 res_;

    ComPtr<ID3D11ShaderResourceView>  srvA_;
    ComPtr<ID3D11ShaderResourceView>  srvB_;
    ComPtr<ID3D11UnorderedAccessView> uavA_;
    ComPtr<ID3D11UnorderedAccessView> uavB_;
};
