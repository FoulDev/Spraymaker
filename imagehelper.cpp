/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "imagehelper.h"
#include "spraymakerexception.h"
#include <crunch/crnlib/crn_color.h>

uint ImageHelper::getPixelArtBoxSize(const vips::VImage img)
{
    // TODO: Detect pixel art not aligned on the edges.
    // TODO: Detect pixel art with some imperfections due to encoding/small errors
    // ex. only a few blocks don't match or are close enough to consider the same

    auto getPixel = [](const uchar* ptr, uint x, uint y, uint width) {
        const uchar* pixelPtr = (uchar*)ptr + (x*4 + y*width*4);
        uint pixel = (pixelPtr[0] << 0 )
                   | (pixelPtr[1] << 8 )
                   | (pixelPtr[2] << 16)
                   | (pixelPtr[2] << 24);

        return pixel;
    };

    auto andBlock = [getPixel](const uchar* ptr, uint blockSize, uint bx, uint by, uint width) {
        uint startPixel = -1U;
        uint blockAnd   = 0;

        for(int x = bx; x < bx + blockSize; x++)
        {
            for(int y = by; y < by + blockSize; y++)
            {
                if (x == bx && y == by)
                {
                    startPixel = getPixel(ptr, x, y, width);
                    blockAnd = startPixel;
                }

                blockAnd &= getPixel(ptr, x, y, width);
            }
        }

        return startPixel == blockAnd;
    };

    // Assumption: 10x10 is the lowest pixel art size somebody would try to make
    int blockSize = std::max(1, std::min(img.width(), img.height()) / 10) + 1;
    bool matching = false;

    while(--blockSize > 1)
    {
        if(img.width() % blockSize != 0 || img.height() % blockSize != 0)
            continue;

        for(int x = 0; x < img.width(); x += blockSize)
        {
            for(int y = 0; y < img.height(); y += blockSize)
            {
                matching = andBlock((uchar*)img.data(), blockSize, x, y, img.width());
                if (matching == false) break;
            }
            if (matching == false) break;
        }
        if (matching == true) break;
    }

    return blockSize;
}

