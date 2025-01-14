/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/
#pragma once

#include <darts/common.h>
#include <darts/spherical.h>
#include <pcg32.h>

/** \addtogroup Random
    @{

    The Random module provides a random number generator suitable for ray tracing (via Wenzel Jakob's tiny
    [pcg32](https://github.com/wjakob/pcg32) library), and several functions to generate points and directions useful in
    path tracing and procedural generation.
*/

/** \name Global RNG and rejection sampling
    @{
*/

/// Global random number generator that produces floats between <tt>[0,1)</tt>
inline float randf()
{
    static thread_local pcg32 rng = pcg32();
    return rng.nextFloat();
}

inline float randf(float min , float max)
{
    return min + (max - min) * randf();
}

// Returns a random integer in [min, max]
inline int randi(int min, int max)
{
    return static_cast<int>(randf(min, max + 1));
}

inline Vec3f rand_vec3f(float min , float max)
{
    return Vec3f(randf(min, max), randf(min, max), randf(min, max));
}

inline Vec3f rand_unit_vec3f(float min, float max)
{
    return normalize(rand_vec3f(min, max));
}

/// Sample a random point uniformly within a unit sphere (uses the global randf() RNG and rejection sampling)
inline Vec3f random_in_unit_sphere()
{
    Vec3f p;
    do
    {
        float a = randf();
        float b = randf();
        float c = randf();
        p       = 2.0f * Vec3f(a, b, c) - Vec3f(1);
    } while (length2(p) >= 1.0f);

    return p;
}

/// Sample a random point uniformly within a unit disk (uses the global randf() RNG and rejection sampling)
inline Vec2f random_in_unit_disk()
{
    Vec2f p;
    do
    {
        float a = randf();
        float b = randf();
        p       = 2.0f * Vec2f(a, b) - Vec2f(1);
    } while (length2(p) >= 1.0f);

    return p;
}

/// Hash two integer coordinates (e.g. pixel coordinates) into a pseudo-random unsigned int
inline uint32_t hash2d(int x, int y)
{
    // hash x,y coordinate into a single an unsigned seed
    uint32_t px  = 1103515245u * ((x >> 1u) ^ y);
    uint32_t py  = 1103515245u * ((y >> 1u) ^ x);
    uint32_t h32 = 1103515245u * ((px) ^ (py >> 3u));
    return h32 ^ (h32 >> 16);
}

/** @}*/


/** \name Sampling a disk
    @{
*/

/// Uniformly sample a vector on a 2D disk with radius 1, centered around the origin
inline Vec2f sample_disk(const Vec2f &rv)
{
    float r                 = std::sqrt(rv.y);
    auto [sin_phi, cos_phi] = Spherical::sincos(2.0f * M_PI * rv.x);

    return Vec2f(cos_phi * r, sin_phi * r);
}


/// Probability density of #sample_disk()
inline float sample_disk_pdf(const Vec2f &p)
{
    return length2(p) <= 1 ? INV_PI : 0.0f;
}

/** @}*/



/** \name Sampling a sphere or a ball
    @{
*/

/// Uniformly sample a vector on the unit sphere with respect to solid angles
inline Vec3f sample_sphere(const Vec2f &rv)
{
    auto phi = 2 * M_PI * rv.x;
    auto cos_theta = rv.y;
    auto sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    auto [sin_phi, cos_phi] = Spherical::sincos(phi);
    return Vec3f{cos_phi * sin_theta, sin_phi * sin_theta, cos_theta}; // CHANGEME
}

/// Probability density of #sample_sphere()
inline float sample_sphere_pdf()
{
    return 1.f / (M_PI * 4); // CHANGEME
}


/** \name Sampling the hemisphere
    @{
*/

