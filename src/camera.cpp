/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/
#include <darts/camera.h>
#include <darts/stats.h>

STAT_COUNTER("Integrator/Camera rays traced", num_camera_rays);

Camera::Camera(const json &j)
{
    m_xform           = j.value("transform", m_xform);
    m_resolution      = j.value("resolution", m_resolution);
    m_focal_distance  = j.value("fdist", m_focal_distance);
    m_aperture_radius = j.value("aperture", m_aperture_radius);

    float vfov = j.value("vfov", 90.f); // Default vfov value. Override this with the value from json
    // TODO: Assignment 1: read the vertical field-of-view from j ("vfov"),
    // and compute the width and height of the image plane. Remember that
    // the "vfov" parameter is specified in degrees, but C++ math functions
    // expect it in radians. You can use deg2rad() from common.h to convert
    // from one to the other
    float aspect_ratio = ((float) m_resolution.x) / m_resolution.y;
    float theta = Spherical::deg2rad(vfov);
    float h = tan(theta / 2.f);
    float viewport_height = 2.f * h;
    float viewport_width = viewport_height * aspect_ratio;
    m_size = Vec2f(viewport_width, viewport_height);
}

Ray3f Camera::generate_ray(const Vec2f &pixel) const
{
    ++num_camera_rays;
    // TODO: Assignment 1: Implement camera ray generation
    Vec2f d = (2.f *
            pixel / m_resolution) - 1.f;

    Vec2f rd = m_aperture_radius * random_in_unit_disk();
    Vec3f offset(rd.x, rd.y, 0);

    Vec3f origin(0, 0, 0);
    Vec3f dir;
    dir.x = d.x * (m_size.x / 2.f);
    dir.y = -d.y * (m_size.y / 2.f);
    dir.z = -1.f;
    return m_xform.ray(Ray3f(origin + offset, dir * m_focal_distance - offset));
}