const ImageHelper::BoundingBox ImageHelper::getImageBorders(
    const void* pixels, uint width, uint height,
    PixelAlphaMode pixelAlphaMode, uint alphaThreshold)
{
    auto ptr = (const uchar*)pixels;

    auto getEffectivePixel = [](const uchar* ptr, uint x, uint y, uint width,
                                ImageHelper::PixelAlphaMode pixelAlphaMode, int alphaThreshold) {
        const uchar* pixelPtr = (uchar*)ptr + (x*4 + y*width*4);
        uint check = (pixelPtr[0] << 0 )
                   | (pixelPtr[1] << 8 )
                   | (pixelPtr[2] << 16);

        switch(pixelAlphaMode)
        {
        case ImageHelper::PixelAlphaMode::FULL:
            check |= pixelPtr[3] << 24;
            break;
        case ImageHelper::PixelAlphaMode::THRESHOLD:
            if (pixelPtr[3] < alphaThreshold)
                // Below alpha threshold, pixel will be turned off
                check  = 0;
            else
                // Above alpha thredhold, pixel will be turned on
                check |= 0xff000000;
            break;
        case ImageHelper::PixelAlphaMode::NONE:
        default:
            break;
        }
        return check;
    };

    auto leftEdge = [getEffectivePixel](const uchar* ptr, uint width, uint height,
                                        ImageHelper::PixelAlphaMode pixelAlphaMode, uint alphaThreshold){
        const uint startPixel = getEffectivePixel(ptr, 0, 0, width,
                                                  pixelAlphaMode, alphaThreshold);
        for(uint x = 0; x < width; x++)
        {
            for(uint y = 0; y < height; y++)
            {
                const uint pixel = getEffectivePixel(ptr, x, y, width,
                                                     pixelAlphaMode, alphaThreshold);
                if (pixel != startPixel)
                    return x;
            }
        }
        return 0U;
    };

    auto rightEdge = [getEffectivePixel](const uchar* ptr, uint width, uint height,
                                         ImageHelper::PixelAlphaMode pixelAlphaMode, uint alphaThreshold){
        const uint startPixel = getEffectivePixel(ptr, width - 1, 0, width,
                                                  pixelAlphaMode, alphaThreshold);
        for(uint x = 0; x < width; x++)
        {
            for(uint y = 0; y < height; y++)
            {
                const uint pixel = getEffectivePixel(ptr, width - 1 - x, y, width,
                                                     pixelAlphaMode, alphaThreshold);
                if (pixel != startPixel)
                    return width - 1 - x;
            }
        }
        return width - 1;
    };

    auto topEdge = [getEffectivePixel](const uchar* ptr, uint width, uint height,
                                       ImageHelper::PixelAlphaMode pixelAlphaMode, uint alphaThreshold){
        const uint startPixel = getEffectivePixel(ptr, 0, 0, width,
                                                  pixelAlphaMode, alphaThreshold);
        for(uint y = 0; y < height; y++)
        {
            for(uint x = 0; x < width; x++)
            {
                const uint pixel = getEffectivePixel(ptr, x, y, width,
                                                     pixelAlphaMode, alphaThreshold);
                if (pixel != startPixel)
                    return y;
            }
        }
        return 0U;
    };

    auto bottomEdge = [getEffectivePixel](const uchar* ptr, uint width, uint height,
                                          ImageHelper::PixelAlphaMode pixelAlphaMode, uint alphaThreshold){
        const uint startPixel = getEffectivePixel(ptr, 0, height - 1, width,
                                                  pixelAlphaMode, alphaThreshold);
        for(uint y = 0; y < height; y++)
        {
            for(uint x = 0; x < width; x++)
            {
                const uint pixel = getEffectivePixel(ptr, x, height - 1 - y, width,
                                                     pixelAlphaMode, alphaThreshold);
                if (pixel != startPixel)
                    return height - 1 - y;
            }
        }
        return height - 1;
    };

    auto left   = leftEdge  (ptr, width, height, pixelAlphaMode, alphaThreshold);
    auto right  = rightEdge (ptr, width, height, pixelAlphaMode, alphaThreshold);
    auto top    = topEdge   (ptr, width, height, pixelAlphaMode, alphaThreshold);
    auto bottom = bottomEdge(ptr, width, height, pixelAlphaMode, alphaThreshold);

    if (left > right)
    {
        left = 0;
        right = width - 1;
    }

    if (top > bottom)
    {
        top = 0;
        bottom = height - 1;
    }

    auto newWidth  = (left == right  ? width  : 1 + right  - left);
    auto newHeight = (top  == bottom ? height : 1 + bottom - top );

    return BoundingBox{
        .left = left,
        .right = right,
        .top = top,
        .bottom = bottom,
        .width = newWidth,
        .height = newHeight,
    };
}

uint ImageHelper::sizeOfDxtImage(uint width, uint height,
                                 uint mipmaps, uint frames, uint bytesPerBlock)
{
    uint sum = 0;

    for(uint mipmap = 0; mipmap < mipmaps; mipmap++) {
        uint mipWidth = std::max(1U, width >> mipmap);
        uint mipHeight = std::max(1U, height >> mipmap);
        uint blocks = ((mipWidth + 3U) >> 2U) * ((mipHeight + 3U) >> 2U);
        sum += blocks * bytesPerBlock;
    }

    return sum * frames;
}

uint ImageHelper::sizeOfImage(uint width, uint height,
                              uint mipmaps, uint frames, uint bytesPerPixel)
{
    uint sum = 0;

    for(uint mipmap = 0; mipmap < mipmaps; mipmap++) {
        uint mipWidth = std::max(1U, width >> mipmap);
        uint mipHeight = std::max(1U, height >> mipmap);
        sum += (mipWidth * mipHeight) * bytesPerPixel;
    }

    return sum * frames;
}

