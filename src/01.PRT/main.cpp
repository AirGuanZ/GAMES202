#include <agz-utils/image.h>
#include <agz-utils/mesh.h>
#include <agz-utils/string.h>
#include <agz-utils/time.h>

#include <common/camera.h>

#include "pre_env.h"
#include "pre_mesh.h"
#include "renderer.h"

class PRTApplication : public Demo
{
    using Demo::Demo;

protected:

    void initialize() override;

    void frame() override;

    void destroy() override;

private:

    static constexpr int MAX_SH_ORDER  = 4;
    static constexpr int FULL_SH_COUNT = (MAX_SH_ORDER + 1) * (MAX_SH_ORDER + 1);

    static constexpr int SAMPLES_PER_VERTEX = 1024;
    static constexpr int SAMPLES_FOR_LIGHT  = 1000000;

    static constexpr float VERTEX_ALBEDO = 0.8f;

    void updateCamera();

    void updateRendererSettings();

    std::string getCacheFilename(const std::string &filename) const;

    void loadMesh(const std::string &filename);

    void loadEnv(const std::string &filename);

    std::vector<float> loadCachedMeshCoefs(
        size_t vertexCount, const std::string &cacheFilename) const;

    std::vector<float> generateMeshCoefs(
        const SHVertex     *vertices,
        size_t              vertexCount,
        const std::string  &cacheFilename) const;

    std::vector<Float3> loadCachedLightCoefs(
        const std::string &cacheFilename) const;

    std::vector<Float3> generateLightCoefs(
        const agz::texture::texture2d_t<Float3> &env,
        const std::string                       &cacheFilename) const;

    const char *getLightingModeName() const;

    ImGui::FileBrowser meshFileBrowser_;
    ImGui::FileBrowser envFileBrowser_;

    std::string meshFilename_;
    std::string envFilename_;

    int maxSHOrder_ = 4;

    bool  autoRotate_  = true;
    float rotateAngle_ = 0;

    LightingMode lightingMode_ = LightingMode::NoShadow;

    Camera   camera_;
    Renderer renderer_;

    std::vector<Float3> vertices_;
    std::vector<float>  fullMeshSHCoefs_;
    std::vector<Float3> envSHCoefs_;

    agz::time::fps_counter_t fps_;
};

void PRTApplication::initialize()
{
    renderer_.initialize();
    renderer_.setSH(agz::math::sqr(maxSHOrder_ + 1));

    loadMesh("./asset/202.obj");
    loadEnv("./asset/01/small_hangar_01_4k.hdr");

    meshFilename_ = "./asset/202.obj";
    envFilename_ = "./asset/01/small_hangar_01_4k.hdr";

    camera_.setPosition({ 0, 2, 3 });
    camera_.setDirection(-3.14159f * 0.5f, 0);
    camera_.setPerspective(60.0f, 0.1f, 100.0f);

    mouse_->setCursorLock(
        true, window_->getClientWidth() / 2, window_->getClientHeight() / 2);
    mouse_->showCursor(false);
    window_->doEvents();

    fps_.restart();
}

