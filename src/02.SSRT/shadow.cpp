#include "./shadow.h"

void ShadowMapRenderer::initialize(const Int2 &res)
{
    // shader

    shader_.initializeStageFromFile<VS>(
        "./asset/02/shadow.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/02/shadow.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // vs transform

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

    // input layout

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, position),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };

    inputLayout_ = InputLayoutBuilder(inputElems)
        .build(shader_.getVertexShaderByteCode());

    // shadow map

    renderTarget_ = RenderTarget(res);
    renderTarget_.addDepthStencil(
        DXGI_FORMAT_R32_TYPELESS,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_R32_FLOAT);

    // ras state

    D3D11_RASTERIZER_DESC rasDesc;
    rasDesc.FillMode              = D3D11_FILL_SOLID;
    rasDesc.CullMode              = D3D11_CULL_NONE;
    rasDesc.FrontCounterClockwise = false;
    rasDesc.DepthBias             = 0;
    rasDesc.DepthBiasClamp        = 0;
    rasDesc.SlopeScaledDepthBias  = 0;
    rasDesc.DepthClipEnable       = true;
    rasDesc.ScissorEnable         = false;
    rasDesc.MultisampleEnable     = false;
    rasDesc.AntialiasedLineEnable = false;

    rasState_ = device.createRasterizerState(rasDesc);
}

void ShadowMapRenderer::setLight(const Mat4 &lightViewProj)
{
    lightViewProj_ = lightViewProj;
}

ComPtr<ID3D11ShaderResourceView> ShadowMapRenderer::getShadowMap() const
{
    return renderTarget_.getDepthShaderResourceView();
}

void ShadowMapRenderer::begin()
{
    renderTarget_.bind();
    renderTarget_.useDefaultViewport();
    renderTarget_.clearDepth(1);

    shader_.bind();
    shaderRscs_.bind();

    deviceContext->RSSetState(rasState_.Get());

    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShadowMapRenderer::end()
{
    deviceContext.setInputLayout(nullptr);

    deviceContext->RSSetState(nullptr);

    shaderRscs_.unbind();
    shader_.unbind();

    renderTarget_.unbind();
}

void ShadowMapRenderer::render(
    const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world)
{
    vsTransform_.update({ world * lightViewProj_ });
    vertexBuffer.bind(0);
    deviceContext.draw(vertexBuffer.getVertexCount(), 0);
    vertexBuffer.unbind(0);
}
