/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IMAGEHELPER_H
#define IMAGEHELPER_H

#include "spraymakermodel.h"

#include <QPixmap>
#include <QString>
#include <crunch/crnlib/crn_color.h>

// ========== ImageHelper ==========

class ImageHelper
{
public:
    enum class PixelAlphaMode : int
    {
        INVALID = -1,

        NONE,
        THRESHOLD,
        FULL,

        _MAX = FULL,
        _COUNT,
    };

    struct BoundingBox
    {
        uint left   = -1U;
        uint right  = 0;
        uint top    = -1U;
        uint bottom = 0;
        uint width  = 0;
        uint height = 0;

        BoundingBox& operator+=(const BoundingBox& bb)
        { add(bb); return *this; }

        friend BoundingBox operator+(BoundingBox lbb, const BoundingBox& bb)
        { lbb += bb; return lbb; }

    private:
        void add(const BoundingBox& bb)
        {
            left   = std::min(left,   bb.left);
            right  = std::max(right,  bb.right);
            top    = std::min(top,    bb.top);
            bottom = std::max(bottom, bb.bottom);
            width  = std::max(width,  bb.width);
            height = std::max(height, bb.height);
        }
    };

    static const BoundingBox getImageBorders(const void* pixels, uint width, uint height,
                                             PixelAlphaMode pixelAlphaMode,
                                             uint alphaThreshold);

    static uint getImageDataSize(SpraymakerModel::ImageFormat format,
                                 uint width, uint height, uint mipmaps, uint frames);
    static void getMaxResForTargetSize(SpraymakerModel::ImageFormat format,
                                       uint &width, uint &height, uint mipmaps, uint frames,
                                       uint size, uint step, bool square, bool powerOfTwo);
    static uint getMaxMipmaps(uint width, uint height);
    static bool isDxt(SpraymakerModel::ImageFormat format);
    static bool hasOneBitAlpha(SpraymakerModel::ImageFormat format);
    static bool hasMultiBitAlpha(SpraymakerModel::ImageFormat format);
    static bool hasAlpha(SpraymakerModel::ImageFormat format);
    static uint getPixelArtBoxSize(const vips::VImage img);
    static void convertPixelFormat(crnlib::color_quad<uchar, int>* dataPtr, uchar* pos, int count,
                                   crnlib::pixel_format srcFormat, SpraymakerModel::ImageFormat dstFormat,
                                   int alphaThreshold);

private:
    static uint sizeOfDxtImage(uint width, uint height,
                               uint mipmaps, uint frames, uint bytesPerBlock);
    static uint sizeOfImage(uint width, uint height,
                            uint mipmaps, uint frames, uint bytesPerPixel);
    static uint bytesPerBlock(SpraymakerModel::ImageFormat format);
    static uint bytesPerPixel(SpraymakerModel::ImageFormat format);
};

#endif // IMAGEHELPER_H
