#include "renderer.h"

void Renderer::initialize()
{
    // shader

    shader_.initializeStageFromFile<VS>("./asset/01/render.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>("./asset/01/render.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    vertexSHCoefsSlot_ =
        shaderRscs_.getShaderResourceViewSlot<VS>("VertexSHCoefs");

    // input layout

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };

    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    // constant buffer

    vsTransform_.initialize();
    auto vsTransformSlot = shaderRscs_.getConstantBufferSlot<VS>("VSTransform");
    vsTransformSlot->setBuffer(vsTransform_);

    vsEnvSH_.initialize();
    auto vsEnvSHSlot = shaderRscs_.getConstantBufferSlot<VS>("VSEnvSH");
    vsEnvSHSlot->setBuffer(vsEnvSH_);
}

void Renderer::setSH(int SHCount)
{
    vsEnvSHData_.count = SHCount;
}

void Renderer::setVertices(
    const Float3 *vertices,
    const float  *vertexSHCoefs,
    int           vertexCount)
{
    vertexBuffer_.initialize(vertexCount, vertices);

    D3D11_BUFFER_DESC SHCoefBufDesc;
    SHCoefBufDesc.ByteWidth           = sizeof(float) * vsEnvSHData_.count * vertexCount;
    SHCoefBufDesc.Usage               = D3D11_USAGE_IMMUTABLE;
    SHCoefBufDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    SHCoefBufDesc.CPUAccessFlags      = 0;
    SHCoefBufDesc.MiscFlags           = 0;
    SHCoefBufDesc.StructureByteStride = sizeof(float);

    auto coefBuffer = createBuffer(SHCoefBufDesc, vertexSHCoefs);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format               = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension        = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement  = 0;
    srvDesc.Buffer.NumElements   = vsEnvSHData_.count * vertexCount;

    auto vertexSHCoefSRV = device.createSRV(coefBuffer.Get(), srvDesc);

    vertexSHCoefsSlot_->setShaderResourceView(vertexSHCoefSRV);
}

void Renderer::setLight(const Float3 *coefs)
{
    for(int i = 0; i < vsEnvSHData_.count; ++i)
    {
        vsEnvSHData_.envSH[i].x = coefs[i][0];
        vsEnvSHData_.envSH[i].y = coefs[i][1];
        vsEnvSHData_.envSH[i].z = coefs[i][2];
    }
}

void Renderer::render(const Mat4 &viewProj)
{
    vsTransform_.update({ viewProj });
    vsEnvSH_.update(vsEnvSHData_);

    shader_.bind();
    shaderRscs_.bind();
    vertexBuffer_.bind(0);
    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    deviceContext->Draw(vertexBuffer_.getVertexCount(), 0);

    vertexBuffer_.unbind(0);
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
}
