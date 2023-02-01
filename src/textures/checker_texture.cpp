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

    float scale = 1.f;
};

CheckerTexture::CheckerTexture(const json &j)
    : Texture(j)
{
    m_even_tex = DartsFactory<Texture>::create(j.at("even"));
    m_odd_tex = DartsFactory<Texture>::create(j.at("odd"));

    scale = j.value("scale", scale);
}

Color3f CheckerTexture::value(const Vec3f &wi, const HitInfo &hit) const
{
    Vec3f p = xform.point(hit.p * scale * 30);
    int sum = ((int)std::floor(p.x)) + ((int)std::floor(p.y)) + ((int)std::floor(p.z));
    if (sum % 2 == 0)
    {
        return m_even_tex->value(wi, hit);
    }
    else
    {
        return m_odd_tex->value(wi, hit);
    }
    /*
    auto sum = std::sin(10 * p.x) * std::sin(10 * p.y) * std::sin(10 * p.z);
    if (sines < 0)
    {
        return m_odd_tex->value(wi, hit);
    }
    else
    {
        return m_even_tex->value(wi, hit);
    }
    */
}

DARTS_REGISTER_CLASS_IN_FACTORY(Texture, CheckerTexture, "checker")