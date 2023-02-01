/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/factory.h>
#include <darts/texture.h>
#include <darts/scene.h>
#include <darts/json.h>

#include <darts/perlin.h>

class MarbleTexture : public Texture
{
public:
    MarbleTexture(const json &j = json::object());

    virtual Color3f value(const Vec3f &wi, const HitInfo &hit) const override;    
protected:
    shared_ptr<Perlin> perlin;

    Color3f veins = Color3f(0.f);
    Color3f base = Color3f(1.f);
    float scale = 1.f;
};

MarbleTexture::MarbleTexture(const json &j)
    : Texture(j)
    , perlin(make_shared<Perlin>())
{
    veins = j.value("veins", veins);
    base = j.value("base", base);
    scale = j.value("scale", scale);
}

Color3f MarbleTexture::value(const Vec3f &wi, const HitInfo &hit) const
{
    float alpha = 0.5f * (1.f + std::sin(scale * hit.p.z + 10 * perlin->turb(hit.p)));
    return lerp(veins, base, alpha);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Texture, MarbleTexture, "marble")