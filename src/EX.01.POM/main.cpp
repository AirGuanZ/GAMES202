#include <agz-utils/mesh.h>

#include <common/camera.h>

#include "./normal.h"
#include "./pom.h"
#include "./relief.h"

class ParallaxOcclusionMapping : public Demo
{
public:

    using Demo::Demo;

private:

    enum class Mode
    {
        Normal,
        Parallax,
        Relief
    };

    void initialize() override
    {
        normal_.initialize();
        normal_.setDiffStep(normalDiffStep_);

        pom_.initialize();
        pom_.setDiffStep(normalDiffStep_);
        pom_.setTracer(linearStep_, maxLinearStepCount_);

        relief_.initialize();
        relief_.setDiffStep(normalDiffStep_);
        relief_.setTracer(
            linearStep_, maxLinearStepCount_, binaryStepCount_);

        mesh_ = loadMesh(
            "./asset/ground.obj",
            "./asset/EX.01/height.png",
            "./asset/EX.01/albedo.png",
            Mat4::right_transform::scale(Float3(0.2f)), 2.5f);
        
        camera_.setPosition({ 0, 2, 3 });
        camera_.setDirection(-3.14159f * 0.5f, 0);
        camera_.setPerspective(60.0f, 0.1f, 100.0f);

        mouse_->setCursorLock(
            true,
            window_->getClientWidth() / 2,
            window_->getClientHeight() / 2);
        mouse_->showCursor(false);

        window_->doEvents();
    }

    void frame() override
    {
        // window events

        if(window_->getKeyboard()->isDown(KEY_ESCAPE))
            window_->setCloseFlag(true);

        if(keyboard_->isDown(KEY_LCTRL))
        {
            mouse_->showCursor(!mouse_->isVisible());
            mouse_->setCursorLock(
                !mouse_->isLocked(), mouse_->getLockX(), mouse_->getLockY());
        }

        // gui

        if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            const char *modeNames[] = {
                "Normal",
                "Parallax",
                "Relief"
            };
            if(ImGui::BeginCombo("Mode", modeNames[static_cast<int>(mode_)]))
            {
                for(int i = 0; i < 3; ++i)
                {
                    const bool selected = static_cast<int>(mode_) == i;
                    if(ImGui::Selectable(modeNames[i], selected))
                        mode_ = static_cast<Mode>(i);
                }
                ImGui::EndCombo();
            }

            ImGui::SliderFloat("Height Scale", &mesh_.heightScale, 0, 5);

            if(ImGui::SliderInt("UV Offset", &normalDiffStep_, 1, 5))
            {
                normal_.setDiffStep(normalDiffStep_);
                pom_.setDiffStep(normalDiffStep_);
                relief_.setDiffStep(normalDiffStep_);
            }
            if(mode_ == Mode::Parallax)
            {
                if(ImGui::InputInt(
                    "Max Linear Step Count", &maxLinearStepCount_) |
                   ImGui::InputFloat("Linear Step", &linearStep_))
                {
                    maxLinearStepCount_ =
                        (std::max)(maxLinearStepCount_, 1);
                    linearStep_ = (std::max)(0.0f, linearStep_);

                    pom_.setTracer(linearStep_, maxLinearStepCount_);
                }
            }
            else if(mode_ == Mode::Relief)
            {
                if(ImGui::InputInt(
                    "Max Linear Step Count", &maxLinearStepCount_) |
                   ImGui::InputFloat("Linear Step", &linearStep_)  |
                   ImGui::InputInt("Binary Step Count", &binaryStepCount_))
                {
                    maxLinearStepCount_ =
                        (std::max)(maxLinearStepCount_, 1);
                    linearStep_ = (std::max)(0.0f, linearStep_);
                    binaryStepCount_ = (std::max)(1, binaryStepCount_);

                    relief_.setTracer(
                        linearStep_, maxLinearStepCount_, binaryStepCount_);
                }
            }
        }
        ImGui::End();

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

        // light

        DirectionalLight light = {};
        light.radiance  = Float3(5);
        light.direction = Float3(1, -2, 1).normalize();
        light.ambient   = Float3(0.05f);

        // render

        window_->useDefaultRTVAndDSV();
        window_->useDefaultViewport();
        window_->clearDefaultDepth(1);
        window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });

        if(mode_ == Mode::Normal)
        {
            normal_.setLight(light);
            normal_.setCamera(camera_.getViewProj());
            normal_.begin();
            normal_.render(mesh_);
            normal_.end();
        }
        else if(mode_ == Mode::Parallax)
        {
            pom_.setLight(light);
            pom_.setCamera(camera_.getPosition(), camera_.getViewProj());
            pom_.begin();
            pom_.render(mesh_);
            pom_.end();
        }
        else
        {
            relief_.setLight(light);
            relief_.setCamera(camera_.getPosition(), camera_.getViewProj());
            relief_.begin();
            relief_.render(mesh_);
            relief_.end();
        }
    }

    static Mesh loadMesh(
        const std::string &objFilename,
        const std::string &heightFilename,
        const std::string &albedoFilename,
        const Mat4        &world,
        float              heightScale)
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
            
            Float3 faceNormal = cross(BA, CA).normalize();
            if(dot(a.normal + b.normal + c.normal, faceNormal) < 0)
                faceNormal = -faceNormal;

            const Float3 faceTangent =
                computeTangent(BA, CA, uvBA, uvCA, faceNormal);
            const Float3 faceBinormal =
                cross(faceTangent, faceNormal).normalize();

            const float dTangent =
                abs(uvBA.x) < 0.001f ? dot(CA, faceTangent) / uvCA.x
                                     : dot(BA, faceTangent) / uvBA.x;
            const float dBinormal =
                abs(uvBA.y) < 0.001f ? dot(CA, faceBinormal) / uvCA.y
                                     : dot(BA, faceBinormal) / uvBA.y;

            for(auto &v : tri.vertices)
            {
                vertexData.push_back({
                    v.position, faceNormal, faceTangent, v.tex_coord,
                    dTangent, dBinormal
                });
            }
        }

        Mesh mesh;
        mesh.vertexBuffer.initialize(vertexData.size(), vertexData.data());
        mesh.albedo = Texture2DLoader::loadFromFile(
            DXGI_FORMAT_R8G8B8A8_UNORM, albedoFilename, 1);
        mesh.height = Texture2DLoader::loadFromFile(
            DXGI_FORMAT_R8_UNORM, heightFilename, 1);
        mesh.world = world;
        mesh.heightScale = heightScale;

        return mesh;
    }

    Mode mode_ = Mode::Normal;

    int   normalDiffStep_ = 3;
    float linearStep_         = 0.005f;
    int   maxLinearStepCount_ = 256;
    int   binaryStepCount_    = 5;

    NormalMappingRenderer            normal_;
    ParallaxOcclusionMappingRenderer pom_;
    ReliefMappingRenderer            relief_;

    Camera camera_;

    Mesh mesh_;
};

int main()
{
    ParallaxOcclusionMapping(
        WindowDesc
        {
            .clientSize  = { 640, 480 },
            .title       = L"EX.01.ParallaxOcclusionMapping",
            .sampleCount = 1,
        })
    .run();
}
