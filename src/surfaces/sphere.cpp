/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/sphere.h>
#include <darts/stats.h>

#include <darts/spherical.h>

bool solve_quadratic(float a, float b, float c, float* t0, float* t1)
{
    double discrim = (double)b * (double)b - 4 * (double)a * (double)c;
    if (discrim < 0) 
        return false;
    double rootDiscrim = sqrt(discrim);
    double q;
    if (b < 0)
        q = -.5 * (b - rootDiscrim);
    else
        q = -.5 * (b + rootDiscrim);
    *t0 = (float)(q / a);
    *t1 = (float)(c / q);
    if (*t0 > *t1)
        std::swap(*t0, *t1);
    return true;
}

Sphere::Sphere(float radius, shared_ptr<const Material> material, const Transform &xform) :
    XformedSurfaceWithMaterial(material, xform), m_radius(radius)
{
}

Sphere::Sphere(const json &j) : XformedSurfaceWithMaterial(j)
{
    m_radius = j.value("radius", m_radius);
}

STAT_RATIO("Intersections/Sphere intersection tests per hit", num_sphere_tests, num_sphere_hits);

bool Sphere::intersect(const Ray3f &ray, HitInfo &hit) const
{
    ++g_num_total_intersection_tests;
    ++num_sphere_tests;
    // TODO: Assignment 1: Implement ray-sphere intersection

    // compute ray intersection (and ray parameter), continue if not hit
    auto tray = m_xform.inverse().ray(ray);
    Vec3f oc = tray.o;
    auto a = dot(tray.d, tray.d);
    auto b = 2.f * dot(oc, tray.d);
    auto c = dot(oc, oc) - m_radius * m_radius;
    float t0, t1;
    if (!solve_quadratic(a, b, c, &t0, &t1))
        return false;

    if (t0 > ray.maxt || t1 <= ray.mint)
        return false;
    float t_shape_hit = t0;
    if (t_shape_hit <= 0) {
        t_shape_hit = t1;
        if (t_shape_hit > ray.maxt)
            return false;
    }

    // TODO: If the ray misses the sphere, you should return false
    // TODO: If you successfully hit something, you should compute the hit point (p),
    //       hit distance (t), and normal (n) and fill in these values
    float t = t_shape_hit;
    Vec3f p = ray(t);
    /*
    Vec3f n = p - m_xform.m.w.xyz();
    // Only handle positive scale here, ignore rotation either. 
    // N = nabula(f(x, y, z)), f(x, y, z) = (x-c_x)^2/a^2 + (y-c_y)^2/b^2 + (z-c_z)^2/c^ - 1 while a = scale_x, b = scale_y, c = scale_z
    float x_a = m_xform.m.x.x;
    float y_a = m_xform.m.y.y;
    float z_a = m_xform.m.z.z;
    n.x /= x_a * x_a;
    n.y /= y_a * y_a;
    n.z /= z_a * z_a;
    n = normalize(n);
    */
    Vec3f spherical_pos = tray(t);
    Vec3f n = m_xform.normal(spherical_pos);

    Vec2f phi_theta = Spherical::direction_to_spherical_coordinates(spherical_pos);

    // For this assignment you can leave these two values as is
    Vec3f shading_normal = n;
    Vec2f uv             = Vec2f(phi_theta.x / (2 * M_PI), phi_theta.y / M_PI);

    // You should only assign hit and return true if you successfully hit something
    hit.t   = t;
    hit.p   = p;
    hit.gn  = n;
    hit.sn  = shading_normal;
    hit.uv  = uv;
    hit.mat = m_material.get();

    ++num_sphere_hits;
    return true;
}

Box3f Sphere::local_bounds() const
{
    return Box3f{Vec3f{-m_radius}, Vec3f{m_radius}};
}


DARTS_REGISTER_CLASS_IN_FACTORY(Surface, Sphere, "sphere")

/**
    \file
    \brief Sphere Surface
*/
