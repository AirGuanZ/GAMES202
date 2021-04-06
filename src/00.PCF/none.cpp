#include "./none.h"

void MeshRendererNoShadow::initialize()
{
    // shader

    shader_.initializeStageFromFile<VS>(
        "./asset/00/none.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/00/none.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    // vs transform

    vsTransformSlot_ = shaderRscs_.getConstantBufferSlot<VS>("VSTransform");
    vsTransform_.initialize();
    vsTransformSlot_->setBuffer(vsTransform_);

    // ps light

    psLightSlot_ = shaderRscs_.getConstantBufferSlot<PS>("PSLight");
    psLight_.initialize();
    psLightSlot_->setBuffer(psLight_);

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

void MeshRendererNoShadow::setCamera(const Mat4 &viewProj)
{
    viewProj_ = viewProj;
}

void MeshRendererNoShadow::setLight(const Light &light)
{
    psLight_.update(light);
}

void MeshRendererNoShadow::begin()
{
    shader_.bind();
    shaderRscs_.bind();
    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void MeshRendererNoShadow::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
}

void MeshRendererNoShadow::render(
    const VertexBuffer<MeshVertex> &vertexBuffer,
    const Mat4                     &world)
{
    vsTransform_.update({ world, world * viewProj_ });

    vertexBuffer.bind(0);
    deviceContext->DrawInstanced(vertexBuffer.getVertexCount(), 1, 0, 0);
    vertexBuffer.unbind(0);
}
