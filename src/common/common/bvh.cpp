#include <queue>
#include <stack>

#include <agz-utils/alloc.h>

#include <common/bvh.h>

namespace
{

    using Arena = agz::alloc::obj_arena_t;
    using AABB  = agz::math::aabb3f;

    struct BuildNode
    {
        AABB       aabb;
        BuildNode *left   = nullptr;
        BuildNode *right  = nullptr;
        uint32_t   triBeg = 0;
        uint32_t   triEnd = 0;
    };

    struct BuildTriangle
    {
        Float3 vertices[3];
        Float3 centroid;
        int    index;
    };

    struct BuildResult
    {
        BuildNode *root;
        int        node_count;
    };

    BuildResult buildLinkedBVH(
        BuildTriangle *triangles,
        int            triangleCount,
        int            leafSizeThreshold,
        Arena         &arena)
    {
        struct BuildTask
        {
            BuildNode **fillbackPtr;
            int triBeg, triEnd;
            int depth;
        };

        BuildResult result = { nullptr, 0 };

        std::queue<BuildTask> tasks;
        tasks.push({ &result.root, 0, triangleCount, 0 });

        while(!tasks.empty())
        {
            const BuildTask task = tasks.front();
            tasks.pop();

            assert(task.triBeg < task.triEnd);

            AABB allBound, centroidBound;
            for(int i = task.triBeg; i < task.triEnd; ++i)
            {
                auto &tri = triangles[i];
                allBound |= tri.vertices[0];
                allBound |= tri.vertices[1];
                allBound |= tri.vertices[2];
                centroidBound |= tri.centroid;
            }

            const int n = task.triEnd - task.triBeg;
            if(n <= leafSizeThreshold)
            {
                ++result.node_count;

                auto leaf = arena.create<BuildNode>();
                leaf->aabb   = allBound;
                leaf->left   = nullptr;
                leaf->right  = nullptr;
                leaf->triBeg = task.triBeg;
                leaf->triEnd = task.triEnd;

                *task.fillbackPtr = leaf;

                continue;
            }

            const auto centroidDelta =
                centroidBound.high - centroidBound.low;
            const int splitAxis = centroidDelta[0] > centroidDelta[1] ?
                (centroidDelta[0] > centroidDelta[2] ? 0 : 2) :
                (centroidDelta[1] > centroidDelta[2] ? 1 : 2);
            
            std::sort(
                triangles + task.triBeg, triangles + task.triEnd,
                [axis = splitAxis]
            (const BuildTriangle &L, const BuildTriangle &R)
            {
                return L.centroid[axis] < R.centroid[axis];
            });
            const int splitMiddle = task.triBeg + n / 2;

            auto interior = arena.create<BuildNode>();
            interior->aabb    = allBound;
            interior->left    = nullptr;
            interior->right   = nullptr;
            interior->triBeg = 0;
            interior->triEnd = 0;

            *task.fillbackPtr = interior;
            ++result.node_count;

            tasks.push({
                &interior->left,
                task.triBeg,
                splitMiddle,
                task.depth + 1
            });

            tasks.push({
                &interior->right,
                splitMiddle,
                task.triEnd,
                task.depth + 1
            });
        }

        return result;
    }

