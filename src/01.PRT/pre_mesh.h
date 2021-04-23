#pragma once

#include "common.h"

struct SHVertex
{
    Float3 position;
    Float3 normal;
};

std::vector<float> computeVertexSHCoefs(
    const SHVertex *vertices,
    int             vertexCount,
    float           vertexAlbedo,
    int             maxOrder,
    int             samplesPerVertex,
    LightingMode    mode);
