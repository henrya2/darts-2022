#include <darts/factory.h>
#include <darts/integrator.h>
#include <darts/scene.h>
#include <darts/ray.h>
#include <darts/json.h>

class AmbientOcclusionIntegrator : public Integrator
{
public:
    AmbientOcclusionIntegrator(const json &j);
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const override;
};

AmbientOcclusionIntegrator::AmbientOcclusionIntegrator(const json& j)
{

}

Color3f AmbientOcclusionIntegrator::Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
{
    bool result = false;
    HitInfo hitinfo;
    if (scene.intersect(ray, hitinfo))
    {
        ScatterRecord srec;
        if (hitinfo.mat->sample(ray.d, hitinfo, srec, sampler.next2f(), sampler.next1f()))
        {
            Ray3f shadowray(hitinfo.p, srec.wo);
            HitInfo shadowhit;
            if (!scene.intersect(shadowray, shadowhit))
            {
                return Color3f(1.f, 1.f, 1.f);
            }
        }
    }

    return Color3f(0.f, 0.f, 0.f);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Integrator, AmbientOcclusionIntegrator, "ao")