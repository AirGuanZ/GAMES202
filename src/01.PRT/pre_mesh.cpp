#include <random>

#include <agz-utils/console.h>
#include <agz-utils/thread.h>

#include <common/bvh.h>

#include "pre_mesh.h"

namespace
{

    constexpr float EPS = 2e-4f;

    class Sampler
    {
        std::minstd_rand                      rng_;
        std::uniform_real_distribution<float> dis_;

    public:

        explicit Sampler(int seed)
            : rng_(seed), dis_(0, 1)
        {
            
        }

        float sample()
        {
            return dis_(rng_);
        }

        Float2 sample2()
        {
            const float s1 = sample();
            const float s2 = sample();
            return { s1, s2 };
        }
    };

    void computeVertexSHNoShadow(
        float         coef,
        const Ray    &ray,
        int           SHCount,
        float        *output)
    {
        if(!agz::math::is_finite(coef))
            return;

        auto SHFuncs = agz::math::spherical_harmonics::linear_table<float>();
        for(int i = 0; i < SHCount; ++i)
            output[i] += coef * SHFuncs[i](ray.d);
    }

    void computeVertexSHShadow(
        float      coef,
        const Ray &ray,
        const BVH &bvh,
        int        SHCount,
        float     *output)
    {
        if(!agz::math::is_finite(coef))
            return;

        if(bvh.hasIntersection(ray))
            return;

        auto SHFuncs = agz::math::spherical_harmonics::linear_table<float>();
        for(int i = 0; i < SHCount; ++i)
            output[i] += coef * SHFuncs[i](ray.d);
    }

    void computeVertexSHInterRefl(
        const SHVertex *vertices,
        float           coef,
        float           brdf,
        Ray             ray,
        const BVH      &bvh,
        int             SHCount,
        int             maxDepth,
        Sampler        &sampler,
        float          *output)
    {
        auto SHFuncs = agz::math::spherical_harmonics::linear_table<float>();

        for(int depth = 1; depth <= maxDepth; ++depth)
        {
            if(!agz::math::is_finite(coef))
                return;

            BVH::Intersection inct;
            if(!bvh.findIntersection(ray, &inct))
            {
                for(int i = 0; i < SHCount; ++i)
                    output[i] += coef * SHFuncs[i](ray.d);
                return;
            }

            const SHVertex *tri = &vertices[3 * inct.triangle];
            const Float3 nor =
                ((1 - inct.uv.sum()) * tri[0].normal +
                 inct.uv.x           * tri[1].normal +
                 inct.uv.y           * tri[2].normal).normalize();

            if(dot(nor, ray.d) >= 0)
                return;
            
            const Float2 sam = sampler.sample2();
            auto [local_dir, pdf_dir] =
                agz::math::distribution::zweighted_on_hemisphere(sam.x, sam.y);

            const Frame localFrame = Frame::from_z(nor);
            ray.d = localFrame.local_to_global(local_dir).normalize();
            ray.o = inct.position + EPS * nor;

            coef *= brdf * abs(cos(ray.d, nor)) / pdf_dir;
        }
    }
    
} // namespace anonymous

std::vector<float> computeVertexSHCoefs(
    const SHVertex *vertices,
    int             vertexCount,
    float           vertexAlbedo,
    int             maxOrder,
    int             samplesPerVertex,
    LightingMode    mode)
{
    assert(vertices && vertexCount > 0);
    assert(vertexCount % 3 == 0);
    assert(samplesPerVertex > 0);

    const float brdf = vertexAlbedo / PI;
    const float invSamplesPerVertex = 1.0f / samplesPerVertex;

    BVH bvh;
    if(mode != LightingMode::NoShadow)
    {
        std::vector<Float3> positions(vertexCount);
        for(int vi = 0; vi < vertexCount; ++vi)
            positions[vi] = vertices[vi].position;
        bvh = BVH::create(positions.data(), vertexCount / 3);
    }

    const int SHCount = agz::math::sqr(maxOrder + 1);
    std::vector<float> result(SHCount * vertexCount, 0.0f);

    const int reportStepSize = (std::max)(vertexCount / 50, 1);
    int lastReportedVi = 0;
    agz::console::progress_bar_f_t pbar(80, '=');
    pbar.display();

    agz::thread::parallel_forrange(0, vertexCount, [&](int threadIdx, int vi)
    {
        Sampler sampler(vi);

        auto &vertex = vertices[vi];
        float *output = &result[SHCount * vi];

        const Float3 o = vertex.position + EPS * vertex.normal;
        const Frame localFrame = Frame::from_z(vertex.normal);
        
        for(int si = 0; si < samplesPerVertex; ++si)
        {
            const auto sam = sampler.sample2();
            const auto [localDir, pdfDir] =
                agz::math::distribution::zweighted_on_hemisphere(sam.x, sam.y);

            const Float3 d = localFrame.local_to_global(localDir).normalize();
            const Ray ray(o, d);
            
            const float initCoef = brdf * abs(cos(d, vertex.normal)) / pdfDir;

            if(mode == LightingMode::NoShadow)
                computeVertexSHNoShadow(initCoef, ray, SHCount, output);
            else if(mode == LightingMode::Shadow)
                computeVertexSHShadow(initCoef, ray, bvh, SHCount, output);
            else
            {
                computeVertexSHInterRefl(
                    vertices, initCoef, brdf, ray, bvh,
                    SHCount, 5, sampler, output);
            }
        }

        for(int i = 0; i < SHCount; ++i)
            output[i] *= invSamplesPerVertex;

        if(threadIdx == 0 && vi - lastReportedVi >= reportStepSize)
        {
            lastReportedVi = vi;
            pbar.set_percent(100.0f * vi / vertexCount);
            pbar.display();
        }
    }, -1);

    pbar.done();
    return result;
}