void PRTApplication::frame()
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

    // camera

    updateCamera();

    // gui

    if(ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Press LCtrl to show/hide cursor");
        ImGui::Text("Use W/A/S/D/Space/LShift to move");
        ImGui::Text("FPS: %d", fps_.fps());

        if(ImGui::RadioButton("NoShadow", lightingMode_ == LightingMode::NoShadow))
        {
            lightingMode_ = LightingMode::NoShadow;
            if(!meshFilename_.empty())
                loadMesh(meshFilename_);
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("Shadow", lightingMode_ == LightingMode::Shadow))
        {
            lightingMode_ = LightingMode::Shadow;
            if(!meshFilename_.empty())
                loadMesh(meshFilename_);
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("InterRefl", lightingMode_ == LightingMode::InterRefl))
        {
            lightingMode_ = LightingMode::InterRefl;
            if(!meshFilename_.empty())
                loadMesh(meshFilename_);
        }

        if(ImGui::SliderInt("Max Order", &maxSHOrder_, 0, 4))
            updateRendererSettings();

        if(ImGui::Button("Reload Mesh"))
        {
            meshFileBrowser_.SetTitle("Select Mesh");
            meshFileBrowser_.Open();
        }

        ImGui::SameLine();

        if(ImGui::Button("Reload Light"))
        {
            envFileBrowser_.SetTitle("Select Env");
            envFileBrowser_.SetTypeFilters({ ".hdr", ".HDR" });
            envFileBrowser_.Open();
        }

        ImGui::Checkbox("Rotate Light", &autoRotate_);

        if(ImGui::SliderFloat("Light Angle", &rotateAngle_, 0, 360))
            updateRendererSettings();
    }
    ImGui::End();

    if(autoRotate_)
    {
        rotateAngle_ += 1;
        if(rotateAngle_ > 360)
            rotateAngle_ -= 360;
        updateRendererSettings();
    }

    meshFileBrowser_.Display();
    envFileBrowser_.Display();

    if(meshFileBrowser_.HasSelected())
    {
        meshFilename_ = meshFileBrowser_.GetSelected().string();
        meshFileBrowser_.ClearSelected();
        loadMesh(meshFilename_);
    }

    if(envFileBrowser_.HasSelected())
    {
        envFilename_ = envFileBrowser_.GetSelected().string();
        envFileBrowser_.ClearSelected();
        loadEnv(envFilename_);
    }

    window_->useDefaultRTVAndDSV();
    window_->useDefaultViewport();
    window_->clearDefaultRenderTarget({ 0, 0, 0, 0 });
    window_->clearDefaultDepth(1.0f);

    if(!fullMeshSHCoefs_.empty() && !envSHCoefs_.empty())
        renderer_.render(camera_.getViewProj());

    fps_.frame_end();
}

void PRTApplication::destroy()
{

}

void PRTApplication::updateCamera()
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

void PRTApplication::updateRendererSettings()
{
    assert(0 <= maxSHOrder_ && maxSHOrder_ <= MAX_SH_ORDER);
    
    const int SHCount = agz::math::sqr(maxSHOrder_ + 1);
    renderer_.setSH(SHCount);
    
    if(!fullMeshSHCoefs_.empty())
    {
        std::vector<float> meshSHCoefs(SHCount * vertices_.size());

        for(size_t vi = 0; vi < vertices_.size(); ++vi)
        {
            const size_t lhsBase = vi * SHCount;
            const size_t rhsBase = vi * FULL_SH_COUNT;

            for(int ci = 0; ci < SHCount; ++ci)
                meshSHCoefs[lhsBase + ci] = fullMeshSHCoefs_[rhsBase + ci];
        }

        renderer_.setVertices(
            vertices_.data(), meshSHCoefs.data(),
            static_cast<int>(vertices_.size()));
    }

    if(!envSHCoefs_.empty())
    {
        std::vector<Float3> rotatedCoefs = envSHCoefs_;

        rotateEnvSHCoefs(
            agz::math::mat3f_c::rotate_y(agz::math::deg2rad(rotateAngle_)),
            rotatedCoefs);

        renderer_.setLight(rotatedCoefs.data());
    }
}

std::string PRTApplication::getCacheFilename(const std::string &filename) const
{
    const auto nor_path =
        relative(std::filesystem::path(filename)).lexically_normal();
    std::string result = nor_path.string();
    agz::stdstr::replace_(result, "/", "__");
    agz::stdstr::replace_(result, "\\", "__");
    agz::stdstr::replace_(result, ":", "__");
    return "./asset/01/.cache/" + result;
}

