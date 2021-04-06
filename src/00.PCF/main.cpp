#include <agz-utils/mesh.h>
#include <agz-utils/time.h>

#include <common/application.h>
#include <common/camera.h>

#include "./blur.h"
#include "./hard.h"
#include "./none.h"
#include "./pcf.h"
#include "./pcss.h"
#include "./sm.h"
#include "./vsm.h"

class ShadowMapApplication : public Application
{
public:

    using Application::Application;

    enum class ShadowAlgorithmType : int
    {
        NoShadow,
        Hard,
        PCF,
        PCSS,
        VSM,
    };

protected:

    void initialize() override;

    bool frame() override;

    void destroy() override;

private:

    struct Mesh
    {
        VertexBuffer<MeshVertex> vertexBuffer;
    };

    void frameNoShadow(const Light &light, bool gui);

    void frameHard(const Light &light, bool gui);

    void framePCF(const Light &light, bool gui);

    void framePCSS(const Light &light, bool gui);

    void frameVSM(const Light &light, bool gui);

    void updateCamera();

    Light getLight();

    Mesh loadMesh(const std::string &filename) const;

    Camera camera_;

    std::vector<Mesh> meshes_;

    ShadowAlgorithmType algoType_ = ShadowAlgorithmType::Hard;

    // algos based on regular shadow map

    ShadowMapRenderer shadowMapRenderer_;

    MeshRendererHardShadow hardShadow_;
    MeshRendererNoShadow   noShadow_;

    MeshRendererPCF PCF_;
    float PCFSampleRadius_ = 10;
    int   PCFSampleCount_  = 20;

    MeshRendererPCSS PCSS_;
    float PCSSLightRadiusOnShadowMap_ = 0.04f;
    int   PCSSBlockSearchSampleCount_ = 18;
    int   PCSSSampleCount_            = 27;

    // vsm

    VarianceShadowMapRenderer varianceShadowMapRenderer_;
    GaussianBlur              gaussianBlur_;
    MeshRendererVSM           VSM_;

    int VSMBlurRadius_  = 5;
    float VSMBlurSigma_ = 2;
    
    bool  rotateLight_ = true;
    float lightAngle_  = 0;

    agz::time::fps_counter_t fps_;
};

void ShadowMapApplication::initialize()
{
    shadowMapRenderer_.initialize({ 2048, 2048 });

    hardShadow_.initialize();
    noShadow_.initialize();

    PCF_.initialize();
    PCF_.setFilter(PCFSampleRadius_, PCFSampleCount_);

    PCSS_.initialize();
    PCSS_.setFilter(PCSSBlockSearchSampleCount_, PCSSSampleCount_);

    varianceShadowMapRenderer_.initialize({ 2048, 2048 });
    gaussianBlur_.initialize({ 2048, 2048 });
    gaussianBlur_.setFilter(VSMBlurRadius_, VSMBlurSigma_);
    VSM_.initialize();

    meshes_.push_back(loadMesh("./asset/202.obj"));
    meshes_.push_back(loadMesh("./asset/ground.obj"));

    camera_.setPosition({ 0, 2, 3 });
    camera_.setDirection(-3.14159f * 0.5f, 0);
    camera_.setPerspective(60.0f, 0.1f, 100.0f);

    mouse_->setCursorLock(
        true, window_->getClientWidth() / 2, window_->getClientHeight() / 2);
    mouse_->showCursor(false);
    window_->doEvents();

    fps_.restart();
}

bool ShadowMapApplication::frame()
{
    window_->doEvents();
    window_->newImGuiFrame();

    if(window_->getKeyboard()->isDown(KEY_ESCAPE))
        window_->setCloseFlag(true);

    if(keyboard_->isDown(KEY_LCTRL))
    {
        mouse_->showCursor(!mouse_->isVisible());
        mouse_->setCursorLock(
            !mouse_->isLocked(), mouse_->getLockX(), mouse_->getLockY());
    }

    updateCamera();

    bool gui = ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if(gui)
    {
        ImGui::Text("FPS: %d", fps_.fps());

        ImGui::Checkbox("Rotate Light", &rotateLight_);
        ImGui::SliderFloat("Light Angle", &lightAngle_, 0, 360);

        const char *algoNames[] = {
            "None",
            "Hard",
            "PCF",
            "PCSS",
            "VSM"
        };
        if(ImGui::BeginCombo("Type", algoNames[static_cast<int>(algoType_)]))
        {
            for(int i = 0; i < agz::array_size<int>(algoNames); ++i)
            {
                bool selected = i == static_cast<int>(algoType_);
                if(ImGui::Selectable(algoNames[i], selected))
                    algoType_ = static_cast<ShadowAlgorithmType>(i);
            }
            ImGui::EndCombo();
        }
    }

    const Light light = getLight();

    switch(algoType_)
    {
    case ShadowAlgorithmType::NoShadow:
        frameNoShadow(light, gui);
        break;
    case ShadowAlgorithmType::Hard:
        frameHard(light, gui);
        break;
    case ShadowAlgorithmType::PCF:
        framePCF(light, gui);
        break;
    case ShadowAlgorithmType::PCSS:
        framePCSS(light, gui);
        break;
    case ShadowAlgorithmType::VSM:
        frameVSM(light, gui);
        break;
    }

    ImGui::End();

    window_->renderImGui();
    window_->swapBuffers();

    fps_.frame_end();

    return !window_->getCloseFlag();
}

