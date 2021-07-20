#include <random>

#include <agz-utils/thread.h>

#include "lut.h"

namespace
{
#define float3 Float3
#include "../../../asset/03/brdf.hlsl"
#undef float3

    Float2 hammersley(uint32_t i, uint32_t N)
    {
        uint32_t bits = (i << 16u) | (i >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        const float rdi = static_cast<float>(bits * 2.3283064365386963e-10);
        return { static_cast<float>(i) / N, rdi };
    }

    Float3 sampleGGX(float roughness, float u1, float u2)
    {
        float alpha = roughness * roughness;
        const float theta = std::atan(alpha * std::sqrt(u1) / std::sqrt(1 - u1));
        const float phi   = 2 * PI * u2;

        const Float3 wh =
        {
            std::sin(phi) * std::sin(theta),
            std::cos(phi) * std::sin(theta),
            std::cos(theta),
        };

        return wh;
    }

    float computeEmiu(float nDotO, float roughness, int spp)
    {
        const Float3 wo = Float3(
            std::sqrt((std::max)(0.0f, 1 - nDotO * nDotO)), 0, nDotO);

        float sum = 0;
        for(int i = 0; i < spp; ++i)
        {
            const Float2 sam = hammersley(i, spp);

            const Float3 wh = sampleGGX(roughness, sam.x, sam.y);
            const Float3 wi = (2 * wh * dot(wh, wo) - wo).normalize();
            if(wi.z <= 0)
                continue;
            
            const float G = ggxSmith(wi.z, wo.z, roughness);
            const float item = G * dot(wo, wh) / (wo.z * wh.z);

            if(std::isfinite(item))
                sum += item;
        }

        return (std::min)(sum / spp, 1.0f);
    }

    agz::texture::texture2d_t<float> generateEmiu(const Int2 &res, int spp)
    {
        agz::texture::texture2d_t<float> result(res.y, res.x);

        agz::thread::parallel_forrange(0, res.y,
            [&](int threadIdx, int y)
        {
            const float roughness = (y + 0.5f) / res.y;
            for(int x = 0; x < res.x; ++x)
            {
                const float nDotO = (x + 0.5f) / res.x;
                result(y, x) = computeEmiu(nDotO, roughness, spp);
            }
        });

        return result;
    }

    std::vector<float> generateEavg(
        const agz::texture::texture2d_t<float> &Emiu)
    {
        std::vector<float> result(Emiu.height());

        const float deltaMiu = 1.0f / Emiu.width();

        for(int y = 0; y < Emiu.height(); ++y)
        {
            float sum = 0;
            for(int x = 0; x < Emiu.width(); ++x)
            {
                const float miu = (x + 0.5f) / Emiu.width();
                sum += miu * Emiu(y, x) * deltaMiu;
            }

            result[y] = 2 * sum;
        }

        return result;
    }

} // namespace anonymous

LUTGenerator::LUT LUTGenerator::generate(const Int2 &res, int spp) const
{
    const auto EmiuData = generateEmiu(res, spp);
    const auto EavgData = generateEavg(EmiuData);

    auto Emiu = Texture2DLoader::loadFromMemory(
        DXGI_FORMAT_R32_FLOAT, res.x, res.y, 1, 1, EmiuData.raw_data());

    auto Eavg = Texture2DLoader::loadFromMemory(
        DXGI_FORMAT_R32_FLOAT, res.y, 1,
        1, 1, EavgData.data());

    return { Emiu, Eavg };
}