/// Uniformly sample a vector on the unit hemisphere around the pole (0,0,1) with respect to solid angles
inline Vec3f sample_hemisphere(const Vec2f &rv)
{
    auto phi = 2 * M_PI * rv.x;
    auto cos_theta = rv.y;
    auto sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    auto [sin_phi, cos_phi] = Spherical::sincos(phi);
    return Vec3f{cos_phi * sin_theta, sin_phi * sin_theta, cos_theta}; // CHANGEME
}

/// Probability density of #sample_hemisphere()
inline float sample_hemisphere_pdf(const Vec3f &v)
{
    return 1.f / (M_PI * 2); // CHANGEME
}

/// Uniformly sample a vector on the unit hemisphere around the pole (0,0,1) with respect to projected solid
/// angles
inline Vec3f sample_hemisphere_cosine(const Vec2f &rv)
{
    auto phi = 2 * M_PI * rv.x;
    auto cos_theta = std::sqrt(rv.y);
    auto sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    auto [sin_phi, cos_phi] = Spherical::sincos(phi);
    return Vec3f{cos_phi * sin_theta, sin_phi * sin_theta, cos_theta}; // CHANGEME
}

/// Probability density of #sample_hemisphere_cosine()
inline float sample_hemisphere_cosine_pdf(const Vec3f &v)
{
    return v.z / (M_PI); // CHANGEME
}

/// Sample a vector on the unit hemisphere with a cosine-power density about the pole (0,0,1)
inline Vec3f sample_hemisphere_cosine_power(float exponent, const Vec2f &rv)
{
    auto phi = 2 * M_PI * rv.x;
    auto cos_theta = std::pow(rv.y, 1.f / (exponent + 1));
    auto sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    auto [sin_phi, cos_phi] = Spherical::sincos(phi);
    return Vec3f{cos_phi * sin_theta, sin_phi * sin_theta, cos_theta}; // CHANGEME
}

/// Probability density of #sample_hemisphere_cosine_power()
inline float sample_hemisphere_cosine_power_pdf(float exponent, float cosine)
{
    return  std::pow(cosine, 1.f / (exponent + 1)) / (M_PI); // CHANGEME
}

/** @}*/



/** \name Sampling a spherical cap
    @{
*/

/**
    Uniformly sample a vector on a spherical cap around (0, 0, 1)

    A spherical cap is the subset of a unit sphere whose directions make an angle of less than 'theta' with the north
    pole. This function expects the cosine of 'theta' as a parameter.
 */
inline Vec3f sample_sphere_cap(const Vec2f &rv, float cos_theta_max)
{
    auto phi = 2 * M_PI * rv.x;
    auto cos_theta = lerp(cos_theta_max, 1.f, rv.y);
    auto sin_theta = std::sqrt(1 - cos_theta * cos_theta);
    auto [sin_phi, cos_phi] = Spherical::sincos(phi);
    return Vec3f{cos_phi * sin_theta, sin_phi * sin_theta, cos_theta}; // CHANGEME
}

/// Probability density of #sample_sphere_cap()
inline float sample_sphere_cap_pdf(float cos_theta, float cos_theta_max)
{
    return 1 / (2 * M_PI * (1 - cos_theta_max)); // CHANGEME
}

/** @}*/

/** \name Sampling a triangle
    @{
*/

/**
    Sample a point uniformly on a triangle with vertices `v0`, `v1`, `v2`.

    \param v0,v1,v2 The vertices of the triangle to sample
    \param rv       Two random variables uniformly distributed in [0,1)
*/
inline Vec3f sample_triangle(const Vec3f &v0, const Vec3f &v1, const Vec3f &v2, const Vec2f &rv)
{
    float alpha = rv.x;
    float beta = rv.y;
    if (alpha + beta > 1)
    {
        alpha = 1 - alpha;
        beta = 1 - beta;
    }
    float gamma = 1 - alpha - beta;

    return alpha * v0 + beta * v1 + gamma * v2;
}

/// Sampling density of #sample_triangle()
inline float sample_triangle_pdf(const Vec3f &v0, const Vec3f &v1, const Vec3f &v2)
{
    auto cross_vec = la::cross(v1 - v0, v2 - v0);
    return 2 / la::length(cross_vec); // 1 / (1/2 * norm(cross_vec))
}

