#include <darts/common.h>
#include <darts/sampling.h>
#include <darts/factory.h>
#include <darts/sampler.h>

int main(int argc, char **argv)
{
    fmt::print("x,y,z\n");
    //fmt::print("0,0,0\n");
    shared_ptr<Sampler> sampler = DartsFactory<Sampler>::create({{"type", "cmj"}, {"samples", 400}});
    sampler->seed(2, 2); 
    sampler->start_pixel(0, 0);
    for (int i = 0; i < (int)sampler->sample_count(); ++i)
    {
        //Vec3f p = Vec3f(sample_disk(sampler->next2f()), 0.f);
        Vec3f p = sample_hemisphere(sampler->next2f());
        fmt::print("{},{},{}\n", p.x, p.y, p.z);

        sampler->advance();
    }

    return 0;
}
