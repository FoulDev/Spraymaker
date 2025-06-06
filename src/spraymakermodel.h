/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SPRAYMAKERMODEL_H
#define SPRAYMAKERMODEL_H

#include "imagemanager.h"
#include "vtf_defs.h"

#include <dds_defs.h>
#include <crnlib.h>

#include <QObject>

class SpraymakerModel : public QObject
{
    Q_OBJECT

public:
    // Spraymaker internal format names
    enum class ImageFormat : int {
        INVALID = -1,

        DXT1 = 0,
        DXT1A,
        DXT3,
        DXT5,

        A8,
        I8,
        P8,
        IA88,

        BGR565,
        BGR888,
        BGR888_BLUESCREEN,
        BGRA4444,
        BGRA5551,
        BGRA8888,
        BGRX5551,
        BGRX8888,

        RGB565,
        RGB888,
        RGB888_BLUESCREEN,
        RGBA8888,

        ABGR8888,
        ARGB8888,

        RGBA16161616,
        RGBA16161616F,

        UV88,
        UVLX8888,
        UVWQ8888,

        R32F,
        RGB323232F,
        RGBA32323232F,

        _MAX = RGBA32323232F,
        _COUNT,
    };
    Q_ENUM(ImageFormat);

    // TODO: These are *probably* mutually exclusive.
    enum class TextureSampleMode : int {
        INVALID = -1,

        TRILINEAR = 0,
        ANISOTROPIC,
        POINT_SAMPLE,
        NONE,

        _MAX = NONE,
        _COUNT,
    };
    Q_ENUM(TextureSampleMode);

    enum class MipmapInputMode : int {
        INVALID = -1,

        ONE_ONLY = 0,
        MAX_ONLY,
        USER_OPTION,

        _MAX = USER_OPTION,
        _COUNT,
    };
    Q_ENUM(MipmapInputMode);

    enum class ResolutionInputMode : int {
        INVALID = -1,

        AUTOMATIC = 0,
        MANUAL,
        FREE,

        _MAX = FREE,
        _COUNT,
    };
    Q_ENUM(ResolutionInputMode);

    enum class MipmapPropagationMode : int {
        INVALID = -1,

        FILL = 0,
        NO_OVERWRITE,
        NONE,

        _MAX = NONE,
        _COUNT,
    };
    Q_ENUM(MipmapPropagationMode);

    enum class AutocropMode : int {
        INVALID = -1,

        AUTOMATIC = 0,
        INDIVIDUAL,
        BOUNDINGBOX,
        NONE,

        _MAX = NONE,
        _COUNT,
    };
    Q_ENUM(AutocropMode);

    struct Formats {
        const ImageFormat format = ImageFormat::INVALID;
        const crnlib::pixel_format crnFormat = crnlib::pixel_format::PIXEL_FMT_INVALID;
        const VTF_IMAGE_FORMAT vtfFormat = VTF_IMAGE_FORMAT::NONE;
        const QString simpleName;
        const QString realName;
        const bool isSimple = false;
        const bool hide = false;
    };

