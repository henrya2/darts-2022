#include <darts/factory.h>
#include <darts/integrator.h>
#include <darts/scene.h>
#include <darts/ray.h>
#include <darts/json.h>

class PathTracerMats : public Integrator
{
public:
    PathTracerMats(const json &j);
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const override;

protected:
    Color3f ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const;

    int max_bounces = 1;
};

PathTracerMats::PathTracerMats(const json& j)
{
    max_bounces = j.value("max bounces", max_bounces);
}

Color3f PathTracerMats::ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const
{
    HitInfo hit;
    if (scene.intersect(ray, hit))
    {
        Color3f emitted_color = hit.mat->emitted(ray, hit);
        ScatterRecord srec;
        if (depth < max_bounces && hit.mat->sample(ray.d, hit, srec, sampler.next2f(), sampler.next1f()))
        {
            Ray3f scattered(hit.p, srec.wo);
            auto rec_color = ComputeColor(scene, sampler, scattered, depth + 1);

            Color3f calc_color(0, 0, 0);
            if (srec.is_specular)
            {
                calc_color = srec.attenuation;
            }
            else
            {
                float pdf = hit.mat->pdf(ray.d, srec.wo, hit);
                if (pdf > 0)
                {
                    calc_color = hit.mat->eval(ray.d, srec.wo, hit) / pdf;
                }
            }
            return emitted_color + calc_color * rec_color;
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

Color3f PathTracerMats::Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
{
    return ComputeColor(scene, sampler, ray, 0);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Integrator, PathTracerMats, "path tracer mats")