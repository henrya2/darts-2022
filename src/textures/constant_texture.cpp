/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/factory.h>
#include <darts/texture.h>
#include <darts/scene.h>
#include <darts/json.h>

// ConstantTexture
class ConstantTexture : public Texture
{
public:
    ConstantTexture(const json &j = json::object());

    virtual Color3f value(const Vec3f &wi, const HitInfo &hit) const override;

    Color3f color = Color3f(0.8f);
};

ConstantTexture::ConstantTexture(const json &j)
    : Texture(j)
{
    if (j.is_object())
    {
        color = j.value("color", color);
    }
    else if (j.is_array() || j.is_number())
    {
        j.get_to(color);
    }
}

Color3f ConstantTexture::value(const Vec3f &wi, const HitInfo &hit) const
{
    return color;
}

DARTS_REGISTER_CLASS_IN_FACTORY(Texture, ConstantTexture, "constant")