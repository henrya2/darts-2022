#include <darts/common.h>
#include <darts/sampling.h>

int main(int argc, char **argv)
{
    fmt::print("x,y,z\n");
    //fmt::print("0,0,0\n");
    pcg32          rng;
    for (int i = 0; i < 500; ++i)
    {
        Vec3f p = sample_hemisphere_cosine_power(20, Vec2f{rng.nextFloat(), rng.nextFloat()});
        fmt::print("{},{},{}\n", p.x, p.y, p.z);
    }

    return 0;
}
