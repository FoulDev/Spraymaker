/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "imageloader_ffmpeg.h"
#include "spraymakerexception.h"
#include "util.h"

#include <generator>
#include <QObject>

ImageLoaderFfmpeg::ImageLoaderFfmpeg(const char* inputFile)
{
    this->inputFile = inputFile;
    initDecoder();
}

ImageLoaderFfmpeg::~ImageLoaderFfmpeg()
{}

void ImageLoaderFfmpeg::fillStreamInfo(const AVCodecParameters& avCodecParameters)
{
    auto avCodec = (AVCodec*)avcodec_find_decoder(avCodecParameters.codec_id);

    if (avCodec == nullptr)
        throw SpraymakerException(QObject::tr("Unsupported file format."));

    avCodecContext = avcc_unique_ptr(avcodec_alloc_context3(avCodec));
    if (avCodecContext.get() == nullptr)
        throw SpraymakerException(QObject::tr("Error reading input file."));

    if (avcodec_parameters_to_context(avCodecContext.get(), &avCodecParameters) < 0)
        throw SpraymakerException(QObject::tr("Error reading input file."));

    if (avcodec_open2(avCodecContext.get(), avCodec, nullptr) < 0)
        throw SpraymakerException(QObject::tr("Error reading input file."));
}

void ImageLoaderFfmpeg::initDecoder()
{
    // Open file
    {
        AVFormatContext* wrapFormatContext = avformat_alloc_context();

        if (wrapFormatContext == nullptr)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        if (avformat_open_input(&wrapFormatContext, inputFile, NULL, NULL) != 0)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        formatContext = avfc_unique_ptr(wrapFormatContext);
        wrapFormatContext = nullptr;
    }

    if (avformat_find_stream_info(formatContext.get(), NULL) < 0)
        throw SpraymakerException(QObject::tr("Error reading input file."));

    // Setup decoder
    for (int i = 0; i < formatContext->nb_streams; i++) {
        auto stream = formatContext->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            auto avStream = formatContext->streams[i];
            fillStreamInfo(*avStream->codecpar);
        }
    }
}

std::generator<const RGBAFrame>
ImageLoaderFfmpeg::decodeToRgba(avp_unique_ptr& inputPacket, avf_unique_ptr& inputFrame)
{
    int response = avcodec_send_packet(avCodecContext.get(), inputPacket.get());
    if (response < 0)
        throw SpraymakerException(QObject::tr("Error reading input file."));

    const int width = avCodecContext->width;
    const int height = avCodecContext->height;
    const int size = width * height * 4;

    RGBAFrame output {
        .width  = width,
        .height = height,
        .size   = size,
        .buffer = new uchar[size],
    };

    while (response >= 0)
    {
        response = avcodec_receive_frame(avCodecContext.get(), inputFrame.get());

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
            break;
        else if (response < 0)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        const auto sourcePixelFormat = avCodecContext->pix_fmt;
        const auto destinationPixelFormat = AV_PIX_FMT_RGBA;

        auto swsContext = avswsc_unique_ptr(sws_getContext(
            width, height, sourcePixelFormat,
            width, height, destinationPixelFormat,
            SWS_BILINEAR, NULL, NULL, NULL
            ));

        if (swsContext.get() == nullptr)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        auto rgbaFrame = avf_unique_ptr(av_frame_alloc());
        if (rgbaFrame.get() == nullptr)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        rgbaFrame->format = destinationPixelFormat;
        rgbaFrame->width  = width;
        rgbaFrame->height = height;

        int ret = av_frame_get_buffer(rgbaFrame.get(), 0);
        if (ret < 0)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        ret = sws_scale(
            swsContext.get(), inputFrame->data, inputFrame->linesize,
            0, height, rgbaFrame->data, rgbaFrame->linesize);

        if (ret < 0)
            throw SpraymakerException(QObject::tr("Error reading input file."));

        for(int y = 0; y < height; y++)
        {
            memcpy((uchar*)output.buffer + (y * width * 4),
                   rgbaFrame->data[0]    + (y * rgbaFrame->linesize[0]),
                   width * 4);
        }

        co_yield output;

        av_frame_unref(inputFrame.get());
    }
}

std::generator<const RGBAFrame>
ImageLoaderFfmpeg::getFrames()
{
    avf_unique_ptr inputFrame(av_frame_alloc());
    if (inputFrame.get() == nullptr)
        throw SpraymakerException(QObject::tr("Error reading input file."));

    avp_unique_ptr inputPacket(av_packet_alloc());
    if (inputPacket.get() == nullptr)
        throw SpraymakerException(QObject::tr("Error reading input file."));

    while(av_read_frame(formatContext.get(), inputPacket.get()) >= 0)
    {
        auto avStream = formatContext->streams[inputPacket->stream_index];
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            for(RGBAFrame frame : decodeToRgba(inputPacket, inputFrame))
            {
                co_yield frame;
            }
            av_packet_unref(inputPacket.get());
        }
    }
}
