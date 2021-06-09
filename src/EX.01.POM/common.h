#pragma once

#include <common/common.h>

struct Vertex
{
    Float3 position;
    Float3 normal;
    Float3 tangent;
    Float2 texCoord;
    float  dTangent; // length per texCoord.x along tangent
    float  dBinormal;
};

struct Mesh
{
    VertexBuffer<Vertex> vertexBuffer;

    ComPtr<ID3D11ShaderResourceView> albedo;
    ComPtr<ID3D11ShaderResourceView> height;

    Mat4  world;
    float heightScale;
};

struct DirectionalLight
{
    Float3 radiance;  float pad0;
    Float3 direction; float pad1;
    Float3 ambient;   float pad2;
};