bool ImageHelper::isDxt(SpraymakerModel::ImageFormat format)
{
    switch(format)
    {
    case SpraymakerModel::ImageFormat::DXT1:
    case SpraymakerModel::ImageFormat::DXT1A:
    case SpraymakerModel::ImageFormat::DXT3:
    case SpraymakerModel::ImageFormat::DXT5:
        return true;
    default:
        return false;
    }
}

bool ImageHelper::hasOneBitAlpha(SpraymakerModel::ImageFormat format)
{
    switch(format)
    {
    case SpraymakerModel::ImageFormat::DXT1A:
    case SpraymakerModel::ImageFormat::BGR888_BLUESCREEN:
    case SpraymakerModel::ImageFormat::RGB888_BLUESCREEN:
    case SpraymakerModel::ImageFormat::BGRA5551:
    //case SpraymakerModel::ImageFormat::BGRX5551:
    case SpraymakerModel::ImageFormat::P8:
        return true;
    default:
        return false;
    }
}

bool ImageHelper::hasMultiBitAlpha(SpraymakerModel::ImageFormat format)
{
    switch(format)
    {
    case SpraymakerModel::ImageFormat::DXT3:
    case SpraymakerModel::ImageFormat::DXT5:
    case SpraymakerModel::ImageFormat::BGRA4444:
    case SpraymakerModel::ImageFormat::RGBA8888:
    case SpraymakerModel::ImageFormat::IA88:
    //case SpraymakerModel::ImageFormat::BGRX8888:
    case SpraymakerModel::ImageFormat::BGRA8888:
    case SpraymakerModel::ImageFormat::ARGB8888:
    case SpraymakerModel::ImageFormat::ABGR8888:
    case SpraymakerModel::ImageFormat::A8:
    case SpraymakerModel::ImageFormat::UVWQ8888:
    //case SpraymakerModel::ImageFormat::UVLX8888:
    case SpraymakerModel::ImageFormat::RGBA16161616:
    case SpraymakerModel::ImageFormat::RGBA16161616F:
    case SpraymakerModel::ImageFormat::RGBA32323232F:
        return true;
    default:
        return false;
    }
}

bool ImageHelper::hasAlpha(SpraymakerModel::ImageFormat format)
{
    return hasOneBitAlpha(format) || hasMultiBitAlpha(format);
}

uint ImageHelper::bytesPerBlock(SpraymakerModel::ImageFormat format)
{
    switch(format)
    {
    case SpraymakerModel::ImageFormat::DXT1:
    case SpraymakerModel::ImageFormat::DXT1A:
        return 8;
    case SpraymakerModel::ImageFormat::DXT3:
    case SpraymakerModel::ImageFormat::DXT5:
        return 16;
    default:
        return 0;
    }
}

uint ImageHelper::bytesPerPixel(SpraymakerModel::ImageFormat format)
{
    switch(format)
    {
    case SpraymakerModel::ImageFormat::A8:
    case SpraymakerModel::ImageFormat::I8:
    case SpraymakerModel::ImageFormat::P8:
        return 1;
    case SpraymakerModel::ImageFormat::BGR565:
    case SpraymakerModel::ImageFormat::BGRA4444:
    case SpraymakerModel::ImageFormat::BGRA5551:
    case SpraymakerModel::ImageFormat::BGRX5551:
    case SpraymakerModel::ImageFormat::IA88:
    case SpraymakerModel::ImageFormat::RGB565:
    case SpraymakerModel::ImageFormat::UV88:
        return 2;
    case SpraymakerModel::ImageFormat::BGR888:
    case SpraymakerModel::ImageFormat::BGR888_BLUESCREEN:
    case SpraymakerModel::ImageFormat::RGB888:
    case SpraymakerModel::ImageFormat::RGB888_BLUESCREEN:
        return 3;
    case SpraymakerModel::ImageFormat::ABGR8888:
    case SpraymakerModel::ImageFormat::ARGB8888:
    case SpraymakerModel::ImageFormat::BGRA8888:
    case SpraymakerModel::ImageFormat::BGRX8888:
    case SpraymakerModel::ImageFormat::RGBA8888:
    case SpraymakerModel::ImageFormat::UVLX8888:
    case SpraymakerModel::ImageFormat::UVWQ8888:
        return 4;
    case SpraymakerModel::ImageFormat::RGBA16161616:
    case SpraymakerModel::ImageFormat::RGBA16161616F:
        return 8;
    case SpraymakerModel::ImageFormat::RGBA32323232F:
    case SpraymakerModel::ImageFormat::RGB323232F:
    case SpraymakerModel::ImageFormat::R32F:
        return 16;
    default:
        return 0;
    }
}

