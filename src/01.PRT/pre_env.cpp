#include <random>

#include "pre_env.h"

namespace
{

    Float3 sampleEnv(
        const agz::texture::texture2d_t<Float3> &env, const Float3 &d)
    {
        const float theta = std::asin(d.y);
        const float phi =
            (d.x == 0.0f && d.z == 0.0f) ? 0.0f : std::atan2(d.z, d.x);

        const float u = agz::math::normalize_radian_0_2pi(phi) / (2 * PI);
        const float v = -theta / PI + 0.5f;

        return agz::texture::linear_sample2d(
            Float2(u, v).saturate(),
            [&](int x, int y) { return env(y, x); },
            env.width(), env.height());
    }

    void rotateEnvSHCoefs(
        const agz::math::mat3f_c &rot, int order, float *coefs)
    {
        using FT = decltype(
            &agz::math::spherical_harmonics::rotate_sh_coefs<0, float>);

        static const FT rot_funcs[] =
        {
            &agz::math::spherical_harmonics::rotate_sh_coefs<0, float>,
            &agz::math::spherical_harmonics::rotate_sh_coefs<1, float>,
            &agz::math::spherical_harmonics::rotate_sh_coefs<2, float>,
            &agz::math::spherical_harmonics::rotate_sh_coefs<3, float>,
            &agz::math::spherical_harmonics::rotate_sh_coefs<4, float>,
        };

        assert(order < agz::array_size<int>(rot_funcs));
        rot_funcs[order](rot, coefs);
    }

} // namespace anonymous

std::vector<Float3> computeEnvSHCoefs(
    const agz::texture::texture2d_t<Float3> &env,
    int                                      maxOrder,
    int                                      numSamples)
{
    const int SHCount = agz::math::sqr(maxOrder + 1);
    auto SHFuncs = agz::math::spherical_harmonics::linear_table<float>();
    
    std::default_random_engine rng{ std::random_device()() };
    std::uniform_real_distribution<float> uniform01;

    std::vector<Float3> result(SHCount);

    for(int i = 0; i < numSamples; ++i)
    {
        const float u1 = uniform01(rng), u2 = uniform01(rng);
        const auto sample = agz::math::distribution::uniform_on_sphere(u1, u2);

        const Float3 l = sampleEnv(env, sample.first) / sample.second;
        for(int j = 0; j < SHCount; ++j)
            result[j] += l * SHFuncs[j](sample.first);
    }

    const float invNumSamples = 1.0f / numSamples;
    for(auto &s : result)
        s *= invNumSamples;

    return result;
}

void rotateEnvSHCoefs(
    const agz::math::mat3f_c &rot, std::vector<Float3> &coefs)
{
    std::vector<float> channel_coefs;

    const int N = static_cast<int>(coefs.size());
    int base = 0, order = 0;
    while(base < N)
    {
        const int cnt = 2 * order + 1;
        channel_coefs.resize(cnt);

        for(int ci = 0; ci < 3; ++ci)
        {
            for(int i = 0; i < cnt; ++i)
                channel_coefs[i] = coefs[base + i][ci];

            rotateEnvSHCoefs(rot, order, channel_coefs.data());

            for(int i = 0; i < cnt; ++i)
                coefs[base + i][ci] = channel_coefs[i];
        }

        base += cnt;
        ++order;
    }
}
