#include <darts/factory.h>
#include <darts/integrator.h>
#include <darts/scene.h>
#include <darts/ray.h>
#include <darts/json.h>

class NormalsIntegrator : public Integrator
{
public:
    NormalsIntegrator(const json &j);
    virtual Color3f Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const override;
};

NormalsIntegrator::NormalsIntegrator(const json& j)
{

}

Color3f NormalsIntegrator::Li(const Scene &scene, Sampler &sampler, const Ray3f &ray) const
{
    HitInfo hitinfo;
    if (scene.intersect(ray, hitinfo))
    {
        auto ret_vec = (hitinfo.sn + Vec3f(1, 1, 1)) / 2.f;
        return Color3f(ret_vec);
    }
    else
    {
        return Color3f(0, 0, 0);
    }
}

DARTS_REGISTER_CLASS_IN_FACTORY(Integrator, NormalsIntegrator, "normals")