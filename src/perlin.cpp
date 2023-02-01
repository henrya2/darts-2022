
#include <darts/perlin.h>
#include <darts/sampling.h>

Perlin::Perlin()
{
    for (int i = 0; i < point_count; ++i)
    {
        ranvec[i] = rand_unit_vec3f(-1.f, 1.f);
    }

    generate_perlin_perm(perm_x, point_count);
    generate_perlin_perm(perm_y, point_count);
    generate_perlin_perm(perm_z, point_count);
}

float Perlin::noise(const Vec3f& p) const
{
    auto u = p.x - std::floor(p.x);
    auto v = p.y - std::floor(p.y);
    auto w = p.z - std::floor(p.z);
    auto i = static_cast<int>(std::floor(p.x));
    auto j = static_cast<int>(std::floor(p.y));
    auto k = static_cast<int>(std::floor(p.z));
    Vec3f c[2][2][2];

    for (int di = 0; di < 2; di++)
        for (int dj = 0; dj < 2; dj++)
            for (int dk = 0; dk < 2; dk++)
                c[di][dj][dk] = ranvec[
                    perm_x[(i + di) & 255] ^ 
                    perm_y[(j + dj) & 255] ^
                    perm_z[(k + dk) & 255]];

    return perlin_interp(c, u, v, w);
}

float Perlin::turb(const Vec3f& p, int depth) const
{
    auto accum  = 0.0f;
    auto temp_p = p;
    auto weight = 1.0f;

    for (int i = 0; i < depth; i++)
    {
        accum += weight * noise(temp_p);
        weight *= 0.5f;
        temp_p *= 2;
    }

    return std::abs(accum);
}

void Perlin::generate_perlin_perm(int* perm, int n)
{
    for (int i = 0; i < n; ++i)
    {
        perm[i] = i;
    }

    permute(perm, n);
}

void Perlin::permute(int *perm, int n)
{
    for (int i = n - 1; i > 0; --i)
    {
        int target = randi(0, i);
        int tmp = perm[i];
        perm[i] = perm[target];
        perm[target] = tmp;
    }
}

float Perlin::perlin_interp(Vec3f c[2][2][2], float u, float v, float w)
{
    auto uu    = u * u * (3 - 2 * u);
    auto vv    = v * v * (3 - 2 * v);
    auto ww    = w * w * (3 - 2 * w);
    auto accum = 0.0f;

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                Vec3f weight_v(u - i, v - j, w - k);
                accum += (i * uu + (1 - i) * (1 - uu)) * 
                    (j * vv + (1 - j) * (1 - vv)) * 
                    (k * ww + (1 - k) * (1 - ww)) * dot(c[i][j][k], weight_v);
            }
        }
    }
    return accum;
}