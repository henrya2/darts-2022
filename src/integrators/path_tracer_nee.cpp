#include <darts/factory.h>
#include <darts/integrator.h>
#include <darts/scene.h>
#include <darts/ray.h>
#include <darts/json.h>

class PathTracerNEE : public Integrator
{
public:
    PathTracerNEE(const json &j);
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const override;

protected:
    Color3f ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const;

    int max_bounces = 1;
};

PathTracerNEE::PathTracerNEE(const json& j)
{
    max_bounces = j.value("max bounces", max_bounces);
}

Color3f PathTracerNEE::ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const
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
            hit.mat->sample(ray.d, hit, srec, rv2, rv);
            const auto& emitters = scene.emiiters();

            EmitterRecord erec;
            erec.o = hit.p;

            emitters.sample(erec, rv2, rv);
            
            if (dot(erec.wi, hit.sn) < 0)
                return emitted_color;

            Color3f calc_color(0, 0, 0);
            if (srec.is_specular)
            {
                calc_color = srec.attenuation;
            }
            else
            {
                float pdf = erec.pdf;
                if (pdf > 0)
                {
                    calc_color = hit.mat->eval(ray.d, erec.wi, hit) / pdf;
                }
            }

            return emitted_color + calc_color * ComputeColor(scene, sampler, Ray3f(erec.o, erec.wi), depth + 1);
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

Color3f PathTracerNEE::Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
{
    return ComputeColor(scene, sampler, ray, 0);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Integrator, PathTracerNEE, "path tracer nee")