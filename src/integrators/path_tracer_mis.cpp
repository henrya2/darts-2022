#include <darts/factory.h>
#include <darts/integrator.h>
#include <darts/scene.h>
#include <darts/ray.h>
#include <darts/json.h>

class PathTracerMIS : public Integrator
{
public:
    PathTracerMIS(const json &j);
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const override;

protected:
    Color3f ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const;

    int max_bounces = 1;
};

PathTracerMIS::PathTracerMIS(const json& j)
{
    max_bounces = j.value("max bounces", max_bounces);
}

Color3f PathTracerMIS::ComputeColor(const Scene &scene, Sampler &sampler, const Ray3f &ray, int depth) const
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

            bool picked_mat = false;
            if (randf() <= 0.5)
            {
                picked_mat = true;
            }
            
            if (!picked_mat)
            {
                if (dot(erec.wi, hit.sn) < 0)
                    return emitted_color;
            }
            else
            {
                if (!material_sample_success)
                    return emitted_color;
            }

            Vec3f scatter_o = hit.p;
            Vec3f scatter_d;

            if (picked_mat)
            {
                scatter_d = srec.wo;
            }
            else
            {
                scatter_d = erec.wi;
            }

            float pdf = (erec.emitter->pdf(scatter_o, erec.wi) + hit.mat->pdf(ray.d, srec.wo, hit)) / 2.f;            

            Color3f calc_color(0, 0, 0);
            if (srec.is_specular)
            {
                calc_color = srec.attenuation;
            }
            else
            {
                if (pdf > 0)
                {
                    calc_color = hit.mat->eval(ray.d, scatter_d, hit) / pdf;
                }
                else
                {
                    return emitted_color;
                }
            }

            return emitted_color + calc_color * ComputeColor(scene, sampler, Ray3f(scatter_o, scatter_d), depth + 1);
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

Color3f PathTracerMIS::Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
{
    return ComputeColor(scene, sampler, ray, 0);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Integrator, PathTracerMIS, "path tracer mis")