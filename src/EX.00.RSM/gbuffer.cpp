#include "./gbuffer.h"

void GBuffer::initialize(const Int2 &res)
{
    // shader

    shader_.initializeStageFromFile<VS>(
        "./asset/EX.00/gbuffer.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/EX.00/gbuffer.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // input layout

    const D3D11_INPUT_ELEMENT_DESC inputElems[] =
    {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, position),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, normal),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "ALBEDO", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, albedo),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };

    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    // render target

    resize(res);

    // constant buffer

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);
}

void GBuffer::resize(const Int2 &res)
{
    renderTarget_ = RenderTarget(res);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
    renderTarget_.addDepthStencil(
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_D32_FLOAT);
}

void GBuffer::setCamera(const Mat4 &viewProj)
{
    viewProj_ = viewProj;
}

ComPtr<ID3D11ShaderResourceView> GBuffer::getGBuffer(int index) const
{
    return renderTarget_.getColorShaderResourceView(index);
}

void GBuffer::begin()
{
    renderTarget_.bind();
    renderTarget_.clearColorBuffer(0, { 0, 0, 0, 100 });
    renderTarget_.clearColorBuffer(1, { 0, 0, 0, 100 });
    renderTarget_.clearDepth(1);
    renderTarget_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GBuffer::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
    renderTarget_.unbind();
}

void GBuffer::render(
    const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world)
{
    vertexBuffer.bind(0);
    vsTransform_.update({ world * viewProj_, world });
    deviceContext.draw(vertexBuffer.getVertexCount(), 0);
    vertexBuffer.unbind(0);
}
