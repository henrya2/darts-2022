#include <darts/factory.h>
#include <darts/integrator.h>
#include <darts/scene.h>
#include <darts/ray.h>
#include <darts/json.h>

class PathTracerMixture : public Integrator
{
public:
    PathTracerMixture(const json &j);
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const override;

protected:
    Color3f ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const;

    int max_bounces = 1;
};

PathTracerMixture::PathTracerMixture(const json& j)
{
    max_bounces = j.value("max bounces", max_bounces);
}

Color3f PathTracerMixture::ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const
{
    const float EPSILON = 0.0000001;

    HitInfo hit;
    if (scene.intersect(ray, hit))
    {
        Color3f emitted_color = hit.mat->emitted(ray, hit);
        ScatterRecord srec;
        Vec2f rv2 = sampler.next2f();
        float rv = sampler.next1f();
        if (depth < max_bounces && !hit.mat->is_emissive())
        {
            bool material_sample_success = hit.mat->sample(ray.d, hit, srec, rv2, rv);
            const auto& emitters = scene.emiiters();

            EmitterRecord erec;
            erec.o = hit.p;

            emitters.sample(erec, rv2, rv);

            bool mat_sample = true;
            bool li_sample = true;
            if (dot(erec.wi, hit.sn) < 0)
            {
                //li_sample = false;
            }
            if (!material_sample_success)
            {
                mat_sample = false;
            }

            Color3f calc_color(0, 0, 0);
            if (srec.is_specular)
            {
                calc_color = srec.attenuation;
            }
            else
            {
                if (mat_sample && li_sample)
                {
                    auto mat_color = hit.mat->eval(ray.d, srec.wo, hit) / hit.mat->pdf(ray.d, srec.wo, hit);
                    mat_color *= ComputeColor(scene, sampler, Ray3f(hit.p, srec.wo), depth + 1);

                    auto li_color = hit.mat->eval(ray.d, erec.wi, hit) / erec.pdf;
                    li_color *= ComputeColor(scene, sampler, Ray3f(hit.p, erec.wi), depth + 1);

                    calc_color = (mat_color + li_color) / 2.f;
                }
                else if (mat_sample)
                {
                    auto mat_color = hit.mat->eval(ray.d, srec.wo, hit) / hit.mat->pdf(ray.d, srec.wo, hit);
                    mat_color *= ComputeColor(scene, sampler, Ray3f(hit.p, srec.wo), depth + 1);

                    calc_color = mat_color;
                }
                else if (li_sample)
                {
                    auto li_color = hit.mat->eval(ray.d, erec.wi, hit) / erec.pdf;
                    li_color *= ComputeColor(scene, sampler, Ray3f(hit.p, erec.wi), depth + 1);

                    calc_color = li_color;
                }
                else
                {
                    return emitted_color;
                }
            }

            return emitted_color + calc_color;
        }
        else
        {
            return emitted_color;
        }
    }
    else
    {
        return scene.background(ray);
    }    
}

Color3f PathTracerMixture::Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
{
    return ComputeColor(scene, sampler, ray, 0);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Integrator, PathTracerMixture, "path tracer mixture")