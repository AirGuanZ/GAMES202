#include "./rsm.h"

void RSMGenerator::initialize(const Int2 &res)
{
    // shader

    shader_.initializeStageFromFile<VS>(
        "./asset/ex.00/rsm.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/ex.00/rsm.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // input layout

    D3D11_INPUT_ELEMENT_DESC inputElems[] = {
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

    renderTarget_ = RenderTarget(res);

    renderTarget_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
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

    // constant buffer

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

    psLight_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSLight")
        ->setBuffer(psLight_);
}

void RSMGenerator::setLight(
    const DirectionalLight &light,
    const Mat4             &lightViewProj)
{
    lightViewProj_     = lightViewProj;
    psLightData_.light = light;
}

ComPtr<ID3D11ShaderResourceView> RSMGenerator::getRSM(int index) const
{
    return renderTarget_.getColorShaderResourceView(index);
}

void RSMGenerator::begin()
{
    psLight_.update(psLightData_);

    renderTarget_.bind();
    renderTarget_.clearColorBuffer(0, { 0, 0, 0, 1 });
    renderTarget_.clearColorBuffer(1, { 0, 0, 0, 0 });
    renderTarget_.clearColorBuffer(2, { 0, 0, 0, 0 });
    renderTarget_.clearDepth(1);
    renderTarget_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();
    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void RSMGenerator::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
    renderTarget_.unbind();
}

void RSMGenerator::render(
    const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world)
{
    vertexBuffer.bind(0);
    vsTransform_.update({ world * lightViewProj_, world });
    deviceContext.draw(vertexBuffer.getVertexCount(), 0);
    vertexBuffer.unbind(0);
}
