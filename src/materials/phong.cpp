#include <darts/factory.h>
#include <darts/material.h>
#include <darts/scene.h>
#include <darts/texture.h>
#include <darts/onb.h>

class Phong : public Material
{
public:
    Phong(const json &j = json::object());

    virtual bool sample(const Vec3f &wi, const HitInfo &hit, ScatterRecord &srec, const Vec2f &rv, float rv1) const override;

    virtual Color3f eval(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const override;

    virtual float pdf(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const override;

    std::shared_ptr<class Texture> albedo;

    float exponent = 0.f;
};

Phong::Phong(const json &j /*= json::object())*/)
{
    albedo = DartsFactory<Texture>::create(j.at("albedo"));
    exponent = j.value("exponent", exponent);
}

bool Phong::sample(const Vec3f &wi, const HitInfo &hit, ScatterRecord &srec, const Vec2f &rv, float rv1) const
{
    srec.is_specular = false;
    srec.attenuation = albedo->value(wi, hit);

    auto mirror_dir = normalize(reflect(wi, hit.sn));
    ONBf onb(mirror_dir);

    auto dir_hem_cosine_pow = onb.to_world(sample_hemisphere_cosine_power(exponent, rv));
    srec.wo = dir_hem_cosine_pow;

    return dot(dir_hem_cosine_pow, hit.sn) > 0;
}

Color3f Phong::eval(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const
{
    return albedo->value(wi, hit) * pdf(wi, scattered, hit);
}

float Phong::pdf(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const
{
    auto mirror_dir = normalize(reflect(wi, hit.sn));
    auto cosine = std::max(dot(normalize(scattered), mirror_dir), 0.f);
    auto constant = (exponent + 1) / (2 * M_PI);

    return constant * powf(cosine, exponent);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Material, Phong, "phong")