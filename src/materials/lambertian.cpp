/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/factory.h>
#include <darts/material.h>
#include <darts/scene.h>
#include <darts/texture.h>
#include <darts/onb.h>

/// A perfectly diffuse (%Lambertian) material. \ingroup Materials
class Lambertian : public Material
{
public:
    Lambertian(const json &j = json::object());

    bool scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const override;

    virtual bool sample(const Vec3f &wi, const HitInfo &hit, ScatterRecord &srec, const Vec2f &rv, float rv1) const override;

    virtual Color3f eval(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const override;

    virtual float pdf(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const override;

    std::shared_ptr<class Texture> albedo;
};

Lambertian::Lambertian(const json &j) : Material(j)
{
    albedo = DartsFactory<Texture>::create(j.at("albedo"));
}

bool Lambertian::scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const
{
    // TODO: Implement Lambertian reflection
    //       You should assign the albedo to ``attenuation'', and
    //       you should assign the scattered ray to ``scattered''
    //       The origin of the scattered ray should be at the hit point,
    //       and the scattered direction is the shading normal plus a random
    //       point on a sphere (please look at the text book for this)

    //       You can get the hit point using hit.p, and the shading normal using hit.sn

    //       Hint: You can use the function random_in_unit_sphere() to get a random
    //       point in a sphere. IMPORTANT: You want to add a random point *on*
    //       a sphere, not *in* the sphere (the text book gets this wrong)
    //       If you normalize the point, you can force it to be on the sphere always, so
    //       add normalize(random_in_unit_sphere()) to your shading normal
    attenuation = albedo->value(ray.d, hit);
    Vec3f unit_sphere_p = normalize(random_in_unit_sphere());
    Vec3f target = hit.p + hit.sn + unit_sphere_p;
    Vec3f out_dir = target - hit.p;
    if (dot(normalize(out_dir), hit.sn) < -Ray3f::epsilon) 
    {
        out_dir = -out_dir;
    }
    scattered = Ray3f(hit.p, out_dir);
    return true;
}

bool Lambertian::sample(const Vec3f &wi, const HitInfo &hit, ScatterRecord &srec, const Vec2f &rv, float rv1) const
{
    srec.is_specular = false;

    ONBf onb(hit.sn); 
    srec.wo = onb.to_world(sample_hemisphere_cosine(rv));

    srec.attenuation = albedo->value(wi, hit);

    return true;
}

Color3f Lambertian::eval(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const
{
    return albedo->value(wi, hit) * std::max(0.f, dot(scattered, hit.sn)) / M_PI;
}

float Lambertian::pdf(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const
{
    return std::max(0.f, dot(scattered, hit.sn)) / M_PI;
}


DARTS_REGISTER_CLASS_IN_FACTORY(Material, Lambertian, "lambertian")

/**
    \file
    \brief Lambertian Material
*/
