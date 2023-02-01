/*
    This file is part of darts – the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2022 by Wojciech Jarosz
*/

#include <darts/texture.h>

Texture::Texture(const json &j)
{
    if (j.is_object())
    {
        xform = j.value("xform", xform);
    }
}
