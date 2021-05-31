#pragma once

#include <common/common.h>

struct Vertex
{
    Float3 position;
    Float3 normal;
    Float3 albedo;
};

struct DirectionalLight
{
    Float3 direction;
    float  pad0;
    Float3 radiance;
    float  pad1;
};
