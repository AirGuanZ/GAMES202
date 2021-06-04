#include <agz-utils/mesh.h>

#include <common/application.h>
#include <common/camera.h>

#include "./gbuffer.h"
#include "./indirect.h"
#include "./rsm.h"
#include "./shade.h"

class ReflectiveShadowMapApplication : public Application
{
public:

    using Application::Application;

private:

    static constexpr int RSM_W = 256;
    static constexpr int RSM_H = 256;

    void initialize() override
    {
        gbuffer_.initialize(window_->getClientSize());

        rsm_.initialize({ RSM_W, RSM_H });

        shade_.initialize();
        shade_.enableIndirect(enabledIndirect_);
        shade_.setSampleParams(rsmSamples_, rsmRadius_);

        lowres_gbuffer_.initialize(lowres_);
        lowres_indirect_.initialize(lowres_);
        lowres_indirect_.setSampleParams(rsmSamples_, rsmRadius_);

        window_->attach([this](const WindowPostResizeEvent &e)
        {
            gbuffer_.resize({ e.width, e.height });
        });

        meshes_.push_back({
            loadMesh("./asset/cylinder.obj", { 0.8f, 0.8f, 0.8f }),
            Mat4::identity()
        });

        meshes_.push_back({
            loadMesh("./asset/corner.obj", { 0.6f, 0, 0 }),
            Mat4::right_transform::translate(0, -0.4f, 0)
        });

        meshes_.push_back({
            loadMesh("./asset/torus.obj", { 0.5f, 0.9f, 0.9f }),
            Mat4::right_transform::translate(0, -0.4f, 0) *
            Mat4::right_transform::scale(1.5f, 1.5f, 1.5f)
        });
        
        camera_.setPosition({ 0, 2, 3 });
        camera_.setDirection(-3.14159f * 0.5f, 0);
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
        
        if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if(ImGui::Checkbox("Enable Indirect", &enabledIndirect_))
                shade_.enableIndirect(enabledIndirect_);

            if(ImGui::InputInt("Sample Count", &rsmSamples_))
            {
                rsmSamples_ = (std::max)(rsmSamples_, 1);
                lowres_indirect_.setSampleParams(rsmSamples_, rsmRadius_);
                shade_.setSampleParams(rsmSamples_, rsmRadius_);
            }

            if(ImGui::SliderFloat("Sample Radius", &rsmRadius_, 0, 1))
            {
                lowres_indirect_.setSampleParams(rsmSamples_, rsmRadius_);
                shade_.setSampleParams(rsmSamples_, rsmRadius_);
            }

            if(ImGui::InputInt("Indirect Resolution", &lowres_.x))
            {
                lowres_.x = (std::max)(lowres_.x, 1);
                lowres_.y = lowres_.x;

                lowres_gbuffer_.resize(lowres_);
                lowres_indirect_.resize(lowres_);
            }
        }
        ImGui::End();

        DirectionalLight light = {};
        light.direction = Float3(-1, -2, -1).normalize();
        light.radiance  = Float3(1, 1, 1);

        const Float3 lightLookAt = { -1, 0, -1 };
        const Mat4 lightViewProj =
            Mat4::right_transform::look_at(
                lightLookAt - 5 * light.direction, lightLookAt, { 0, 1, 0 }) *
            Mat4::right_transform::orthographic(-6, 6, 6, -6, 1, 30);

        // rsm

        rsm_.setLight(light, lightViewProj);

        rsm_.begin();
        for(auto &m : meshes_)
            rsm_.render(m.vertexBuffer, m.world);
        rsm_.end();

        // lowres gbuffer

        lowres_gbuffer_.setCamera(camera_.getViewProj());

        lowres_gbuffer_.begin();
        for(auto &m : meshes_)
            lowres_gbuffer_.render(m.vertexBuffer, m.world);
        lowres_gbuffer_.end();

        // lowres indirect

        lowres_indirect_.setLight(lightViewProj, 144);
        lowres_indirect_.setGBuffer(
            lowres_gbuffer_.getGBuffer(0), lowres_gbuffer_.getGBuffer(1));
        lowres_indirect_.setRSM(rsm_.getRSM(0), rsm_.getRSM(1), rsm_.getRSM(2));

        lowres_indirect_.render();

        // gbuffer

        gbuffer_.setCamera(camera_.getViewProj());

        gbuffer_.begin();
        for(auto &m : meshes_)
            gbuffer_.render(m.vertexBuffer, m.world);
        gbuffer_.end();

        // render

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultDepth(1);
        window_->clearDefaultRenderTarget({ 0, 1, 1, 0 });

        shade_.setLight(light, lightViewProj, 144);
        shade_.setGBuffer(gbuffer_.getGBuffer(0), gbuffer_.getGBuffer(1));
        shade_.setRSM(rsm_.getRSM(0), rsm_.getRSM(1), rsm_.getRSM(2));
        shade_.setLowResIndirect(
            lowres_gbuffer_.getGBuffer(0),
            lowres_gbuffer_.getGBuffer(1),
            lowres_indirect_.getOutput());

        shade_.render();

        window_->renderImGui();
        window_->swapBuffers();

        return !window_->getCloseFlag();
    }

    VertexBuffer<Vertex> loadMesh(
        const std::string &filename, const Float3 &albedo)
    {
        const auto tris = agz::mesh::load_from_file(filename);

        std::vector<Vertex> vertices;
        vertices.reserve(tris.size() * 3);
        for(auto &tri : tris)
        {
            for(auto &v : tri.vertices)
                vertices.push_back({ v.position, v.normal, albedo });
        }

        VertexBuffer<Vertex> result;
        result.initialize(vertices.size(), vertices.data());
        return result;
    }

    struct Mesh
    {
        VertexBuffer<Vertex> vertexBuffer;
        Mat4                 world;
    };

    Int2 lowres_ = { 100, 100 };

    int   rsmSamples_ = 400;
    float rsmRadius_  = 0.2f;

    bool enabledIndirect_ = true;

    Camera camera_;

    GBuffer          gbuffer_;
    GBuffer          lowres_gbuffer_;
    IndirectRenderer lowres_indirect_;
    RSMGenerator     rsm_;
    Renderer         shade_;

    std::vector<Mesh> meshes_;
};

int main()
{
    ReflectiveShadowMapApplication(
        WindowDesc
        {
            .clientSize  = { 640, 480 },
            .title       = L"EX.00.ReflectiveShadowMap",
            .sampleCount = 1,
        })
    .run();
}
