#pragma once

#include <common/common.h>

struct Vertex
{
    Float3 position;
    Float3 normal;
    Float3 tangent;
    Float2 texCoord;
};

struct Mesh
{
    Mat4                             world;
    VertexBuffer<Vertex>             vertexBuffer;
    ComPtr<ID3D11ShaderResourceView> albedo;
    ComPtr<ID3D11ShaderResourceView> normal;
};

struct DirectionalLight
{
    Float3 direction;
    float  pad0;
    Float3 radiance;
    float  pad1;
};
