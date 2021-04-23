#pragma once

#include <limits>

#include <common/common.h>

class Ray
{
public:

    Float3 o;
    Float3 d;
    float t0;
    float t1;

    Ray()
        : Ray({}, { 1, 0, 0 })
    {
        
    }

    Ray(
        const Float3 &o,
        const Float3 &d,
        float         t0 = 0,
        float         t1 = std::numeric_limits<float>::infinity())
        : o(o), d(d), t0(t0), t1(t1)
    {
        
    }

    Float3 at(float t) const
    {
        return o + t * d;
    }

    bool isBetween(float t) const
    {
        return t0 <= t && t <= t1;
    }
};
