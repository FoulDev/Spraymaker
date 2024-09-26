/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IMAGELOADER_FFMPEG_H
#define IMAGELOADER_FFMPEG_H

#include "util.h"
#include <generator>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

struct RGBAFrame
{
    int width    = -1;
    int height   = -1;
    int size     = -1;
    void* buffer = nullptr;
};

class ImageLoaderFfmpeg
{
public:
    ImageLoaderFfmpeg(const char* inputFile);
    ~ImageLoaderFfmpeg();
    std::generator<const RGBAFrame> getFrames();

protected:
    const char* inputFile;
    avfc_unique_ptr formatContext;
    avcc_unique_ptr avCodecContext;

    void initDecoder();
    void openFile();
    void fillStreamInfo(const AVCodecParameters& avCodecParameters);
    std::generator<const RGBAFrame> decodeToRgba(avp_unique_ptr& inputPacket, avf_unique_ptr& inputFrame);
};

#endif // IMAGELOADER_FFMPEG_H
