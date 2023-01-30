/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/factory.h>
#include <darts/texture.h>
#include <darts/scene.h>
#include <darts/json.h>

// CheckerTexture
class CheckerTexture : public Texture
{
public:
    CheckerTexture(const json &j = json::object());

    virtual Color3f value(const Vec3f &wi, const HitInfo &hit) const;

protected:
    shared_ptr<Texture> m_even_tex;
    shared_ptr<Texture> m_odd_tex;
};

CheckerTexture::CheckerTexture(const json &j)
    : Texture(j)
{
    m_even_tex = DartsFactory<Texture>::create(j.at("even"));
    m_odd_tex = DartsFactory<Texture>::create(j.at("odd"));
}

Color3f CheckerTexture::value(const Vec3f &wi, const HitInfo &hit) const
{
    auto sines = std::sin(10 * hit.p.x) * std::sin(10 * hit.p.y) * std::sin(10 * hit.p.z);
    if (sines < 0)
    {
        return m_odd_tex->value(wi, hit);
    }
    else
    {
        return m_even_tex->value(wi, hit);
    }
}

DARTS_REGISTER_CLASS_IN_FACTORY(Texture, CheckerTexture, "checker")