/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef VTF_DEFS_H
#define VTF_DEFS_H

#include <stdint.h>

// TODO: This depends on target Source engine branch
enum class VTF_IMAGE_FORMAT : uint32_t
{
    NONE = -1U,
    RGBA8888 = 0,
    ABGR8888,
    RGB888,
    BGR888,
    RGB565,
    I8,
    IA88,
    P8,
    A8,
    RGB888_BLUESCREEN,
    BGR888_BLUESCREEN,
    ARGB8888,
    BGRA8888,
    DXT1,
    DXT3,
    DXT5,
    BGRX8888,
    BGR565,
    BGRX5551,
    BGRA4444,
    DXT1_ONEBITALPHA,
    BGRA5551,
    UV88,
    UVWQ8888,
    RGBA16161616F,
    RGBA16161616,
    UVLX8888,
    R32F,
    RGB323232F,
    RGBA32323232F,
};

// TODO: This depends on target Source engine branch
enum class VTF_FLAGS : uint32_t
{
    NONE              = 0x00000000,
    POINTSAMPLE       = 0x00000001, // Pixel art style. Incompatible with mipmaps.
    TRILINEAR         = 0x00000002, // Filtering between mipmap levels
    CLAMPS            = 0x00000004, // Prevent tiling on sides
    CLAMPT            = 0x00000008, // Prevent tiling on top
    ANISOTROPIC       = 0x00000010, // Improved filtering between mipmap levels
    HINT_DXT5         = 0x00000020, // Makes edges seamless in skyboxes?
    PWL_CORRECTED     = 0x00000040,
    NORMAL            = 0x00000080, // Normal map
    NOMIP             = 0x00000100, // Show only largest mipmap
    NOLOD             = 0x00000200, // Bypass graphics settings to show all mip levels
    ALL_MIPS          = 0x00000400, // Show mipmaps under 32 pixels in size
    PROCEDURAL        = 0x00000800,
    ONEBITALPHA       = 0x00001000, // 1-bit alpha
    EIGHTBITALPHA     = 0x00002000, // Greater than 1-bit alpha (name lies)
    ENVMAP            = 0x00004000, // Environment map
    RENDERTARGET      = 0x00008000, // Render target
    DEPTHRENDERTARGET = 0x00010000, // Depth render target
    NODEBUGOVERRIDE   = 0x00020000,
    SINGLECOPY        = 0x00040000,
    PRE_SRGB          = 0x00080000, // Preapplied SRGB correction
    UNUSED_00100000   = 0x00100000,
    UNUSED_00200000   = 0x00200000,
    UNUSED_00400000   = 0x00400000,
    NODEPTHBUFFER     = 0x00800000, // No z-buffering
    UNUSED_01000000   = 0x01000000,
    CLAMPU            = 0x02000000, // Prevent tiling on "U" for volumetric textures
    VERTEXTEXTURE     = 0x04000000, // Vertex texture
    SSBUMP            = 0x08000000, // Self-shading bumpmap
    UNUSED_10000000   = 0x10000000,
    BORDER            = 0x20000000,
    UNUSED_40000000   = 0x40000000,
    UNUSED_80000000   = 0x80000000,
};

// Enable bitwise operators on enum class VTF_FLAGS
constexpr enum VTF_FLAGS operator |(const enum VTF_FLAGS selfValue, const enum VTF_FLAGS inValue)
{ return (enum VTF_FLAGS)(uint32_t(selfValue) | uint32_t(inValue)); }

constexpr enum VTF_FLAGS operator &(const enum VTF_FLAGS selfValue, const enum VTF_FLAGS inValue)
{ return (enum VTF_FLAGS)(uint32_t(selfValue) & uint32_t(inValue)); }

constexpr enum VTF_FLAGS operator ^(const enum VTF_FLAGS selfValue, const enum VTF_FLAGS inValue)
{ return (enum VTF_FLAGS)(uint32_t(selfValue) ^ uint32_t(inValue)); }

constexpr enum VTF_FLAGS operator ~(const enum VTF_FLAGS selfValue)
{ return (enum VTF_FLAGS)(~uint32_t(selfValue)); }

#pragma pack(push,1)
// Only VTF version 7.1 is relevant for our purposes
struct VTF_HEADER_71
{
    char              signature[4];
    uint32_t          version[2];
    uint32_t          headerSize;
    uint16_t          width;
    uint16_t          height;
    VTF_FLAGS         flags;
    uint16_t          frames;
    uint16_t          firstFrame;
    uint8_t           padding0[4];
    float             reflectivity[3];
    uint8_t           padding1[4];
    float             bumpmapScale;
    VTF_IMAGE_FORMAT  highResImageFormat;
    uint8_t           mipmapCount;
    VTF_IMAGE_FORMAT  lowResImageFormat;
    uint8_t           lowResImageWidth;
    uint8_t           lowResImageHeight;
    uint8_t           padding2;
};
#pragma pack(pop)

#endif // VTF_DEFS_H
