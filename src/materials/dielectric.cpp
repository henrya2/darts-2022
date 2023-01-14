/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/factory.h>
#include <darts/material.h>
#include <darts/scene.h>

/// A smooth dielectric surface that reflects and refracts light according to the specified index of refraction #ior.
/// \ingroup Materials
class Dielectric : public Material
{
public:
    Dielectric(const json &j = json::object());

    bool scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const override;


    float ior; ///< The (relative) index of refraction of the material
};

Dielectric::Dielectric(const json &j) : Material(j)
{
    ior = j.value("ior", ior);
}

bool Dielectric::scatter(const Ray3f &ray, const HitInfo &hit, Color3f &attenuation, Ray3f &scattered) const
{
    // TODO: Implement dielectric scattering
    attenuation = Color3f(1.f, 1.f, 1.f);
    float cos_theta_i = dot(normalize(-ray.d), hit.sn);
    bool entering = cos_theta_i > 0.0f;
    float refraction_ratio = entering ? (1.f / ior) : ior;
    float fr = fresnel_dielectric(cos_theta_i, 1.f, ior);

    Vec3f scatter_dir;

    Vec3f refracted;
    if (!refract(normalize(ray.d), hit.sn, refraction_ratio, refracted) || fr > randf())
    {
        scatter_dir = reflect(ray.d, hit.sn);
    }
    else
    {
        scatter_dir = refracted;
    }
    scattered = Ray3f(hit.p, normalize(scatter_dir));
    return true;
}


DARTS_REGISTER_CLASS_IN_FACTORY(Material, Dielectric, "dielectric")

/**
    \file
    \brief Dielectric Material
*/