    void linearizeBVH(
        const BuildNode     *buildNode,
        const BuildTriangle *triangles,
        BVH::Node           *nodeArr,
        BVH::Triangle       *triArr)
    {
        struct CompactingTask
        {
            const BuildNode *tree;
            uint32_t *fillbackPtr;
        };

        uint32_t nextNodeIdx = 0, nextTriIdx = 0;

        std::stack<CompactingTask> tasks;
        tasks.push({ buildNode, nullptr });

        while(!tasks.empty())
        {
            const CompactingTask task = tasks.top();
            tasks.pop();

            const BuildNode *tree = task.tree;

            if(task.fillbackPtr)
                *task.fillbackPtr = nextNodeIdx;

            if(tree->left && tree->right)
            {
                auto &node = nodeArr[nextNodeIdx++];
                node.lower      = tree->aabb.low;
                node.upper      = tree->aabb.high;
                node.triBeg     = BVH::Node::TRI_NIL;
                node.rightChild = 0;
                tasks.push({ tree->right, &node.rightChild});
                tasks.push({ tree->left, nullptr });
            }
            else
            {
                const uint32_t beg = nextTriIdx;
                const uint32_t end = nextTriIdx + (tree->triEnd - tree->triBeg);

                auto &node = nodeArr[nextNodeIdx++];
                node.lower   = tree->aabb.low;
                node.upper   = tree->aabb.high;
                node.triBeg  = beg;
                node.triEnd  = end;

                for(uint32_t i = beg, j = tree->triBeg; i < end; ++i, ++j)
                {
                    assert(j < tree->triEnd);

                    auto &buildTri = triangles[j];
                    auto &tri = triArr[i];
                    
                    tri.a     = buildTri.vertices[0];
                    tri.b_a   = buildTri.vertices[1] - buildTri.vertices[0];
                    tri.c_a   = buildTri.vertices[2] - buildTri.vertices[0];
                    tri.index = buildTri.index;
                }

                nextTriIdx = end;
            }
        }
    }

    bool hasIntersectionWithTriangle(
        const Ray    &r,
        const Float3 &A,
        const Float3 &B_A,
        const Float3 &C_A) noexcept
    {
        const Float3 s1 = cross(r.d, C_A);
        const float div = dot(s1, B_A);
        if(div == 0.0f)
            return false;
        const float invDiv = 1 / div;
    
        const Float3 o_A = r.o - A;
        const float alpha = dot(o_A, s1) * invDiv;
        if(alpha < 0 || alpha > 1)
            return false;
    
        const Float3 s2  = cross(o_A, B_A);
        const float beta = dot(r.d, s2) * invDiv;
        if(beta < 0 || alpha + beta > 1)
            return false;
    
        const float t = dot(C_A, s2) * invDiv;
        return r.isBetween(t);
    }

    bool closestIntersectionWithTriangle(
        const Ray    &r,
        const Float3 &A,
        const Float3 &B_A,
        const Float3 &C_A,
        float        *r_t,
        Float2       *uv) noexcept
    {
        const Float3 s1 = cross(r.d, C_A);
        const float div = dot(s1, B_A);
        if(div == 0.0f)
            return false;
        const float invDiv = 1 / div;

        const Float3 o_A = r.o - A;
        const float alpha = dot(o_A, s1) * invDiv;
        if(alpha < 0)
            return false;

        const Float3 s2 = cross(o_A, B_A);
        const float beta = dot(r.d, s2) * invDiv;
        if(beta < 0 || alpha + beta > 1)
            return false;

        const float t = dot(C_A, s2) * invDiv;
        if(!r.isBetween(t))
            return false;

        *r_t = t;
        *uv  = Float2(alpha, beta);

        return true;
    }

    bool bboxHasIntersection(
        const BVH::Node &node,
        const Float3    &ori,
        const Float3    &invDir,
        float            t0,
        float            t1)
    {
        const float nx = invDir[0] * (node.lower[0] - ori[0]);
        const float ny = invDir[1] * (node.lower[1] - ori[1]);
        const float nz = invDir[2] * (node.lower[2] - ori[2]);

        const float fx = invDir[0] * (node.upper[0] - ori[0]);
        const float fy = invDir[1] * (node.upper[1] - ori[1]);
        const float fz = invDir[2] * (node.upper[2] - ori[2]);

        t0 = (std::max)(t0, (std::min)(nx, fx));
        t0 = (std::max)(t0, (std::min)(ny, fy));
        t0 = (std::max)(t0, (std::min)(nz, fz));

        t1 = (std::min)(t1, (std::max)(nx, fx));
        t1 = (std::min)(t1, (std::max)(ny, fy));
        t1 = (std::min)(t1, (std::max)(nz, fz));

        return t0 <= t1;
    }

    bool isLeaf(const BVH::Node &node)
    {
        return node.triBeg != BVH::Node::TRI_NIL;
    }