uint ImageHelper::getImageDataSize(SpraymakerModel::ImageFormat format,
                                   uint width, uint height, uint mipmaps, uint frames)
{
    uint size;
    if (isDxt(format))
        size = sizeOfDxtImage(width, height, mipmaps, frames, bytesPerBlock(format));
    else
        size = sizeOfImage(width, height, mipmaps, frames, bytesPerPixel(format));
    return size;
}

void ImageHelper::getMaxResForTargetSize(SpraymakerModel::ImageFormat format,
                                         uint &width, uint &height, uint mipmaps, uint frames,
                                         uint size, uint step, bool square, bool powerOfTwo)
{
    width = 0;
    height = 0;
    step = std::max(1U, step);
    bool varyMipmaps = false;
    uint checkSize = 0;

    if (mipmaps == 0)
        varyMipmaps = true;

    int i = 0;

    // Assumption: Resoluton is desired to be either (k*step)x(k*step) or (k*step)x((k-1)*step)
    // In powerOfTwo mode the resolution is (2^k)x(2^k) or (2^k)x(2^(k-1))
    // Assumption: nobody is trying to make a top-level texture that's below the step
    // even though 1x1, 2x3, 5x1, etc. are valid depending on target format
    for(uint res = step; res <= 65535;)
    {
        width = res;
        height = res;

        if (varyMipmaps)
            mipmaps = ImageHelper::getMaxMipmaps(width, height);

        checkSize = getImageDataSize(format, width, height, mipmaps, frames);

        if (checkSize == size)
            break;

        if (checkSize > size)
        {
            if (powerOfTwo == true)
                height >>= 1;
            else
                height -= step;

            if (varyMipmaps)
                mipmaps = ImageHelper::getMaxMipmaps(width, height);

            checkSize = getImageDataSize(format, width, height, mipmaps, frames);

            if (checkSize > size || square == true)
                width = height;

            break;
        }

        if (powerOfTwo == true)
            res <<= 1;
        else
            res += step;
    }
}

uint ImageHelper::getMaxMipmaps(uint width, uint height)
{
    uint res = std::max(width, height);
    uint mipmaps = 1;
    while(res >>= 1 > 0) mipmaps++;
    return mipmaps;
}

