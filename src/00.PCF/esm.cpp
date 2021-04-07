#include "./esm.h"

void ExponentialShadowMapRenderer::initialize(const Int2 &resolution)
{
    // shader

    shader_.initializeStageFromFile<VS>("./asset/00/esm_sm.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>("./asset/00/esm_sm.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // vs transform

    auto vsTransformSlot = shaderRscs_.getConstantBufferSlot<VS>("VSTransform");
    vsTransform_.initialize();
    vsTransformSlot->setBuffer(vsTransform_);

    auto psParamSlot = shaderRscs_.getConstantBufferSlot<PS>("PSParam");
    psParam_.initialize();
    psParamSlot->setBuffer(psParam_);

    // input layout

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(MeshVertex, position),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };

    inputLayout_ = InputLayoutBuilder(inputElems)
        .build(shader_.getVertexShaderByteCode());

    // shadow map

    renderTarget_ = RenderTarget(resolution);
    renderTarget_.addDepthStencil(DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D32_FLOAT);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
}

ComPtr<ID3D11ShaderResourceView> ExponentialShadowMapRenderer::getSRV() const
{
    return renderTarget_.getColorShaderResourceView(0);
}

const Mat4 &ExponentialShadowMapRenderer::getLightViewProj() const
{
    return lightViewProj_;
}

void ExponentialShadowMapRenderer::setC(float c)
{
    psParamData_.c = c;
}

void ExponentialShadowMapRenderer::setLight(const Light &light)
{
    light_ = light;
    updateLightViewProj();
}

void ExponentialShadowMapRenderer::begin()
{
    psParam_.update(psParamData_);

    renderTarget_.clearDepth(1);
    renderTarget_.bind();
    shader_.bind();
    shaderRscs_.bind();
    deviceContext.setInputLayout(inputLayout_.Get());
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_VIEWPORT viewport = {
        0.0f, 0.0f,
        static_cast<float>(renderTarget_.getWidth()),
        static_cast<float>(renderTarget_.getHeight()),
        0.0f, 1.0f
    };
    deviceContext->RSSetViewports(1, &viewport);
}

void ExponentialShadowMapRenderer::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
    renderTarget_.unbind();
}

void ExponentialShadowMapRenderer::render(
    const VertexBuffer<MeshVertex> &vertexBuffer,
    const Mat4                     &world)
{
    vsTransform_.update({ world * lightViewProj_ });
    vertexBuffer.bind(0);
    deviceContext->DrawInstanced(vertexBuffer.getVertexCount(), 1, 0, 0);
    vertexBuffer.unbind(0);
}

void ExponentialShadowMapRenderer::updateLightViewProj()
{
    Float3 up;
    if(std::abs(std::abs(dot(light_.direction, { 1, 0, 0 })) - 1) < 0.1f)
        up = { 0, 0, 1 };
    else
        up = { 1, 0, 0 };

    const Mat4 view = Trans4::look_at(
        light_.position, light_.position + light_.direction, up);
    const Mat4 proj = Trans4::perspective(
        agz::math::deg2rad(
            (std::min)(180.0f, 0.1f + agz::math::rad2deg(
                2 * std::acos(light_.fadeCosEnd)))),
        static_cast<float>(renderTarget_.getWidth()) / renderTarget_.getHeight(),
        5.0f, 30.0f);

    lightViewProj_ = view * proj;
}

void MeshRendererESM::initialize()
{
    // shader

    shader_.initializeStageFromFile<VS>("./asset/00/esm_mesh.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>("./asset/00/esm_mesh.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // vs transform

    vsTransform_.initialize();
    auto vsTransformSlot = shaderRscs_.getConstantBufferSlot<VS>("VSTransform");
    vsTransformSlot->setBuffer(vsTransform_);

    // ps param

    auto psParamSlot = shaderRscs_.getConstantBufferSlot<PS>("PSParam");
    psParam_.initialize();
    psParamSlot->setBuffer(psParam_);

    // ps light

    psLight_.initialize();
    auto psLightSlot = shaderRscs_.getConstantBufferSlot<PS>("PSLight");
    psLightSlot->setBuffer(psLight_);

    // shadow map & sampler

    psShadowMapSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("ShadowMap");

    auto psShadowMapSamplerSlot =
        shaderRscs_.getSamplerSlot<PS>("ShadowMapSampler");

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW       = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.MipLODBias     = 0;
    samplerDesc.MaxAnisotropy  = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
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

void MeshRendererESM::setC(float c)
{
    psParamData_.c = c;
}

void MeshRendererESM::setCamera(const Mat4 &viewProj)
{
    viewProj_ = viewProj;
}

void MeshRendererESM::setLight(const Light &light)
{
    psLight_.update(light);
}

void MeshRendererESM::setShadowMap(
    ComPtr<ID3D11ShaderResourceView> sm,
    const Mat4                      &viewProj)
{
    psShadowMapSlot_->setShaderResourceView(std::move(sm));
    lightViewProj_ = viewProj;
}

void MeshRendererESM::begin()
{
    psParam_.update(psParamData_);

    shader_.bind();
    shaderRscs_.bind();
    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void MeshRendererESM::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
}

void MeshRendererESM::render(
    const VertexBuffer<MeshVertex> &vertexBuffer, const Mat4 &world)
{
    vsTransform_.update(
        { world, world * viewProj_, world * lightViewProj_ });
    vertexBuffer.bind(0);
    deviceContext->DrawInstanced(vertexBuffer.getVertexCount(), 1, 0, 0);
    vertexBuffer.unbind(0);
}
