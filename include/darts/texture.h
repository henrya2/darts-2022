/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/
#pragma once

#include <darts/common.h>
#include <darts/factory.h>
#include <darts/image.h>

// Base class for all textures
class Texture
{
public:
    Texture(const json &j = json::object());
    virtual ~Texture() = default;

    virtual Color3f value(const Vec3f &wi, const HitInfo &hit) const = 0;

protected:
    Transform xform;
};