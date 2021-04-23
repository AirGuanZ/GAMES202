#include "./sm.h"

void ShadowMapRenderer::initialize(const Int2 &resolution)
{
    // shader

    shader_.initializeStageFromFile<VS>("./asset/00/sm.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>("./asset/00/sm.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // vs transform

    vsTransformSlot_ = shaderRscs_.getConstantBufferSlot<VS>("VSTransform");
    vsTransform_.initialize();
    vsTransformSlot_->setBuffer(vsTransform_);

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
    renderTarget_.addDepthStencil(
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_D32_FLOAT);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_FLOAT,
        DXGI_FORMAT_R32_FLOAT);
}

ComPtr<ID3D11ShaderResourceView> ShadowMapRenderer::getSRV() const
{
    return renderTarget_.getColorShaderResourceView(0);
}

const Mat4 &ShadowMapRenderer::getLightViewProj() const
{
    return lightViewProj_;
}

float ShadowMapRenderer::getNearPlane() const
{
    return 5.0f;
}

void ShadowMapRenderer::setLight(const Light &light)
{
    light_ = light;
    updateLightViewProj();
}

void ShadowMapRenderer::begin()
{
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

void ShadowMapRenderer::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
    renderTarget_.unbind();
}

void ShadowMapRenderer::render(
    const VertexBuffer<MeshVertex> &vertexBuffer,
    const Mat4                     &world)
{
    vsTransform_.update({ world * lightViewProj_ });
    vertexBuffer.bind(0);
    deviceContext->DrawInstanced(vertexBuffer.getVertexCount(), 1, 0, 0);
    vertexBuffer.unbind(0);
}

void ShadowMapRenderer::updateLightViewProj()
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
