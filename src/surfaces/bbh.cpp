/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/parallel.h>
#include <darts/progress.h>
#include <darts/sampling.h>
#include <darts/stats.h>
#include <darts/surface_group.h>
#include <future>

#include <functional>

struct BBHNode;

// STAT_MEMORY_COUNTER("Memory/BBH", treeBytes);
STAT_RATIO("BBH/Surfaces per leaf node", total_surfaces, total_leaf_nodes);
STAT_COUNTER("BBH/Interior nodes", interior_nodes);
STAT_COUNTER("BBH/Leaf nodes", leaf_nodes);
STAT_RATIO("BBH/Nodes visited per ray", bbh_nodes_visited, total_rays);

/// An axis-aligned bounding box hierarchy acceleration structure. \ingroup Surfaces
struct BBH : public SurfaceGroup
{
    shared_ptr<BBHNode> root;
    enum class SplitMethod
    {
        SAH,
        Middle,
        Equal
    } split_method    = SplitMethod::Middle;
    int max_leaf_size = 1;

    BBH(const json &j = json::object());

    /// Construct the BBH (must be called before @ref intersect)
    void build() override;

    /// Intersect a ray against all surfaces registered with the Accelerator
    bool intersect(const Ray3f &ray, HitInfo &hit) const override;
};


/// A lighter-weight version of SurfaceGroup for BBH leaf nodes that need to store multiple surfaces, but which don't
/// need to store additional information like a transform or explicitly stored bounds.
struct BBHLeaf : public Surface
{
public:
    bool intersect(const Ray3f &ray_, HitInfo &hit) const override
    {
        // copy the ray so we can modify the tmax values as we traverse
        Ray3f ray          = ray_;
        bool  hit_anything = false;

        // This is a linear intersection test that iterates over all primitives
        // within the scene. It's the most naive intersection test and hence very
        // slow if you have many primitives.
        for (auto surface : surfaces)
        {
            if (surface->intersect(ray, hit))
            {
                hit_anything = true;
                ray.maxt     = hit.t;
            }
        }

        // record closest intersection
        return hit_anything;
    }

    Box3f bounds() const override
    {
        Box3f b;
        for (auto &s : surfaces)
            b.enclose(s->bounds());

        return b;
    }

    vector<shared_ptr<Surface>> surfaces; ///< All children
};

/// A node of an axis-aligned bounding box hierarchy. \ingroup Surfaces
struct BBHNode : public Surface
{
    Box3f               bbox;        ///< The bounding box of this node
    shared_ptr<Surface> left_child;  ///< Pointer to left child
    shared_ptr<Surface> right_child; ///< Pointer to right child

    BBHNode(vector<shared_ptr<Surface>> surfaces, Progress &progress, int depth = 0);

    ~BBHNode();

    bool intersect(const Ray3f &ray, HitInfo &hit) const override;

    Box3f bounds() const override
    {
        return bbox;
    }
};

namespace
{
    bool box_compare(const shared_ptr<Surface>& op1, const shared_ptr<Surface>& op2, int axis)
    {
        return op1->bounds().center()[axis] < op2->bounds().center()[axis];
    }

    std::function<bool(const shared_ptr<Surface>& op1, const shared_ptr<Surface>& op2)> comparors[] = {
        std::bind(box_compare, std::placeholders::_1, std::placeholders::_2, 0), 
        std::bind(box_compare, std::placeholders::_1, std::placeholders::_2, 1), 
        std::bind(box_compare, std::placeholders::_1, std::placeholders::_2, 2)};

    void compute_surfaces_bound(Box3f& bbox, const vector<shared_ptr<Surface>>& surfaces)
    {
        for (auto surface : surfaces)
        {
            bbox.enclose(surface->bounds());
        }
    }
}

