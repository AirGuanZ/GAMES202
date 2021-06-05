#include "./gbuffer.h"

void GBufferGenerator::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<VS>(
        "./asset/02/gbuffer.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/02/gbuffer.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    albedoSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("Albedo");
    normalSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("Normal");

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

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
            "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, tangent),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,
            0, offsetof(Vertex, texCoord),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
    };

    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    auto sampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_WRAP);
    shaderRscs_.getSamplerSlot<PS>("LinearSampler")->setSampler(sampler);

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

    resize(res);
}

void GBufferGenerator::resize(const Int2 &res)
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

void GBufferGenerator::setCamera(const Mat4 &view, const Mat4 &viewProj)
{
    view_         = view;
    viewProj_     = viewProj;
}

ComPtr<ID3D11ShaderResourceView> GBufferGenerator::getGBuffer(int index) const
{
    return renderTarget_.getColorShaderResourceView(index);
}

void GBufferGenerator::begin()
{
    renderTarget_.bind();
    renderTarget_.clearColorBuffer(0, { 0, 0, 0, 100 });
    renderTarget_.clearColorBuffer(1, { 0, 0, 0, 100 });
    renderTarget_.clearDepth(1);
    renderTarget_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext->RSSetState(rasState_.Get());

    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GBufferGenerator::end()
{
    albedoSlot_->setShaderResourceView(nullptr);
    normalSlot_->setShaderResourceView(nullptr);

    deviceContext.setInputLayout(nullptr);

    deviceContext->RSSetState(nullptr);

    shaderRscs_.unbind();
    shader_.unbind();

    renderTarget_.unbind();
}

void GBufferGenerator::render(const Mesh &mesh)
{
    vsTransform_.update({
        mesh.world * viewProj_,
        mesh.world * view_,
        mesh.world
    });

    albedoSlot_->setShaderResourceView(mesh.albedo);
    normalSlot_->setShaderResourceView(mesh.normal);
    
    albedoSlot_->bind();
    normalSlot_->bind();

    mesh.vertexBuffer.bind(0);
    deviceContext.draw(mesh.vertexBuffer.getVertexCount(), 0);
    mesh.vertexBuffer.unbind(0);
}
