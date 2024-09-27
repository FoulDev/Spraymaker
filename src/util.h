/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UTIL_H
#define UTIL_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

#include <crnlib.h>

#include <memory>

// ========== crnlib memory management ==========

template<typename T, auto FreeBlock>
using crn_unique_ptr_t = std::unique_ptr<T, decltype([](auto p){FreeBlock(p);})>;
using crn_unique_ptr = crn_unique_ptr_t<char, crn_free_block>;

// ========== ffmpeg memory management ==========

template<typename T, auto AvFree>
struct AVDeleter  { void operator()(T* p) { AvFree(&p); } };

template<typename T, auto AvFree>
struct AVDeleter2 { void operator()(T* p) { AvFree(p);  } };

template<typename T, auto AvFree>
using av_unique_ptr_t   = std::unique_ptr<T, decltype(AVDeleter <T, AvFree>())>;

template<typename T, auto AvFree>
using av_unique_ptr_t2  = std::unique_ptr<T, decltype(AVDeleter2<T, AvFree>())>;

using avf_unique_ptr    = av_unique_ptr_t <AVFrame,         av_frame_free>;
using avp_unique_ptr    = av_unique_ptr_t <AVPacket,        av_packet_free>;
using avfc_unique_ptr   = av_unique_ptr_t <AVFormatContext, avformat_close_input>;
using avcc_unique_ptr   = av_unique_ptr_t <AVCodecContext,  avcodec_free_context>;
using avswsc_unique_ptr = av_unique_ptr_t2<SwsContext,      sws_freeContext>;

#endif // UTIL_H
