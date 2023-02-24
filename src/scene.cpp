/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/scene.h>
#include <darts/progress.h>
#include <darts/stats.h>
#include <fstream>
#include <spdlog/sinks/stdout_sinks.h>

#include <nanothread/nanothread.h>
#include <map>

namespace dr = drjit;

#define USE_NANOTHREAD_RAY_TRACING 1

STAT_RATIO("Integrator/Number of NaN pixel samples", num_NaN_samples, num_pixel_samples);

uint32_t Scene::random_seed = 53;

/// Construct a new scene from a json object
Scene::Scene(const json &j)
{
    parse(j);
}

Scene::~Scene()
{
}

Color3f Scene::background(const Ray3f &ray) const
{
    return m_background;
}

bool Scene::intersect(const Ray3f &ray, HitInfo &hit) const
{
    ++g_num_traced_rays;
    return m_surfaces->intersect(ray, hit);
}

// compute the color corresponding to a ray by raytracing
Color3f Scene::recursive_color(const Ray3f &ray, int depth) const
{
    constexpr int max_depth = 64;

    HitInfo hit;
    if (intersect(ray, hit))
    {
        Color3f attenuation;
        Ray3f scattered;
        Color3f emitted_color = hit.mat->emitted(ray, hit);
        if (depth < max_depth && hit.mat->scatter(ray, hit, attenuation, scattered))
        {
            auto rec_color = recursive_color(scattered, depth + 1);
            return emitted_color + attenuation * rec_color;
        }
        else
        {
            return emitted_color;
        }
    }
    else
    {
        return background(ray);
    }
    // TODO: Recursively raytrace the scene, similar to the code you wrote in darts_tutorial1
    //       Different to before, you should also take into account surfaces that are self-emitting
    // Pseudo-code:
    //
    // if scene.intersect:
    //      get emitted color (hint: you can use hit.mat->emitted)
    // 		if depth < max_depth and hit_material.scatter(....) is successful:
    //			recursive_color = call this function recursively with the scattered ray and increased depth
    //          return emitted color + attenuation * recursive_color
    //		else
    //			return emitted color;
    // else:
    // 		return background color (hint: look at background())
}

namespace
{
    const int32_t RAY_TRACE_BLOCK_SIZE = 32;
}

// raytrace an image
Image3f Scene::raytrace() const
{
    std::map<uint32_t, std::unique_ptr<Sampler>> thread_samplers;

    // allocate an image of the proper size
    auto image = Image3f(m_camera->resolution().x, m_camera->resolution().y);

    Progress progress("Rendering", image.length());

    // Generate a ray for each pixel in the ray image
#if USE_NANOTHREAD_RAY_TRACING
    dr::parallel_for(
        dr::blocked_range<uint32_t>(/* begin = */ 0, /* end = */ image.width() * image.height(), /* block_size = */ RAY_TRACE_BLOCK_SIZE), 
        [&, this, width = image.width(), height = image.height()](dr::blocked_range<uint32_t> brange)
        {
            auto& sampler = thread_samplers[pool_thread_id()];
            if (!sampler)
            {
                sampler = m_sampler->clone();
            }
            for (uint32_t r = brange.begin(); r != brange.end(); ++r)
            {
                auto y = r / width;
                auto x = r % width;

                Color3f sum_color = Color3f(0.f);
                for (auto i : range(sampler->sample_count()))
                {
                    auto ray = m_camera->generate_ray(Vec2f(x + 0.5f + randf(), y + 0.5f + randf()));
                    if (m_integrator)
                    {
                        sum_color += m_integrator->Li(*this, *sampler.get(), ray);
                    }
                    else
                    {
                        sum_color += recursive_color(ray, 0);
                    }
                }

                image(x, y) = sum_color / m_num_samples;
                ++progress;
            }
        }
    );
#else
    for (auto y : range(image.height()))
    {
        for (auto x : range(image.width()))
        {
            Color3f sum_color = Color3f(0.f);
            for (auto i : range(m_sampler->sampler_count()))
            {
                auto ray = m_camera->generate_ray(Vec2f(x + 0.5f + randf(), y + 0.5f + randf()));
                if (m_integrator)
                {
                    sum_color += m_integrator->Li(*this, *m_sampler.get(), ray);
                }
                else
                {
                    sum_color += recursive_color(ray, 0);
                }
            }

            image(x, y) = sum_color / m_num_samples;
            ++progress;
        }
    }
#endif

    // TODO: Render the image, similar to the tutorial
    // Pseudo-code:
    //
    // foreach image row (go over image height)
    //     foreach pixel in the row (go over image width)
    //         init accumulated color to zero
    //         repeat m_num_samples times:
    //             compute a random point within the pixel (you can just add a random number between 0 and 1
    //                                                      to the pixel coordinate. You can use randf() for
    //                                                      this)
    //             compute camera ray
    //             accumulate color raytraced with the ray (by calling recursive_color)
    //         divide color by the number of pixel samples

    // Hint: you can create a Progress object (progress.h) to provide a
    // progress bar during rendering.

    // the code below finalizes and prints out the statistics gathered during rendering
    accumulate_thread_stats();
    spdlog::info(stats_report());
    clear_stats();

    // return the ray-traced image
    return image;
}

