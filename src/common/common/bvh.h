#pragma once

#include <common/ray.h>

class BVH
{
public:

    struct Triangle
    {
        int index;
        Float3 a, b_a, c_a;
    };

    struct Node
    {
        static constexpr uint32_t TRI_NIL =
            (std::numeric_limits<uint32_t>::max)();

        Float3 lower;
        Float3 upper;

        // interior node when prim_beg == TRI_NIL
        // left_child is *(this + 1)
        uint32_t triBeg;

        union
        {
            uint32_t triEnd;
            uint32_t rightChild;
        };
    };

    struct Intersection
    {
        int    triangle;
        Float3 position;
        Float2 uv;
        float  t;
    };

    static constexpr int TRAVERSAL_STACK_SIZE = 256;

    static BVH create(const Float3 *triangleVertices, int triangleCount);

    bool hasIntersection(const Ray &ray) const;

    bool findIntersection(const Ray &ray, Intersection *inct) const;

private:

    std::vector<Node>     nodes_;
    std::vector<Triangle> triangles_;
};