void ShadowMapApplication::destroy()
{

}

void ShadowMapApplication::frameNoShadow(const Light &light, bool gui)
{
    window_->useDefaultRTVAndDSV();
    window_->useDefaultViewport();
    window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
    window_->clearDefaultDepth(1.0f);

    noShadow_.setCamera(camera_.getViewProj());
    noShadow_.setLight(light);

    noShadow_.begin();
    for(auto &mesh : meshes_)
        noShadow_.render(mesh.vertexBuffer, Mat4::identity());
    noShadow_.end();
}

void ShadowMapApplication::frameHard(const Light &light, bool gui)
{
    shadowMapRenderer_.setLight(light);
    shadowMapRenderer_.begin();
    for(auto &mesh : meshes_)
        shadowMapRenderer_.render(mesh.vertexBuffer, Mat4::identity());
    shadowMapRenderer_.end();

    window_->useDefaultRTVAndDSV();
    window_->useDefaultViewport();
    window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
    window_->clearDefaultDepth(1.0f);

    hardShadow_.setCamera(camera_.getViewProj());
    hardShadow_.setLight(light);
    hardShadow_.setShadowMap(
        shadowMapRenderer_.getSRV(), shadowMapRenderer_.getLightViewProj());

    hardShadow_.begin();
    for(auto &mesh : meshes_)
        hardShadow_.render(mesh.vertexBuffer, Mat4::identity());
    hardShadow_.end();
}

void ShadowMapApplication::framePCF(const Light &light, bool gui)
{
    if(gui)
    {
        if(ImGui::SliderFloat("Sample Radius", &PCFSampleRadius_, 0, 100))
            PCF_.setFilter(PCFSampleRadius_, PCFSampleCount_);

        if(ImGui::InputInt("Sample Count", &PCFSampleCount_))
        {
            PCFSampleCount_ = agz::math::clamp(PCFSampleCount_, 1, 1000);
            PCF_.setFilter(PCFSampleRadius_, PCFSampleCount_);
        }
    }

    shadowMapRenderer_.setLight(light);
    shadowMapRenderer_.begin();
    for(auto &mesh : meshes_)
        shadowMapRenderer_.render(mesh.vertexBuffer, Mat4::identity());
    shadowMapRenderer_.end();

    window_->useDefaultRTVAndDSV();
    window_->useDefaultViewport();
    window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
    window_->clearDefaultDepth(1.0f);

    PCF_.setCamera(camera_.getViewProj());
    PCF_.setLight(light);
    PCF_.setShadowMap(
        shadowMapRenderer_.getSRV(), shadowMapRenderer_.getLightViewProj());

    PCF_.begin();
    for(auto &mesh : meshes_)
        PCF_.render(mesh.vertexBuffer, Mat4::identity());
    PCF_.end();
}

void ShadowMapApplication::framePCSS(const Light &light, bool gui)
{
    if(gui)
    {
        if(ImGui::InputFloat("Light Radius UV", &PCSSLightRadiusOnShadowMap_))
        {
            PCSSLightRadiusOnShadowMap_ =
                (std::max)(PCSSLightRadiusOnShadowMap_, 0.0f);
        }

        if(ImGui::InputInt("Blocker Sample Count", &PCSSBlockSearchSampleCount_))
        {
            PCSSBlockSearchSampleCount_ =
                agz::math::clamp(PCSSBlockSearchSampleCount_, 1, 1000);
            PCSS_.setFilter(PCSSBlockSearchSampleCount_, PCSSSampleCount_);
        }

        if(ImGui::InputInt("Sample Count", &PCSSSampleCount_))
        {
            PCSSSampleCount_ = agz::math::clamp(PCSSSampleCount_, 1, 1000);
            PCSS_.setFilter(PCSSBlockSearchSampleCount_, PCSSSampleCount_);
        }
    }

    shadowMapRenderer_.setLight(light);
    shadowMapRenderer_.begin();
    for(auto &mesh : meshes_)
        shadowMapRenderer_.render(mesh.vertexBuffer, Mat4::identity());
    shadowMapRenderer_.end();

    window_->useDefaultRTVAndDSV();
    window_->useDefaultViewport();
    window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
    window_->clearDefaultDepth(1.0f);

    PCSS_.setCamera(camera_.getViewProj());
    PCSS_.setLight(light);
    PCSS_.setShadowMap(
        shadowMapRenderer_.getSRV(),
        shadowMapRenderer_.getLightViewProj(),
        shadowMapRenderer_.getNearPlane(),
        PCSSLightRadiusOnShadowMap_);

    PCSS_.begin();
    for(auto &mesh : meshes_)
        PCSS_.render(mesh.vertexBuffer, Mat4::identity());
    PCSS_.end();
}

