#include "./pom.h"

void ParallaxOcclusionMappingRenderer::initialize()
{
    shader_.initializeStageFromFile<VS>(
        "./asset/EX.01/pom.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/EX.01/pom.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    heightSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("Height");
    albedoSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("Albedo");

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
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
        {
            "DTANGENT", 0, DXGI_FORMAT_R32_FLOAT,
            0, offsetof(Vertex, dTangent),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "DBINORMAL", 0, DXGI_FORMAT_R32_FLOAT,
            0, offsetof(Vertex, dBinormal),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };
    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

    psPerFrame_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSPerFrame")
        ->setBuffer(psPerFrame_);

    psHeight_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSHeight")
        ->setBuffer(psHeight_);

    auto linearSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("LinearSampler")->setSampler(linearSampler);

    auto pointSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("PointSampler")->setSampler(pointSampler);
}

void ParallaxOcclusionMappingRenderer::setCamera(const Float3 &eye, const Mat4 &viewProj)
{
    psPerFrameData_.eye = eye;
    viewProj_           = viewProj;
}

void ParallaxOcclusionMappingRenderer::setLight(const DirectionalLight &light)
{
    psPerFrameData_.light = light;
}

void ParallaxOcclusionMappingRenderer::setTracer(
    float linearStep, int maxLinearStepCount)
{
    psPerFrameData_.linearStep         = linearStep;
    psPerFrameData_.maxLinearStepCount = maxLinearStepCount;
}

void ParallaxOcclusionMappingRenderer::setDiffStep(int step)
{
    psPerFrameData_.diffStep = static_cast<float>(step);
}

void ParallaxOcclusionMappingRenderer::begin()
{
    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ParallaxOcclusionMappingRenderer::end()
{
    psPerFrame_.update(psPerFrameData_);

    heightSlot_->setShaderResourceView(nullptr);
    albedoSlot_->setShaderResourceView(nullptr);

    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
}

void ParallaxOcclusionMappingRenderer::render(const Mesh &mesh)
{
    vsTransform_.update({ mesh.world * viewProj_, mesh.world });
    psHeight_.update({ mesh.heightScale, 0, 0, 0 });

    heightSlot_->setShaderResourceView(mesh.height);
    albedoSlot_->setShaderResourceView(mesh.albedo);

    heightSlot_->bind();
    albedoSlot_->bind();

    mesh.vertexBuffer.bind(0);
    deviceContext.draw(mesh.vertexBuffer.getVertexCount(), 0);
    mesh.vertexBuffer.unbind(0);
}
