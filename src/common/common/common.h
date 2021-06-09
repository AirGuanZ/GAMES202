#pragma once

#include <agz-utils/graphics_api.h>

using namespace agz::d3d11;

constexpr float PI = agz::math::PI_f;

inline Float3 computeTangent(
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