void ShadowMapApplication::frameVSM(const Light &light, bool gui)
{
    if(gui)
    {
        if(ImGui::SliderInt("Blur Radius", &VSMBlurRadius_, 0, 15))
            gaussianBlur_.setFilter(VSMBlurRadius_, VSMBlurSigma_);
        if(ImGui::SliderFloat("Blur Sigma", &VSMBlurSigma_, 0.01f, 20))
            gaussianBlur_.setFilter(VSMBlurRadius_, VSMBlurSigma_);
    }

    varianceShadowMapRenderer_.setLight(light);
    varianceShadowMapRenderer_.begin();
    for(auto &mesh : meshes_)
        varianceShadowMapRenderer_.render(mesh.vertexBuffer, Mat4::identity());
    varianceShadowMapRenderer_.end();

    gaussianBlur_.blur(varianceShadowMapRenderer_.getSRV());

    window_->useDefaultRTVAndDSV();
    window_->useDefaultViewport();
    window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
    window_->clearDefaultDepth(1.0f);

    VSM_.setCamera(camera_.getViewProj());
    VSM_.setLight(light);
    VSM_.setShadowMap(
        gaussianBlur_.getOutput(),
        varianceShadowMapRenderer_.getLightViewProj());

    VSM_.begin();
    for(auto &mesh : meshes_)
        VSM_.render(mesh.vertexBuffer, Mat4::identity());
    VSM_.end();
}

void ShadowMapApplication::updateCamera()
{
    camera_.setWOverH(window_->getClientWOverH());
    if(!mouse_->isVisible())
    {
        camera_.update({
            .front      = keyboard_->isPressed('W'),
            .left       = keyboard_->isPressed('A'),
            .right      = keyboard_->isPressed('D'),
            .back       = keyboard_->isPressed('S'),
            .up         = keyboard_->isPressed(KEY_SPACE),
            .down       = keyboard_->isPressed(KEY_LSHIFT),
            .cursorRelX = static_cast<float>(mouse_->getRelativePositionX()),
            .cursorRelY = static_cast<float>(mouse_->getRelativePositionY())
        });
    }
    camera_.recalculateMatrics();
}

Light ShadowMapApplication::getLight()
{
    if(rotateLight_)
    {
        const double us = fps_.elasped_microseconds();
        lightAngle_ += 15 * static_cast<float>(us) / 1e6f;
    }

    while(lightAngle_ > 360)
        lightAngle_ -= 360;
    while(lightAngle_ < 0)
        lightAngle_ += 360;

    const float lightRadian = agz::math::deg2rad(lightAngle_);

    const Float3 light_pos = {
        10 * std::cos(lightRadian), 10, 10 * std::sin(lightRadian) };
    const Float3 light_dst = { 0, 1, 0 };

    return Light{
        .position     = light_pos,
        .fadeCosBegin = 1,
        .direction    = (light_dst - light_pos).normalize(),
        .fadeCosEnd   = 0.97f,
        .incidence    = Float3(80),
        .ambient      = 0.01f
    };
}

ShadowMapApplication::Mesh ShadowMapApplication::loadMesh(
    const std::string &filename) const
{
    const auto triangles = agz::mesh::load_from_file(filename);

    std::vector<MeshVertex> vertices;
    vertices.reserve(triangles.size() * 3);
    for(auto &tri : triangles)
    {
        for(auto &v : tri.vertices)
            vertices.push_back({ v.position, v.normal });
    }

    Mesh mesh;
    mesh.vertexBuffer.initialize(vertices.size(), vertices.data());

    return mesh;
}

int main()
{
    ShadowMapApplication(
        WindowDesc
        {
            .clientSize  = { 640, 480 },
            .title       = L"00.ShadowMap",
            .sampleCount = 4,
        })
    .run();
}
