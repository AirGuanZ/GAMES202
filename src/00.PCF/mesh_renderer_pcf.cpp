#include "./mesh_renderer_pcf.h"

void MeshRendererPCF::initialize()
{
    // shader

    shader_.initializeStageFromFile<VS>(
        "./asset/00/pcf.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/00/pcf.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // vs transform

    vsTransformSlot_ = shaderRscs_.getConstantBufferSlot<VS>("VSTransform");
    vsTransform_.initialize();
    vsTransformSlot_->setBuffer(vsTransform_);

    // ps light

    psLightSlot_ = shaderRscs_.getConstantBufferSlot<PS>("PSLight");
    psLight_.initialize();
    psLightSlot_->setBuffer(psLight_);

    // ps shadow map

    psShadowMapConstSlot_ = shaderRscs_.getConstantBufferSlot<PS>("PSShadowMap");
    psShadowMapConst_.initialize();
    psShadowMapConstSlot_->setBuffer(psShadowMapConst_);

    psShadowMapConstData_.sampleCount  = 1;
    psShadowMapConstData_.filterRadius = 1;

    // shadow map & sampler

    psShadowMapSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("ShadowMap");

    auto psShadowMapSamplerSlot =
        shaderRscs_.getSamplerSlot<PS>("ShadowMapSampler");

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.MipLODBias     = 0;
    samplerDesc.MaxAnisotropy  = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    samplerDesc.BorderColor[0] = 1;
    samplerDesc.BorderColor[1] = 1;
    samplerDesc.BorderColor[2] = 1;
    samplerDesc.BorderColor[3] = 1;
    samplerDesc.MinLOD         = -FLT_MAX;
    samplerDesc.MaxLOD         = +FLT_MAX;
    
    psShadowMapSamplerSlot->setSampler(device.createSampler(samplerDesc));

    // input layout

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(MeshVertex, position),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(MeshVertex, normal),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };

    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);
}

void MeshRendererPCF::setCamera(const Mat4 &viewProj)
{
    viewProj_ = viewProj;
}

void MeshRendererPCF::setLight(const Light &light)
{
    psLight_.update(light);
}

void MeshRendererPCF::setFilter(float filterRadius, int sampleCount)
{
    psShadowMapConstData_.filterRadius = filterRadius;
    psShadowMapConstData_.sampleCount  = sampleCount;
}

void MeshRendererPCF::setShadowMap(
    ComPtr<ID3D11ShaderResourceView> sm, const Mat4 &viewProj)
{
    ComPtr<ID3D11Resource> rsc;
    sm->GetResource(rsc.GetAddressOf());

    ComPtr<ID3D11Texture2D> tex;
    rsc->QueryInterface(IID_PPV_ARGS(tex.GetAddressOf()));

    D3D11_TEXTURE2D_DESC desc;
    tex->GetDesc(&desc);

    psShadowMapConstData_.textureResolution = static_cast<float>(desc.Width);
    psShadowMapConstData_.lightViewProj     = viewProj;

    psShadowMapSlot_->setShaderResourceView(std::move(sm));
}

void MeshRendererPCF::begin()
{
    psShadowMapConst_.update({ psShadowMapConstData_ });

    shader_.bind();
    shaderRscs_.bind();
    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void MeshRendererPCF::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
}

void MeshRendererPCF::render(
    const VertexBuffer<MeshVertex> &vertexBuffer, const Mat4 &world)
{
    vsTransform_.update({ world, world * viewProj_ });

    vertexBuffer.bind(0);
    deviceContext->DrawInstanced(vertexBuffer.getVertexCount(), 1, 0, 0);
    vertexBuffer.unbind(0);
}