    const Formats EnumMapper[(int)ImageFormat::_COUNT] =
        {
            {
                .format     = ImageFormat::DXT1,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_DXT1,
                .vtfFormat  = VTF_IMAGE_FORMAT::DXT1,
                .simpleName = tr("Compressed with no transparency"),
                .realName   = "DXT1 / BC1",
            },{
                .format     = ImageFormat::DXT1A,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_DXT1A,
                .vtfFormat  = VTF_IMAGE_FORMAT::DXT1, // Note: DXT1A as specificed in VTF enum is broken
                .simpleName = tr("Compressed with background transparency"),
                .realName   = "DXT1A / BC1",
                .isSimple   = true,
            },{
                .format     = ImageFormat::DXT3,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_DXT3,
                .vtfFormat  = VTF_IMAGE_FORMAT::DXT3,
                .simpleName = tr("Compressed with crappy transparency"),
                .realName   = "DXT3 / BC3",
            },{
                .format     = ImageFormat::DXT5,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_DXT5,
                .vtfFormat  = VTF_IMAGE_FORMAT::DXT5,
                .simpleName = tr("Compressed with full transparency"),
                .realName   = "DXT5 / BC5",
                .isSimple   = true,
            },{
                .format     = ImageFormat::RGB888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGB888,
                .simpleName = tr("Uncompressed"),
                .realName   = "RGB888",
            },{
                .format     = ImageFormat::BGR888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGR888,
                .simpleName = tr("Uncompressed"),
                .realName   = "BGR888",
            },{
                .format     = ImageFormat::BGR888_BLUESCREEN,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGR888_BLUESCREEN,
                .simpleName = tr("Uncompressed with background transparency"),
                .realName   = tr("BGR888 Bluescreen"),
                .isSimple   = true
            },{
                .format     = ImageFormat::RGBA8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGBA8888,
                .simpleName = tr("Uncompressed with full transparency"),
                .realName   = "RGBA8888",
                .isSimple   = true,
            },{
                .format     = ImageFormat::BGRA8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGRA8888,
                .simpleName = tr("Uncompressed with full transparency"),
                .realName   = "BGRA8888",
            },{
                .format     = ImageFormat::BGRX8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGRX8888,
                .simpleName = "BGRX8888",
                .realName   = "BGRX8888",
                .hide       = true, // X component
            },{
                .format     = ImageFormat::A8,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8,
                .vtfFormat  = VTF_IMAGE_FORMAT::A8,
                .simpleName = tr("Stencil with full transparency"),
                .realName   = "A8",
            },{
                .format     = ImageFormat::I8,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_L8,
                .vtfFormat  = VTF_IMAGE_FORMAT::I8,
                .simpleName = tr("Uncompressed black and white"),
                .realName   = "I8",
                .isSimple   = true,
            },{
                .format     = ImageFormat::P8,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_INVALID,
                .vtfFormat  = VTF_IMAGE_FORMAT::P8,
                .simpleName = tr("256-colour palette"),
                .realName   = "P8",
                .isSimple   = false,
                .hide       = true, // Not implemented; VTF doesn't support it anyway
            },{
                .format     = ImageFormat::IA88,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8L8,
                .vtfFormat  = VTF_IMAGE_FORMAT::IA88,
                .simpleName = tr("Uncompressed black and white with full transparency"),
                .realName   = "IA88",
                .isSimple   = true,
            },{
                // TF2: The bluescreen effect here doesn't work on sprays. Use BGR888_BLUESCREEN
                .format     = ImageFormat::RGB888_BLUESCREEN,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGB888,
                .simpleName = tr("RGB888 Bluescreen"),
                .realName   = tr("RGB888 Bluescreen"),
            },{
                // TF2: Not supported
                .format     = ImageFormat::RGB565,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGB565,
                .simpleName = "RGB565",
                .realName   = "RGB565",
            },{
                .format     = ImageFormat::BGR565,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGR565,
                .simpleName = "BGR565",
                .realName   = "BGR565",
            },{
                .format     = ImageFormat::BGRA4444,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGRA4444,
                .simpleName = "BGRA4444",
                .realName   = "BGRA4444",
            },{
                .format     = ImageFormat::BGRA5551,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGRA5551,
                .simpleName = "BGRA5551",
                .realName   = "BGRA5551",
            },{
                .format     = ImageFormat::BGRX5551,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::BGRX5551,
                .simpleName = "BGRX5551",
                .realName   = "BGRX5551",
                .hide       = true, // X component
            },{
                .format     = ImageFormat::UV88,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::UV88,
                .simpleName = "UV88",
                .realName   = "UV88",
                .hide       = true, // Fake implementation
            },{
                .format     = ImageFormat::UVWQ8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::UVWQ8888,
                .simpleName = "UVWQ8888",
                .realName   = "UVWQ8888",
                .hide       = true, // Fake implementation
            },{
                .format     = ImageFormat::UVLX8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::UVLX8888,
                .simpleName = "UVLX8888",
                .realName   = "UVLX8888",
                .hide       = true, // Fake implementation, X component
            },{
                .format     = ImageFormat::RGBA16161616,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGBA16161616,
                .simpleName = "RGBA16161616",
                .realName   = "RGBA16161616",
                .hide       = true, // Not implemented correctly
            },{
                .format     = ImageFormat::RGBA16161616F,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGBA16161616F,
                .simpleName = "RGBA16161616F",
                .realName   = "RGBA16161616F",
                .hide       = true, // Not implemented
            },{
                .format     = ImageFormat::R32F,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::R32F,
                .simpleName = "R32F",
                .realName   = "R32F",
                .hide       = true, // Not implemented
            },{
                .format     = ImageFormat::RGB323232F,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGB323232F,
                .simpleName = "RGB323232F",
                .realName   = "RGB323232F",
                .hide       = true, // Not implemented
            },{
                .format     = ImageFormat::RGBA32323232F,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::RGBA32323232F,
                .simpleName = "RGBA32323232F",
                .realName   = "RGBA32323232F",
                .hide       = true, // Not implemented correctly
            },{
                .format     = ImageFormat::ABGR8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::ABGR8888,
                .simpleName = "ABGR8888",
                .realName   = "ABGR8888",
                .hide       = true, // Not implemented
            },{
                .format     = ImageFormat::ARGB8888,
                .crnFormat  = crnlib::pixel_format::PIXEL_FMT_A8R8G8B8,
                .vtfFormat  = VTF_IMAGE_FORMAT::ARGB8888,
                .simpleName = "ARGB8888",
                .realName   = "ARGB8888",
                .hide       = true, // Not implemented
            },
        };

    SpraymakerModel();

    void beginSetup();
    void finishSetup();
    void invalidateProgress();

    int getMipmapCount();
    int getFrameCount();
    int getMaxMipmapCount();

    int getWidth();
    int getHeight();
    int getMaxResolution();

    int getVtfFileSize();
    int getMaxVtfFileSize();

    int getBackgroundRed();
    int getBackgroundGreen();
    int getBackgroundBlue();
    int getBackgroundAlpha();

    bool getUseSimpleFormatNames();

