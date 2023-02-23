#include <darts/factory.h>
#include <darts/material.h>
#include <darts/scene.h>
#include <darts/texture.h>
#include <darts/onb.h>

class BlinnPhong : public Material
{
public:
    BlinnPhong(const json &j = json::object());

    virtual bool sample(const Vec3f &wi, const HitInfo &hit, ScatterRecord &srec, const Vec2f &rv, float rv1) const override;

    virtual Color3f eval(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const override;

    virtual float pdf(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const override;

    std::shared_ptr<class Texture> albedo;

    float exponent = 0.f;
};

BlinnPhong::BlinnPhong(const json &j /*= json::object())*/)
{
    albedo = DartsFactory<Texture>::create(j.at("albedo"));
    exponent = j.value("exponent", exponent);
}

bool BlinnPhong::sample(const Vec3f &wi, const HitInfo &hit, ScatterRecord &srec, const Vec2f &rv, float rv1) const
{
    srec.is_specular = false;
    srec.attenuation = albedo->value(wi, hit);

    ONBf onb(hit.sn);
    auto new_normal = onb.to_world(sample_hemisphere_cosine_power(exponent, rv));
    auto reflect_dir = normalize(reflect(wi, new_normal));

    srec.wo = reflect_dir;

    return dot(reflect_dir, hit.sn) > 0;
}

Color3f BlinnPhong::eval(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const
{
    return albedo->value(wi, hit) * pdf(wi, scattered, hit);
}

float BlinnPhong::pdf(const Vec3f &wi, const Vec3f &scattered, const HitInfo &hit) const
{
    auto random_normal = normalize(-normalize(wi) + scattered);

    auto cosine = max(dot(random_normal, hit.sn), 0.f);
    auto normal_pdf = (exponent + 1) / (2 * M_PI) * powf(cosine, exponent);

    return normal_pdf / ( 4 * dot(-normalize(wi), random_normal));
}

DARTS_REGISTER_CLASS_IN_FACTORY(Material, BlinnPhong, "blinn-phong")