BBHNode::BBHNode(vector<shared_ptr<Surface>> surfaces, Progress &progress, int depth)
{
    // TODO: Implement BBH construction, following chapter 2 of the book.
    // Hints:
    //     -- To get a random number in [0, n-1], you can use int(randf()*n)
    //     -- To sort a list of surfaces based on some sorting criterion, you can use std::sort and lambdas, like so:
    //
    //          std::sort(surfaces.begin(), surfaces.end(), [&](const Surface *a, const Surface *b) {
    //              return isASmallerThanB(a, b);
    //          });
    //
    //     For example, to sort them based on centroid along an axis, you could do
    //
    //          return a->bounds().center()[axis] < b->bounds().center()[axis]
    //
    //     -- To split a list (in C++: a 'vector') of things into two lists so that the first list has the first
    //     k elements of the original list, and the other list has the rest of the elements, you can do
    //          std::vector<Surface*> listA, listB
    //          listA.insert(listA.end(), originalList.begin(), originalList.begin() + k);
    //          listB.insert(listB.end(), originalList.begin() + k, originalList.end();
    //
    //     -- After construction, you need to compute the bounding box of this BBH node and assign it to bbox
    //     -- You can get the bounding box of a surface using surf->bounds();
    //     -- To take the union of two boxes, look at Box2f::enclose()
    if (surfaces.size() == 0)
        return;
        
    if (surfaces.size() > 0)
    {
        bbox = surfaces[0]->bounds();

        for (uint32_t i = 1; i < surfaces.size(); ++i)
        {
            bbox.enclose(surfaces[i]->bounds());
        }
    }

    int axis = (int)(randf() * 3);
    auto comp = comparors[axis];

    vector<shared_ptr<Surface>> left_surfaces;
    vector<shared_ptr<Surface>> right_surfaces;

    if (surfaces.size() == 1)
    {
        left_surfaces = right_surfaces = surfaces;

        progress.step(1);
    }
    else if (surfaces.size() == 2)
    {
        if (comp(surfaces[0], surfaces[1]))
        {
            left_surfaces.push_back(surfaces[0]);
            right_surfaces.push_back(surfaces[1]);
        }
        else
        {
            left_surfaces.push_back(surfaces[1]);
            right_surfaces.push_back(surfaces[0]);
        }

        progress.step(2);
    }
    else
    {
        std::sort(surfaces.begin(), surfaces.end(), comp);

        int mid = surfaces.size() / 2;
        left_surfaces.assign(surfaces.begin(), surfaces.begin() + mid);
        right_surfaces.assign(surfaces.begin() + mid, surfaces.end());

        if (surfaces.size() == 3)
        {
            progress.step(1);
        }
    }

    if (left_surfaces.size() == 1)
    {
        auto left_leaf = make_shared<BBHLeaf>();
        left_leaf->surfaces = left_surfaces;
        left_child = left_leaf;
    }
    else
    {
        left_child = make_shared<BBHNode>(left_surfaces, progress, depth + 1);
    }

    if (right_surfaces.size() == 1)
    {
        auto right_leaf = make_shared<BBHLeaf>();
        right_leaf->surfaces = right_surfaces;
        right_child = right_leaf;
    }
    else
    {
        right_child = make_shared<BBHNode>(right_surfaces, progress, depth + 1);
    }
}


BBHNode::~BBHNode()
{
}

bool BBHNode::intersect(const Ray3f &ray_, HitInfo &hit) const
{
    ++bbh_nodes_visited;
    // TODO: Implement BBH intersection, following chapter 2 of the book.
    if (!bbox.intersect(ray_))
        return false;
    
    Ray3f tray(ray_);
    bool hit_left = false;
    bool hit_right = false;
    if (left_child)
    {
        hit_left = left_child->intersect(tray, hit);
    }
    if (hit_left)
    {
        tray.maxt = hit.t;
    }
    if (right_child)
    {
        hit_right = right_child->intersect(tray, hit);
    }

    return hit_left || hit_right;
}

BBH::BBH(const json &j) : SurfaceGroup(j)
{
    // These values aren't used in the base code right now - but you can use these for when you want to extend the basic
    // BBH functionality

    max_leaf_size = j.value("max_leaf_size", max_leaf_size);

    string sm = j.value("split_method", "equal");
    if (sm == "sah")
        // Surface-area heuristic
        split_method = SplitMethod::SAH;
    else if (sm == "middle")
        // Split at the center of the bounding box
        split_method = SplitMethod::Middle;
    else if (sm == "equal")
        // Split so that an equal number of objects are on either side
        split_method = SplitMethod::Equal;
    else
    {
        spdlog::error("Unrecognized split_method \"{}\". Using \"equal\" instead.", sm);
        split_method = SplitMethod::Equal;
    }
}

void BBH::build()
{
    Progress progress("Building BBH", m_surfaces.size());
    if (!m_surfaces.empty())
        root = make_shared<BBHNode>(m_surfaces, progress);
    else
        root = nullptr;
    progress.set_done();
    spdlog::info("BBH contains {} surfaces.", m_surfaces.size());
}

bool BBH::intersect(const Ray3f &ray_, HitInfo &hit) const
{
    ++total_rays;
    if (!root)
        return false;

    auto ray           =ray_;
    bool hit_something = root->intersect(ray, hit);

    return hit_something;
}

DARTS_REGISTER_CLASS_IN_FACTORY(Surface, BBH, "bbh")
// this clumsy notation with the extra namespace is needed since we want to register BBH in both the Surface and
// SurfaceGroup factories, and the DARTS_REGISTER_CLASS_IN_FACTORY macros would create duplicate definitions otherwise
namespace
{
DARTS_REGISTER_CLASS_IN_FACTORY(SurfaceGroup, BBH, "bbh")
}

/**
    \file
    \brief BBH SurfaceGroup
*/