/** @}*/

namespace cmj
{
    inline float randfloat(uint32_t i, uint32_t p)
    {
        i ^= p;
        i ^= i >> 17;   i *= 0xb36534e5;
        i ^= i >> 12;
        i ^= i >> 21;   i *= 0x93fc4795;
        i ^= 0xdf6e307f;
        i ^= i >> 17;   i *= 1 | p >> 18;
        return i * (1.0f / 4294967808.0f);
    }

    inline uint32_t permute(uint32_t i, uint32_t l, uint32_t p)
    {
        uint32_t w = l - 1;
        w |= w >> 1;
        w |= w >> 2;
        w |= w >> 4;
        w |= w >> 8;
        w |= w >> 16;
        
        do
        {
            i ^= p;             i *= 0xe170893d;
            i ^= p       >> 16;
            i ^= (i & w) >> 4;
            i ^= p       >> 8;  i *= 0x0929eb3f;
            i ^= p       >> 23;
            i ^= (i & w) >> 1;  i *= 1 | p >> 27;
                                i *= 0x6935fa69;
            i ^= (i & w) >> 11; i *= 0x74dcb303;
            i ^= (i & w) >> 2;  i *= 0x9e501cc3;
            i ^= (i & w) >> 2;  i *= 0xc860a3df;
            i &= w;
            i ^= i       >> 5;
        } 
        while (i >= l);
        
        return (i + p) % l;
    }

    inline Vec2f cmj(int s, int m, int n, int p)
    {
        int sx = permute(s % m, m, p * 0xa511e9b3);
        int sy = permute(s / m, n, p * 0x63d83595);
        float jx = randfloat(s, p * 0xa399d265);
        float jy = randfloat(s, p * 0x711ad6a5);
        
        Vec2f r2 = {(s % m + (sy + jx) / n) / m,
                    (s / m + (sx + jy) / m) / n};

        return r2;
    }

    inline Vec2f cmj(int s, int N, int p, float a = 1.0f)
    {
        int m = static_cast<int>(sqrtf(N * a));
        int n = (N + m - 1) / m;
        s = permute(s, N, p * 0x51633e2d);
        int sx = permute(s % m, m, p * 0x68bc21eb);
        int sy = permute(s / m, n, p * 0x02e5be93);
        float jx = randfloat(s, p * 0x967a889b);
        float jy = randfloat(s, p * 0x368cc8b7);
        Vec2f r2 = {(sx + (sy + jx) / n) / m,  
                    (s + jy) / N};

        return r2;
    }
}


/** \name Sampling tabulated 1D and 2D distributions
    @{
*/

/*
    The Distribution1D and Distribution2D classes below are adapted from the PBRTv3 source code, which under a
    BSD-2-Clause license. Copyright(c) 1998-2016 Matt Pharr, Greg Humphreys, and Wenzel Jakob.
*/

/**
    A tabulated 1D probability distribution (either continuous or discrete).

    This data structure can be used to transform uniformly distributed
    samples to a stored 1D probability distribution.
*/
struct Distribution1D
{
    /// Construct a 1D distribution from an array of \p n floats starting at \p f
    Distribution1D(const float *f, int n) : func(f, f + n), cdf(n + 1)
    {
        // Compute integral of step function at x_i
        cdf[0] = 0;
        for (int i = 1; i < n + 1; ++i)
            cdf[i] = cdf[i - 1] + func[i - 1] / n;

        // Transform step function integral into CDF
        func_int = cdf[n];
        if (func_int == 0)
        {
            for (int i = 1; i < n + 1; ++i)
                cdf[i] = float(i) / float(n);
        }
        else
        {
            for (int i = 1; i < n + 1; ++i)
                cdf[i] /= func_int;
        }
    }

    /// Number of elements in the distribution
    int count() const
    {
        return (int)func.size();
    }

