#include <agz-utils/mesh.h>

#include <common/application.h>
#include <common/camera.h>

#include "./accumulate.h"
#include "./direct.h"
#include "./final.h"
#include "./gbuffer.h"
#include "./indirect.h"
#include "./mipmap.h"
#include "./shadow.h"

class ScreenSpaceRayTracingApplication : public Application
{
public:

    using Application::Application;

private:

    void initialize() override
    {
        accu_    .initialize(window_->getClientSize());
        direct_  .initialize(window_->getClientSize());
        final_   .initialize();
        gbufferA_.initialize(window_->getClientSize());
        gbufferB_.initialize(window_->getClientSize());
        indirect_.initialize(window_->getClientSize());
        mipmap_  .initialize(window_->getClientSize());
        shadow_  .initialize({ 1024, 1024});

        gbuffer_ = &gbufferA_;

        indirect_.setSampleCount(sampleCount_);
        indirect_.setDepthThreshold(depthThreshold_);
        indirect_.setTracer(
            maxTraceSteps_, initialMipLevel_, initialTraceStep_);

        accu_.setFactor(accuAlpha_);

        window_->attach([this](const WindowPostResizeEvent &e)
        {
            accu_    .resize({ e.width, e.height });
            direct_  .resize({ e.width, e.height });
            gbufferA_.resize({ e.width, e.height });
            gbufferB_.resize({ e.width, e.height });
            indirect_.resize({ e.width, e.height });
            mipmap_  .resize({ e.width, e.height });
        });

        meshes_.push_back(loadMesh(
            "./asset/02/cave.obj",
            "./asset/02/albedo.jpg",
            "./asset/02/normal.jpg",
            Mat4::identity()));

        camera_.setPosition({ 3.54f, 0.58f, 1.55f });
        camera_.setDirection(3.19f, 0.17f);
        camera_.setPerspective(60.0f, 0.1f, 100.0f);

        mouse_->setCursorLock(
            true, window_->getClientWidth() / 2, window_->getClientHeight() / 2);
        mouse_->showCursor(false);
        window_->doEvents();
    }

    bool frame() override
    {
        // window events

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

        // camera

        auto lastViewProj = camera_.getViewProj();

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
        
        // gui

        ImGui::SetNextWindowBgAlpha(0.5f);
        ImGui::SetNextWindowPos({ 0, 0 });
        if(ImGui::Begin(
            "Help", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration))
        {
            ImGui::Text("Use LCtrl to show/hide cursor");
        }
        ImGui::End();

        ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
        if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SliderFloat("Light Hori Angle", &lightHoriAngle_, 0, 360);
            ImGui::SliderFloat("Light Vert Angle", &lightVertAngle_, 0, 90);
            ImGui::SliderFloat("Light Radiance", &lightRadiance_, 0, 30);

            if(ImGui::InputInt("Sample Count", &sampleCount_))
            {
                sampleCount_ = (std::max)(1, sampleCount_);
                indirect_.setSampleCount(sampleCount_);
            }

            if(ImGui::SliderFloat("Depth Threshold", &depthThreshold_, 0.1f, 10.0f))
                indirect_.setDepthThreshold(depthThreshold_);

            if(ImGui::InputInt("Max Trace Steps", &maxTraceSteps_))
            {
                maxTraceSteps_ = (std::max)(1, maxTraceSteps_);
                indirect_.setTracer(
                    maxTraceSteps_, initialMipLevel_, initialTraceStep_);
            }

            if(ImGui::InputInt("Initial Mipmap Level", &initialMipLevel_))
            {
                initialMipLevel_ = (std::max)(initialMipLevel_, 0);
                indirect_.setTracer(
                    maxTraceSteps_, initialMipLevel_, initialTraceStep_);
            }

            if(ImGui::InputFloat("Initial Trace Step", &initialTraceStep_))
            {
                initialTraceStep_ = (std::max)(initialTraceStep_, 1.0f);
                indirect_.setTracer(
                    maxTraceSteps_, initialMipLevel_, initialTraceStep_);
            }

            if(ImGui::SliderFloat("Alpha", &accuAlpha_, 0, 1))
                accu_.setFactor(accuAlpha_);

            if(ImGui::Checkbox("Enable Direct", &enableDirect_) && !enableDirect_)
                enableIndirect_ = true;
            ImGui::SameLine();
            if(ImGui::Checkbox("Enable Indirect", &enableIndirect_) && !enableIndirect_)
                enableDirect_ = true;

            if(!enableDirect_ && enableIndirect_)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Enable Indirect Color", &enableIndirectColor_);
            }

            assert(enableDirect_ || enableIndirect_);
            if(enableDirect_ && enableIndirect_)
                enableIndirectColor_ = true;