    thread_local uint32_t traversalStack[BVH::TRAVERSAL_STACK_SIZE];

} // namespace anonymous

BVH BVH::create(const Float3 *triangle_vertices, int triangle_count)
{
    assert(triangle_vertices && triangle_count > 0);

    std::vector<BuildTriangle> build_triangles(triangle_count);
    for(int i = 0; i < triangle_count; ++i)
    {
        auto &dst = build_triangles[i];
        auto  src = &triangle_vertices[3 * i];
        dst.vertices[0] = src[0];
        dst.vertices[1] = src[1];
        dst.vertices[2] = src[2];
        dst.index       = i;
    }

    Arena arena;
    auto [root, node_count] = buildLinkedBVH(
        build_triangles.data(), triangle_count, 4, arena);
    
    BVH bvh;
    bvh.nodes_.resize(node_count);
    bvh.triangles_.resize(triangle_count);
    
    linearizeBVH(
        root, build_triangles.data(),
        bvh.nodes_.data(), bvh.triangles_.data());
    
    return bvh;
}

bool BVH::hasIntersection(const Ray &ray) const
{
    const Float3 invDir = { 1 / ray.d.x, 1 / ray.d.y, 1 / ray.d.z };

    if(!bboxHasIntersection(nodes_[0], ray.o, invDir, ray.t0, ray.t1))
        return false;

    int top = 0;
    traversalStack[top++] = 0;

    while(top)
    {
        const uint32_t task_node_idx = traversalStack[--top];
        const Node &node = nodes_[task_node_idx];

        if(isLeaf(node))
        {
            for(uint32_t i = node.triBeg; i < node.triEnd; ++i)
            {
                const Triangle &tri = triangles_[i];
                if(hasIntersectionWithTriangle(
                    ray, tri.a, tri.b_a, tri.c_a))
                    return true;
            }
        }
        else
        {
            assert(top + 2 < TRAVERSAL_STACK_SIZE);

            if(bboxHasIntersection(
                nodes_[task_node_idx + 1], ray.o, invDir, ray.t0, ray.t1))
                traversalStack[top++] = task_node_idx + 1;

            if(bboxHasIntersection(
                nodes_[node.rightChild], ray.o, invDir, ray.t0, ray.t1))
                traversalStack[top++] = node.rightChild;
        }
    }

    return false;
}

bool BVH::findIntersection(const Ray &ray, Intersection *inct) const
{
    auto r = ray;
    const Float3 invDir = { 1 / r.d.x, 1 / r.d.y, 1 / r.d.z };

    if(!bboxHasIntersection(
        nodes_[0], r.o, invDir, r.t0, r.t1))
        return false;

    int top = 0;
    traversalStack[top++] = 0;

    uint32_t finalIndex = 0;
    Float2 finalUV;
    float finalT = std::numeric_limits<float>::infinity();

    while(top)
    {
        const uint32_t taskNodeIdx = traversalStack[--top];
        const Node &node = nodes_[taskNodeIdx];

        if(isLeaf(node))
        {
            for(uint32_t i = node.triBeg; i < node.triEnd; ++i)
            {
                const Triangle &tri = triangles_[i];
                if(closestIntersectionWithTriangle(
                    r, tri.a, tri.b_a, tri.c_a, &finalT, &finalUV))
                {
                    r.t1 = finalT;
                    finalIndex = tri.index;
                }
            }
        }
        else
        {
            assert(top + 2 < TRAVERSAL_STACK_SIZE);

            const bool addLeft = bboxHasIntersection(
                nodes_[taskNodeIdx + 1], r.o, invDir, r.t0, r.t1);
            if(addLeft)
                traversalStack[top++] = taskNodeIdx + 1;

            const bool addRight = bboxHasIntersection(
                nodes_[node.rightChild], r.o, invDir, r.t0, r.t1);
            if(addRight)
                traversalStack[top++] = node.rightChild;
        }
    }

    if(isinf(finalT))
        return false;

    inct->triangle = finalIndex;
    inct->position = r.at(finalT); // IMPROVE: better precision
    inct->t        = finalT;
    inct->uv       = finalUV;

    return true;
}