void PRTApplication::loadMesh(const std::string &filename)
{
    auto triangles = agz::mesh::load_from_file(filename);

    vertices_.clear();
    vertices_.reserve(triangles.size() * 3);
    for(auto &t : triangles)
    {
        vertices_.push_back(t.vertices[0].position);
        vertices_.push_back(t.vertices[1].position);
        vertices_.push_back(t.vertices[2].position);
    }

    const auto cacheFilenameSuffix = std::string(".") + getLightingModeName();
    const auto cacheFilename = getCacheFilename(filename) + cacheFilenameSuffix;
    auto meshCoefs = loadCachedMeshCoefs(vertices_.size(), cacheFilename);

    if(meshCoefs.empty())
    {
        std::vector<SHVertex> SHVertices;
        SHVertices.reserve(triangles.size() * 3);
        for(auto &t : triangles)
        {
            for(int i = 0; i < 3; ++i)
            {
                SHVertices.push_back({
                    t.vertices[i].position,
                    t.vertices[i].normal
                });
            }
        }

        meshCoefs = generateMeshCoefs(
            SHVertices.data(), SHVertices.size(), cacheFilename);
    }

    fullMeshSHCoefs_ = std::move(meshCoefs);
    updateRendererSettings();
}

void PRTApplication::loadEnv(const std::string &filename)
{
    const std::string cacheFilename = getCacheFilename(filename) + ".env";
    auto result = loadCachedLightCoefs(cacheFilename);

    if(result.empty())
    {
        auto data = agz::img::load_rgb_from_hdr_file(filename).map(
            [](const agz::math::color3f &c)
        {
            return Float3(c.r, c.g, c.b);
        });
        auto env = agz::texture::texture2d_t<Float3>(std::move(data));
        result = generateLightCoefs(env, cacheFilename);
    }

    envSHCoefs_ = result;
    updateRendererSettings();
}

std::vector<float> PRTApplication::loadCachedMeshCoefs(
    size_t vertexCount, const std::string &cacheFilename) const
{
    if(!std::filesystem::exists(cacheFilename))
        return {};

    auto bytes = agz::file::read_raw_file(cacheFilename);
    const size_t size = sizeof(float) * FULL_SH_COUNT * vertexCount;
    if(bytes.size() < size)
        return {};

    std::vector<float> result(vertexCount * FULL_SH_COUNT);
    std::memcpy(result.data(), bytes.data(), size);

    return result;
}

std::vector<float> PRTApplication::generateMeshCoefs(
    const SHVertex    *vertices,
    size_t             vertexCount,
    const std::string &cacheFilename) const
{
    auto result = computeVertexSHCoefs(
        vertices, static_cast<int>(vertexCount),
        VERTEX_ALBEDO, MAX_SH_ORDER, SAMPLES_PER_VERTEX, lightingMode_);

    agz::file::create_directory_for_file(cacheFilename);
    agz::file::write_raw_file(
        cacheFilename, result.data(), result.size() * sizeof(float));

    return result;
}

std::vector<Float3> PRTApplication::loadCachedLightCoefs(
    const std::string &cacheFilename) const
{
    if(!std::filesystem::exists(cacheFilename))
        return {};

    auto bytes = agz::file::read_raw_file(cacheFilename);
    const size_t size = sizeof(Float3) * FULL_SH_COUNT;
    if(bytes.size() < size)
        return {};

    std::vector<Float3> result(FULL_SH_COUNT);
    std::memcpy(result.data(), bytes.data(), size);

    return result;
}

std::vector<Float3> PRTApplication::generateLightCoefs(
    const agz::texture::texture2d_t<Float3> &env,
    const std::string                       &cacheFilename) const
{
    auto result = computeEnvSHCoefs(env, MAX_SH_ORDER, SAMPLES_FOR_LIGHT);

    agz::file::create_directory_for_file(cacheFilename);
    agz::file::write_raw_file(
        cacheFilename, result.data(), result.size() * sizeof(Float3));

    return result;
}

const char *PRTApplication::getLightingModeName() const
{
    switch(lightingMode_)
    {
    case LightingMode::NoShadow:
        return "noshadow";
    case LightingMode::Shadow:
        return "shadow";
    case LightingMode::InterRefl:
        return "interrefl";
    }
    agz::misc::unreachable();
}

int main()
{
    PRTApplication(
        WindowDesc{
            .clientSize  = { 640, 480 },
            .title       = L"01.PRT",
            .sampleCount = 4
        })
    .run();
}
