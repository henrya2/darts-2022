#pragma once

#include <darts/common.h>

class Perlin
{
public:
    Perlin();

    float noise(const Vec3f& p) const;

    float turb(const Vec3f& p, int depth = 7) const;

private:
    static void generate_perlin_perm(int* perm, int n);
    static void permute(int* perm, int n);

    static float perlin_interp(Vec3f c[2][2][2], float u, float v, float w);

private:
    static const int point_count = 256;

    int perm_x[point_count];
    int perm_y[point_count];
    int perm_z[point_count]; 

    Vec3f ranvec[point_count];
};