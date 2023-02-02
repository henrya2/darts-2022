/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/factory.h>
#include <darts/texture.h>
#include <darts/scene.h>
#include <darts/json.h>
#include <darts/image.h>
#include <filesystem/resolver.h>

// CheckerTexture
class ImageTexture : public Texture
{
public:
    ImageTexture(const json &j = json::object());

    virtual Color3f value(const Vec3f &wi, const HitInfo &hit) const override;

protected:
    Image3f image;
};

ImageTexture::ImageTexture(const json& j)
    : Texture(j)
{
    std::string filename;
    filename = j.value("filename", filename);

    std::string path = get_file_resolver().resolve(filename).str();

    if (!image.load(path))
    {
        spdlog::error("Load image '{}' failed!!!", filename);
    }
}

Color3f ImageTexture::value(const Vec3f &wi, const HitInfo &hit) const
{
    auto uv = clamp(hit.uv, {0.f, 0.f}, {1.f, 1.f});
    //uv.y = 1 - uv.y;

    //auto img_xy = (uv - Vec2f(.5f / image.width(), .5 / image.height())) * Vec2f(image.width(), image.height());
    auto img_xy = uv * Vec2f(image.width(), image.height()) - Vec2f(0.5f, 0.5f);

    return image.at((int)img_xy.x, (int)img_xy.y);
}

DARTS_REGISTER_CLASS_IN_FACTORY(Texture, ImageTexture, "image")