    std::string getFile(int mipmap, int frame);
    const vips::VImage* getImage(int mipmap, int frame);
    const QPixmap& getPreview(int mipmap, int frame);

    ImageFormat getFormat();
    Formats mapFormat();

    MipmapInputMode getMipmapInputMode();
    ResolutionInputMode getResolutionInputMode();
    TextureSampleMode getTextureSampleMode();
    MipmapPropagationMode getMipmapPropagationMode();
    AutocropMode getAutocropMode();

    ImageFormat getFormatFromComboBoxIndex(int index);
    int getComboBoxIndexFromFormat(ImageFormat format);

private:
    // images[mipmap][frame]
    std::vector<std::vector<vips::VImage>> images;
    // previews[mipmap][frame]
    std::vector<std::vector<QPixmap>> previews;
    // files[mipmap][frame] = /some/filesystem/path.png
    std::vector<std::vector<std::string>> files;

    void resizeVectors();

    bool suppress;

    int mipmaps;
    int frames;
    int maxMipmaps;
    int width;
    int height;
    int vtfFileSize;
    int maxVtfFileSize;
    int maxResolution;
    int backgroundRed;
    int backgroundGreen;
    int backgroundBlue;
    int backgroundAlpha;
    bool useSimpleFormatNames;

    ImageFormat imageFormat;

    MipmapInputMode mipmapInputMode;
    ResolutionInputMode resolutionInputMode;
    TextureSampleMode textureSampleMode;
    MipmapPropagationMode mipmapPropagationMode;
    AutocropMode autocropMode;

    std::unordered_map<int, ImageFormat> comboBoxIndexToFormatMap;
    std::unordered_map<ImageFormat, int> formatToComboBoxIndexMap;

public slots:
    void importImage(const ImageInfo& imageInfo, int mipmap, int frame);
    void importImages(const std::list<ImageInfo>& files, int mipmap, int frame);
    void copyImage(int fromMipmap, int fromFrame, int toMipmap, int toFrame);
    void setPreview(const QPixmap preview, int mipmap, int frame);
    void setImage(vips::VImage image, std::string file, int mipmap, int frame);
    void setDimensions(int mipmaps, int frames);
    void setMipmapCount(int mipmaps);
    void setMaxMipmapCount(int maxMipmaps);
    void setMipmapInputMode(MipmapInputMode mipmapInputMode);
    void setFrameCount(int frames);
    void setWidth(int width);
    void setHeight(int height);
    void setResolution(int width, int height);
    void setMaxResolution(int maxResolution);
    void setVtfFileSize(int vtfFileSize);
    void setMaxVtfFileSize(int maxVtfFileSize);
    void setImageFormat(ImageFormat imageFormat);
    void setUseSimpleFormatNames(bool useSimpleFormatNames);
    void setFormatComboBoxMappers(
        std::unordered_map<int, ImageFormat> comboBoxIndexToFormatMap,
        std::unordered_map<ImageFormat, int> formatToComboBoxIndexMap);
    void setResolutionInputMode(ResolutionInputMode resolutionInputMode);
    void setMipmapPropagationMode(MipmapPropagationMode mipmapPropagationMode);
    void setTextureSampleMode(TextureSampleMode textureSampleMode);
    void setAutocropMode(AutocropMode autocropMode);
    void setBackground(int r, int g, int b, int a);
    void setBackgroundRed(int value);
    void setBackgroundGreen(int value);
    void setBackgroundBlue(int value);
    void setBackgroundAlpha(int value);

signals:
    void dimensionsChanged(int mipmaps, int frames);
    void mipmapCountChanged(int mipmaps);
    void frameCountChanged(int frames);
    void maxMipmapCountChanged(int maxMipmaps);
    void mipmapInputModeChanged(MipmapInputMode mipmapInputMode);
    void selectedImageChanged(int mipmap, int frame);
    void progressInvalidated();
    void previewChanged(const QPixmap& preview, int mipmap, int frame);
    void widthChanged(int width);
    void heightChanged(int height);
    void resolutionChanged(int width, int height);
    void maxResolutionChanged(int resolution);
    void vtfFileSizeChanged(int vtfFileSize);
    void maxVtfFileSizeChanged(int maxVtfFileSize);
    void imageFormatChanged(ImageFormat imageFormat);
    void newVtfFileSizeNeeded();
    void useSimpleFormatNamesChanged(bool useSimpleFormatNames);
    void formatComboBoxMappersChanged();
    void resolutionInputModeChanged(ResolutionInputMode resolutionInputMode);
    void newResolutionNeeded();
    void textureSampleModeChanged(TextureSampleMode textureSampleMode);
    void mipmapPropagationModeChanged(MipmapPropagationMode mipmapPropagationMode);
    void autocropModeChanged(AutocropMode autocropMode);
    void backgroundRedChanged(int value);
    void backgroundGreenChanged(int value);
    void backgroundBlueChanged(int value);
    void backgroundAlphaChanged(int value);
    void backgroundColourChanged(int r, int g, int b, int a);
};

#endif // SPRAYMAKERMODEL_H
