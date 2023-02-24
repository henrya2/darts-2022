#pragma once

#include <darts/common.h>

#include <darts/ray.h>

class Integrator
{
public:
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
    {
        return Color3f(1, 0, 1);
    }
};