    /**
        Sample from a piecewise-constant tabulated 1D distribution.

        \param [in] u      Uniform random number in [0,1)
        \param [out] pdf   If not null, stores the pdf of the sample
        \param [out] off   If not null, stores the
        \return            The sample.
    */
    float sample_continuous(float u, float *pdf, int *off = nullptr) const
    {
        // Find surrounding CDF segments and offset
        int offset = find_interval(u);

        if (off)
            *off = offset;

        // Compute offset along CDF segment
        float du = u - cdf[offset];
        if ((cdf[offset + 1] - cdf[offset]) > 0)
            du /= (cdf[offset + 1] - cdf[offset]);

        // Compute PDF for sampled offset
        if (pdf)
            *pdf = (func_int > 0) ? func[offset] / func_int : 0;

        // Return x in [0,1) corresponding to sample
        return (offset + du) / count();
    }

    /**
        Sample from a discrete 1D distribution.

        \param [in] u           Uniform random number in [0,1)
        \param [out] pmf        If not null, stores the probability mass function of the sample
        \param [out] u_remapped If not null, stores a remapped version of u for reuse
        \return                 The sample.
    */
    int sample_discrete(float u, float *pmf = nullptr, float *u_remapped = nullptr) const
    {
        // Find surrounding CDF segments and _offset_
        int offset = find_interval(u);
        if (pmf)
            *pmf = (func_int > 0) ? func[offset] / (func_int * count()) : 0;
        if (u_remapped)
            *u_remapped = (u - cdf[offset]) / (cdf[offset + 1] - cdf[offset]);
        return offset;
    }
    /// The discrete PDF value
    float discrete_PDF(int index) const
    {
        return func[index] / (func_int * count());
    }

    // Distribution1D Public Data
    std::vector<float> func, cdf;
    float              func_int;

private:
    int find_interval(float u) const
    {
        std::vector<float>::const_iterator entry = std::lower_bound(cdf.begin(), cdf.end(), u);
        int                                index = (int)std::max((ptrdiff_t)0, entry - cdf.begin() - 1);
        return std::min(index, int(cdf.size()) - 2);
    }
};

/// Allows sampling from a piecewise-constant 2D distribution.
class Distribution2D
{
public:
    // Distribution2D Public Methods
    Distribution2D(const float *func, int nu, int nv)
    {
        p_conditional.reserve(nv);
        for (int v = 0; v < nv; ++v)
        {
            // Compute conditional sampling distribution for vs
            p_conditional.emplace_back(new Distribution1D(&func[v * nu], nu));
        }
        // Compute marginal sampling distribution p[v]
        std::vector<float> marginal_func;
        marginal_func.reserve(nv);
        for (int v = 0; v < nv; ++v)
            marginal_func.push_back(p_conditional[v]->func_int);
        p_marginal.reset(new Distribution1D(&marginal_func[0], nv));
    }
    Vec2f sample_continuous(const Vec2f &u, float *pdf) const
    {
        float pdfs[2];
        int   v;
        float d1 = p_marginal->sample_continuous(u[1], &pdfs[1], &v);
        float d0 = p_conditional[v]->sample_continuous(u[0], &pdfs[0]);
        *pdf     = pdfs[0] * pdfs[1];
        return Vec2f(d0, d1);
    }
    float pdf(const Vec2f &p) const
    {
        int iu = clamp(int(p[0] * p_conditional[0]->count()), 0, p_conditional[0]->count() - 1);
        int iv = clamp(int(p[1] * p_marginal->count()), 0, p_marginal->count() - 1);
        return p_conditional[iv]->func[iu] / p_marginal->func_int;
    }

private:
    // Distribution2D Private Data
    std::vector<std::unique_ptr<Distribution1D>> p_conditional;
    std::unique_ptr<Distribution1D>              p_marginal;
};

/** @}*/


/** @}*/

/**
    \file
    \brief Random sampling on various domains
*/