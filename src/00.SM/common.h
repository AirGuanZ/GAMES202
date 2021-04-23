#pragma once

#include <common/common.h>

struct MeshVertex
{
    Float3 position;
    Float3 normal;
};

struct Light
{
    Float3 position;
    float  fadeCosBegin;

    Float3 direction;
    float  fadeCosEnd;

    Float3 incidence;
    float  ambient;
};
