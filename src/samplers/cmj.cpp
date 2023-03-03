/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/sampler.h>
#include <darts/sampling.h>

class CMJSampler : public Sampler
{
public:
    CMJSampler(const json &j)
    {
        m_sample_count = j.at("samples").get<int>();

        m_dim_1D = 0;
        m_dim_2D = 0;
    }

    /**
        Create an exact clone of the current instance

        This is useful if you want to duplicate a sampler to use in multiple threads
    */
    std::unique_ptr<Sampler> clone() const override
    {
        std::unique_ptr<CMJSampler> cloned(new CMJSampler());
        cloned->m_sample_count      = m_sample_count;
        cloned->m_base_seed         = m_base_seed;
        cloned->m_sample_count      = m_sample_count;
        cloned->m_current_sample    = m_current_sample;
        cloned->m_current_dimension = m_current_dimension;

        return std::move(cloned);
    }

    void set_base_seed(uint32_t s) override
    {
        Sampler::set_base_seed(s);

        p1D = s;

        m_rng.seed(s);
    }

    void seed(int x, int y) override
    {
        Sampler::seed(x, y);

        p2D = x * y;

        m_rng.seed(x, y);
    }

    void start_pixel(int x, int y) override
    {
        Sampler::start_pixel(x, y);

        reset_current_sample();

        m_dim_1D = 0;
        m_dim_2D = 0;
    }

    float next1f() override
    {
        float result = cmj::cmj(m_current_sample, m_sample_count, 1, (int)m_rng.nextUInt(32768)).x;
        m_dim_1D += 1;
        return result;
    }

    Vec2f next2f() override
    {
        Vec2f result = cmj::cmj(m_current_sample, m_sample_count, (int)m_rng.nextUInt(32768));
        m_dim_2D += 2;
        return result;
    }

protected:
    CMJSampler()
    {
    }

    int m_dim_1D;
    int m_dim_2D;
    int p1D;
    int p2D;

    pcg32 m_rng;
};

DARTS_REGISTER_CLASS_IN_FACTORY(Sampler, CMJSampler, "cmj")

/**
    \file
    \brief CMJSampler Sampler
*/