            ImGui::SliderFloat("Exposure", &exposure_, 0, 2);
        }
        ImGui::End();

        // light

        DirectionalLight light = {};
        light.direction = {
             std::sin(agz::math::deg2rad(lightVertAngle_)) *
             std::cos(agz::math::deg2rad(lightHoriAngle_)),
            -std::cos(agz::math::deg2rad(lightVertAngle_)),
             std::sin(agz::math::deg2rad(lightVertAngle_)) *
             std::sin(agz::math::deg2rad(lightHoriAngle_))
        };
        light.radiance  = Float3(lightRadiance_);

        const Float3 lightLookAt = { 0, 0, 0 };
        const Mat4 lightViewProj =
            Mat4::right_transform::look_at(
                lightLookAt - 15 * light.direction, lightLookAt, { 1, 0, 0 }) *
            Mat4::right_transform::orthographic(-5, 5, 5, -5, 3, 40);

        // shadow

        shadow_.setLight(lightViewProj);
        shadow_.begin();
        for(auto &m : meshes_)
            shadow_.render(m.vertexBuffer, m.world);
        shadow_.end();

        // gbuffer

        auto lastGBuffer_ = gbuffer_;
        gbuffer_ = gbuffer_ == &gbufferA_ ? &gbufferB_ : &gbufferA_;

        gbuffer_->setCamera(camera_.getView(), camera_.getViewProj());
        gbuffer_->begin();
        for(auto &m : meshes_)
            gbuffer_->render(m);
        gbuffer_->end();

        // mipmap

        mipmap_.generate(gbuffer_->getGBuffer(1));

        // direct light

        direct_.setLight(light, lightViewProj);
        direct_.render(
            shadow_.getShadowMap(),
            gbuffer_->getGBuffer(0),
            gbuffer_->getGBuffer(1));

        // indirect light

        if(enableIndirect_)
        {
            // hack: regenerate samples in every frame so that we can accumulate
            // different estimates in history frames
            // ideally, progressive low-disparency sequence should be used
            indirect_.setSampleCount(sampleCount_);

            indirect_.setCamera(
                camera_.getView(), camera_.getProj(), camera_.getNearZ());
            indirect_.render(
                gbuffer_->getGBuffer(0),
                gbuffer_->getGBuffer(1),
                mipmap_.getOutput(),
                direct_.getOutput());
        }

        // accumulate

        if(enableIndirect_)
        {
            accu_.setCamera(lastViewProj);
            accu_.accumulate(
                lastGBuffer_->getGBuffer(0),
                gbuffer_->getGBuffer(0),
                indirect_.getOutput());
        }

        // render

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultDepth(1);
        window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
        
        final_.render(
            gbuffer_->getGBuffer(1),
            enableDirect_   ? direct_.getOutput() : nullptr,
            enableIndirect_ ? accu_  .getOutput() : nullptr,
            enableIndirectColor_,
            exposure_);

        // finalize
        
        window_->renderImGui();
        window_->swapBuffers();

        return !window_->getCloseFlag();
    }

    static Float3 computeTangent(
        const Float3 &BA,
        const Float3 &CA,
        const Float2 &uvBA,
        const Float2 &uvCA,
        const Float3 &nor)
    {
        const float m00 = uvBA.x, m01 = uvBA.y;
        const float m10 = uvCA.x, m11 = uvCA.y;
        const float det = m00 * m11 - m01 * m10;
        if(std::abs(det) < 0.0001f)
            return agz::math::tcoord3<float>::from_z(nor).x;
        const float inv_det = 1 / det;
        return (m11 * inv_det * BA - m01 * inv_det * CA).normalize();
    }

    static Mesh loadMesh(
        const std::string &objFilename,
        const std::string &albedoFilename,
        const std::string &normalFilename,
        const Mat4        &world)
    {
        const auto tris = agz::mesh::load_from_file(objFilename);

        std::vector<Vertex> vertexData;
        for(auto &tri : tris)
        {
            const auto &a = tri.vertices[0];
            const auto &b = tri.vertices[1];
            const auto &c = tri.vertices[2];
            const Float3 BA = b.position - a.position;
            const Float3 CA = c.position - a.position;
            const Float2 uvBA = b.tex_coord - a.tex_coord;
            const Float2 uvCA = c.tex_coord - a.tex_coord;

            for(auto &v : tri.vertices)
            {
                vertexData.push_back({
                    v.position, v.normal,
                    computeTangent(BA, CA, uvBA, uvCA, v.normal),
                    v.tex_coord
                });
            }
        }

        Mesh mesh;
        mesh.vertexBuffer.initialize(vertexData.size(), vertexData.data());
        mesh.albedo = Texture2DLoader::loadFromFile(
            DXGI_FORMAT_R8G8B8A8_UNORM, albedoFilename, 0);
        mesh.normal = Texture2DLoader::loadFromFile(
            DXGI_FORMAT_R8G8B8A8_UNORM, normalFilename, 0);
        mesh.world = world;

        return mesh;
    }

    IndirectAccumulator accu_;
    DirectRenderer      direct_;
    FinalRenderer       final_;
    GBufferGenerator    gbufferA_;
    GBufferGenerator    gbufferB_;
    IndirectRenderer    indirect_;
    MipmapsGenerator    mipmap_;
    ShadowMapRenderer   shadow_;

    GBufferGenerator *gbuffer_ = nullptr;

    float lightRadiance_ = 15;

    float lightHoriAngle_ = 26;
    float lightVertAngle_ = 16;

    bool  enableDirect_        = true;
    bool  enableIndirect_      = true;
    bool  enableIndirectColor_ = true;
    float exposure_            = 1;

    int   sampleCount_      = 4;
    int   maxTraceSteps_    = 32;
    int   initialMipLevel_  = 4;
    float initialTraceStep_ = 20;
    float depthThreshold_   = 1;

    float accuAlpha_ = 0.05f;

    Camera camera_;

    std::vector<Mesh> meshes_;
};

int main()
{
    ScreenSpaceRayTracingApplication(
        WindowDesc
        {
            .clientSize  = { 640, 480 },
            .title       = L"02.ScreenSpaceRayTracing",
            .sampleCount = 1,
        })
    .run();
}
