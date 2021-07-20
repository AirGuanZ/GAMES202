#include "lut.h"
#include "renderer.h"

void Renderer::initialize()
{
    shader_.initializeStageFromFile<VS>(
        "./asset/03/render.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/03/render.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

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
        }
    };
    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

    psParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSParams")
        ->setBuffer(psParams_);

    const auto lut = LUTGenerator().generate({ 128, 128 }, 512);
    shaderRscs_.getShaderResourceViewSlot<PS>("EMiu")
        ->setShaderResourceView(lut.Emiu);
    shaderRscs_.getShaderResourceViewSlot<PS>("EAvg")
        ->setShaderResourceView(lut.Eavg);

    auto sampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("ESampler")
        ->setSampler(sampler);
}

void Renderer::setCamera(const Camera &camera)
{
    viewProj_         = camera.getViewProj();
    psParamsData_.eye = camera.getPosition();
}

void Renderer::setLight(const Float3 &direction, const Float3 &intensity)
{
    psParamsData_.lightDirection = direction;
    psParamsData_.lightIntensity = intensity;
}

void Renderer::setKCModel(bool enabled)
{
    enableKC_ = enabled;
}

void Renderer::begin()
{
    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Renderer::end()
{
    deviceContext.setInputLayout(nullptr);

    shaderRscs_.unbind();
    shader_.unbind();
}

void Renderer::render(
    const VertexBuffer<Vertex> &vertexBuffer,
    const Mat4                 &world,
    const Float3               &R0,
    const Float3               &edgeTint,
    float                       roughness)
{
    vsTransform_.update({ world * viewProj_, world });

    psParamsData_.R0        = R0;
    psParamsData_.roughness = roughness;
    psParamsData_.edgeTintR = enableKC_ ? edgeTint.x : -1.0f;
    psParamsData_.edgeTintG = edgeTint.y;
    psParamsData_.edgeTintB = edgeTint.z;
    psParams_.update(psParamsData_);

    vertexBuffer.bind(0);
    deviceContext.draw(vertexBuffer.getVertexCount(), 0);
    vertexBuffer.unbind(0);
}
