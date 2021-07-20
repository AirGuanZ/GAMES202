#include <agz-utils/mesh.h>

#include "lut.h"
#include "renderer.h"

class KullaContyApplication : public Demo
{
public:

    using Demo::Demo;

protected:

    void initialize() override
    {
        renderer_.initialize();

        const Mat4 rot = Trans4::rotate_z(agz::math::PI_f / 2);

        constexpr int count = 7;
        for(int i = 0; i < count; ++i)
        {
            const float t = i / (count - 1.0f);
            const float z = agz::math::lerp(-10.0f, 10.0f, t);

            addMesh(
                "./asset/torus.obj",
                rot * Trans4::translate(0, 0, z),
                agz::math::lerp(0.1f, 1.0f, t));
        }

        lut_ = LUTGenerator().generate({ 32, 32 }, 64);

        camera_.setPosition({ -15, 0, 0 });
        camera_.setDirection(0, 0);
        camera_.setPerspective(60.0f, 0.1f, 100.0f);

        mouse_->setCursorLock(
            true, window_->getClientWidth() / 2, window_->getClientHeight() / 2);
        mouse_->showCursor(false);
        window_->doEvents();
    }

    void frame() override
    {
        if(keyboard_->isDown(KEY_ESCAPE))
            window_->setCloseFlag(true);

        if(keyboard_->isDown(KEY_LCTRL))
        {
            mouse_->showCursor(!mouse_->isVisible());
            mouse_->setCursorLock(
                !mouse_->isLocked(), mouse_->getLockX(), mouse_->getLockY());
        }

        if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::ColorEdit3("R0", &R0_.x);
            ImGui::ColorEdit3("Edge Tint", &edgeTint_.x);
            ImGui::Checkbox("Enable KC Model", &enableKC_);
        }
        ImGui::End();
        
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

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
        window_->clearDefaultDepth(1);

        renderer_.setKCModel(enableKC_);
        renderer_.setLight(Float3(4, -1.5f, 0.5f).normalize(), Float3(1));
        renderer_.setCamera(camera_);
        renderer_.begin();
        for(auto &m : meshes_)
        {
            renderer_.render(
                m.vertexBuffer, m.world, R0_, edgeTint_, m.roughness);
        }
        renderer_.end();
    }

private:

    struct Mesh
    {
        VertexBuffer<Renderer::Vertex> vertexBuffer;
        Mat4   world;
        float  roughness;
    };

    void addMesh(
        const std::string &meshFilename,
        const Mat4        &world,
        float              roughness)
    {
        const auto tris = agz::mesh::load_from_file(meshFilename);

        std::vector<Renderer::Vertex> vertexData;
        for(auto &t : tris)
        {
            for(auto &v : t.vertices)
            {
                vertexData.push_back({
                    v.position,
                    v.normal
                });
            }
        }

        Mesh mesh;
        mesh.vertexBuffer.initialize(vertexData.size(), vertexData.data());
        mesh.world     = world;
        mesh.roughness = roughness;

        meshes_.push_back(std::move(mesh));
    }

    Float3 R0_       = { 0.2f, 0.7f, 0.7f };
    Float3 edgeTint_ = { 0.3f, 0.8f, 0.8f };

    std::vector<Mesh> meshes_;

    bool enableKC_ = true;
    Renderer renderer_;

    LUTGenerator::LUT lut_;

    Camera camera_;
};

int main()
{
    KullaContyApplication(WindowDesc{
        .clientSize  = { 640, 480 },
        .title       = L"Kulla-Conty Model",
        .sampleCount = 4
    }).run();
}