void ImageHelper::convertPixelFormat(crnlib::color_quad_u8* dataPtr, uchar* pos, int count,
                                     crnlib::pixel_format srcFormat, SpraymakerModel::ImageFormat dstFormat,
                                     int alphaThreshold)
{
    bool doR = false;
    bool doG = false;
    bool doB = false;
    bool doA = false;
    switch(srcFormat)
    {
    case crnlib::pixel_format::PIXEL_FMT_A8R8G8B8:
        doR = true; doG = true; doB = true; doA = true;
        break;
    case crnlib::pixel_format::PIXEL_FMT_R8G8B8:
        doR = true; doG = true; doB = true;
        break;
    case crnlib::pixel_format::PIXEL_FMT_A8L8:
        doR = true; doA = true;
        break;
    case crnlib::pixel_format::PIXEL_FMT_L8:
        doR = true;
        break;
    case crnlib::pixel_format::PIXEL_FMT_A8:
        doA = true;
        break;
    default:
        throw SpraymakerException(QObject::tr("Unsupported crnlib pixel format. This should never happen."));
    }

    // Modify pixel bits for target output formats not directly supported by crnlib
    // Reference: https://learn.microsoft.com/en-us/windows/uwp/gaming/complete-code-for-ddstextureloader
    // The "X" formats don't set bits to 1, but this probably doesn't matter?
    if (dstFormat == SpraymakerModel::ImageFormat::ABGR8888)
    {
        for(int i = 0; i < count; i++)
        {
            *pos = dataPtr->a;
            pos++;
            *pos = dataPtr->b;
            pos++;
            *pos = dataPtr->g;
            pos++;
            *pos = dataPtr->r;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::BGR888)
    {
        for(int i = 0; i < count; i++)
        {
            *pos = dataPtr->b;
            pos++;
            *pos = dataPtr->g;
            pos++;
            *pos = dataPtr->r;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::BGR888_BLUESCREEN)
    {
        // Assumption: input image has an alpha channel and the user wants this to be
        // translated into the bluescreen format
        for(int i = 0; i < count; i++)
        {
            uchar r = dataPtr->r;
            uchar g = dataPtr->g;
            uchar b = dataPtr->b;

            // Prevent unintentional bluescreen effect
            if (r == 0 && g == 0 && b == 255)
                b = 254;

            // Convert alpha channel into bluescreen
            if (dataPtr->a < alphaThreshold)
            {
                r = 0;
                g = 0;
                b = 255;
            }

            *pos = b;
            pos++;
            *pos = g;
            pos++;
            *pos = r;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::RGB888_BLUESCREEN)
    {
        // TF2: Not supported.
        // Assumption: input image has an alpha channel and the user wants this to be
        // translated into the bluescreen format
        for(int i = 0; i < count; i++)
        {
            uchar r = dataPtr->r;
            uchar g = dataPtr->g;
            uchar b = dataPtr->b;

            // Prevent unintentional bluescreen effect
            if (r == 0 && g == 0 && b == 255)
                b = 254;

            // Convert alpha channel into bluescreen
            if (dataPtr->a < alphaThreshold)
            {
                r = 0;
                g = 0;
                b = 255;
            }

            *pos = r;
            pos++;
            *pos = g;
            pos++;
            *pos = b;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::BGRA8888
          || dstFormat == SpraymakerModel::ImageFormat::BGRX8888)
    {
        for(int i = 0; i < count; i++)
        {
            *pos = dataPtr->b;
            pos++;
            *pos = dataPtr->g;
            pos++;
            *pos = dataPtr->r;
            pos++;
            *pos = dataPtr->a;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::BGRA4444)
    {
        // TODO: Even with max alpha the textures are slightly transparent
        // Is that a bug, or just how this texture works with sprays?
        for(int i = 0; i < count; i++)
        {
            ushort bgra4444 = ((dataPtr->r & 0b11110000) << 4)
                            | ((dataPtr->g & 0b11110000) << 0)
                            | ((dataPtr->b & 0b11110000) >> 4)
                            | ((dataPtr->a & 0b11110000) << 8);
            *pos = bgra4444;
            pos++;
            *pos = bgra4444 >> 8;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::BGRA5551
             || dstFormat == SpraymakerModel::ImageFormat::BGRX5551)
    {
        for(int i = 0; i < count; i++)
        {
            ushort bgra5551 = ((dataPtr->r & 0b11111000) << 7)
                            | ((dataPtr->g & 0b11111000) << 2)
                            | ((dataPtr->b & 0b11111000) >> 3)
                            | ((dataPtr->a & 0b10000000) << 8);
            *pos = bgra5551;
            pos++;
            *pos = bgra5551 >> 8;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::BGR565)
    {
        for(int i = 0; i < count; i++)
        {
            ushort bgr565 = ((dataPtr->r & 0b11111000) << 8)
                          | ((dataPtr->g & 0b11111100) << 3)
                          | ((dataPtr->b & 0b11111000) >> 3);
            *pos = bgr565;
            pos++;
            *pos = bgr565 >> 8;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::RGB565)
    {
        // Note: not supported in TF2, other engines?
        for(int i = 0; i < count; i++)
        {
            ushort rgb565 = ((dataPtr->r & 0b11111000) >> 3)
                          | ((dataPtr->g & 0b11111100) << 3)
                          | ((dataPtr->b & 0b11111000) << 8);
            *pos = rgb565;
            pos++;
            *pos = rgb565 >> 8;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::UV88)
    {
        // TODO: This is probably wrong
        // Note: In sprays blue is always max with this format
        for(int i = 0; i < count; i++)
        {
            *pos = 0x7f * ((double)dataPtr->r / 255.0);
            pos++;
            *pos = 0x7f * ((double)dataPtr->g / 255.0);
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::UVWQ8888
             || dstFormat == SpraymakerModel::ImageFormat::UVLX8888)
    {
        // TODO: This is probably wrong
        for(int i = 0; i < count; i++)
        {
            *pos = 0x7f * ((double)dataPtr->r / 255.0);
            pos++;
            *pos = 0x7f * ((double)dataPtr->g / 255.0);
            pos++;
            *pos = 0x7f * ((double)dataPtr->b / 255.0);
            pos++;
            *pos = 0x7f * ((double)dataPtr->a / 255.0);
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::RGBA16161616)
    {
        // TODO: This is faked
        for(int i = 0; i < count; i++)
        {
            *pos =         0xffff * ((double)dataPtr->r / 255.0);
            pos++;
            *pos = ((uint)(0xffff * ((double)dataPtr->r / 255.0))) >> 8;
            pos++;
            *pos =         0xffff * ((double)dataPtr->g / 255.0);
            pos++;
            *pos = ((uint)(0xffff * ((double)dataPtr->g / 255.0))) >> 8;
            pos++;
            *pos =         0xffff * ((double)dataPtr->b / 255.0);
            pos++;
            *pos = ((uint)(0xffff * ((double)dataPtr->b / 255.0))) >> 8;
            pos++;
            *pos =         0xffff * ((double)dataPtr->a / 255.0);
            pos++;
            *pos = ((uint)(0xffff * ((double)dataPtr->a / 255.0))) >> 8;
            pos++;
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::RGBA32323232F)
    {
        // TODO: This is faked and also totally wrong
        for(int i = 0; i < count; i++)
        {
            auto r = (float)(0xffffffff * ((double)dataPtr->r / 255.0));
            auto g = (float)(0xffffffff * ((double)dataPtr->g / 255.0));
            auto b = (float)(0xffffffff * ((double)dataPtr->b / 255.0));
            auto a = (float)(0xffffffff * ((double)dataPtr->a / 255.0));
            auto rf = reinterpret_cast<const uchar*>(&r);
            auto gf = reinterpret_cast<const uchar*>(&g);
            auto bf = reinterpret_cast<const uchar*>(&b);
            auto af = reinterpret_cast<const uchar*>(&a);
            for(int i = 0; i < 4; i++) { *pos = rf[i]; pos++; }
            for(int i = 0; i < 4; i++) { *pos = gf[i]; pos++; }
            for(int i = 0; i < 4; i++) { *pos = bf[i]; pos++; }
            for(int i = 0; i < 4; i++) { *pos = af[i]; pos++; }
            dataPtr++;
        }
    }
    else if (dstFormat == SpraymakerModel::ImageFormat::RGB888
          || dstFormat == SpraymakerModel::ImageFormat::RGBA8888
          || dstFormat == SpraymakerModel::ImageFormat::A8
          || dstFormat == SpraymakerModel::ImageFormat::I8
          || dstFormat == SpraymakerModel::ImageFormat::IA88)
    {
        // Uncompressed pixel output formats supported by crnlib directly:
        // RGB888, RGBA8888, A8, I8, IA88
        // Has a 4 byte pixel stride regardless of output format
        for(int i = 0; i < count; i++)
        {
            if (doR)
            {
                *pos = dataPtr->r;
                pos++;
            }
            if (doG)
            {
                *pos = dataPtr->g;
                pos++;
            }
            if (doB)
            {
                *pos = dataPtr->b;
                pos++;
            }
            if (doA)
            {
                *pos = dataPtr->a;
                pos++;
            }
            dataPtr++;
        }
    }
}
