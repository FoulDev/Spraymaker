/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "spraymakermodel.h"

// ========== SpraymakerModel ==========

SpraymakerModel::SpraymakerModel()
    // Reasonable defaults.
    // Some of these values will be automatically recalculated.
    : suppress(true)
    , mipmaps(1)
    , frames(1)
    , width(1024)
    , height(1020)
    , maxResolution(4096)
    , vtfFileSize(0)
    , maxVtfFileSize(512*1024)
    , maxMipmaps(11)
    , mipmapPropagationMode(MipmapPropagationMode::FILL)
    , resolutionInputMode(ResolutionInputMode::AUTOMATIC)
    , imageFormat(ImageFormat::DXT1A)
    , mipmapInputMode(MipmapInputMode::ONE_ONLY)
    , textureSampleMode(TextureSampleMode::ANISOTROPIC)
    , autocropMode(AutocropMode::AUTOMATIC)
    , backgroundRed(0)
    , backgroundGreen(0)
    , backgroundBlue(0)
    , backgroundAlpha(0)
{}

void SpraymakerModel::beginSetup()
{
    suppress = false;
    resizeVectors();
}

void SpraymakerModel::finishSetup()
{
    setUseSimpleFormatNames(useSimpleFormatNames);
    setMipmapPropagationMode(mipmapPropagationMode);
    setResolutionInputMode(resolutionInputMode);
    setMipmapInputMode(mipmapInputMode);
    setAutocropMode(autocropMode);
    setTextureSampleMode(textureSampleMode);

    setBackground(backgroundRed, backgroundGreen, backgroundBlue, backgroundAlpha);
    setVtfFileSize(vtfFileSize);
    setMaxVtfFileSize(maxVtfFileSize);
    setImageFormat(imageFormat);
    setMaxMipmapCount(maxMipmaps);
    setDimensions(mipmaps, frames);
    setResolution(width, height);

    suppress = true;
}

int SpraymakerModel::getMipmapCount()
{ return mipmaps; }

int SpraymakerModel::getFrameCount()
{ return frames; }

void SpraymakerModel::setDimensions(int mipmaps, int frames)
{
    setMipmapCount(mipmaps);
    setFrameCount(frames);
}

void SpraymakerModel::setMipmapCount(int mipmaps)
{
    if (suppress && this->mipmaps == mipmaps)
        return;

    emit progressInvalidated();

    this->mipmaps = mipmaps;
    resizeVectors();

    emit mipmapCountChanged(mipmaps);
    emit dimensionsChanged(mipmaps, frames);
    emit newVtfFileSizeNeeded();
    emit newResolutionNeeded();
}

void SpraymakerModel::setMaxMipmapCount(int maxMipmaps)
{
    if (suppress && this->maxMipmaps == maxMipmaps)
        return;

    this->maxMipmaps = maxMipmaps;
    emit maxMipmapCountChanged(maxMipmaps);
}

void SpraymakerModel::setMipmapInputMode(MipmapInputMode mipmapInputMode)
{
    if (suppress && this->mipmapInputMode == mipmapInputMode)
        return;

    this->mipmapInputMode = mipmapInputMode;
    emit mipmapInputModeChanged(mipmapInputMode);
}

SpraymakerModel::MipmapInputMode SpraymakerModel::getMipmapInputMode()
{ return mipmapInputMode; }

int SpraymakerModel::getMaxMipmapCount()
{ return maxMipmaps; }

void SpraymakerModel::setFrameCount(int frames)
{
    if (suppress && this->frames == frames)
        return;

    emit progressInvalidated();

    this->frames = frames;
    resizeVectors();

    emit frameCountChanged(frames);
    emit dimensionsChanged(mipmaps, frames);
    emit newVtfFileSizeNeeded();
    emit newResolutionNeeded();
}

void SpraymakerModel::setWidth(int width)
{
    if (suppress && this->width == width)
        return;

    emit progressInvalidated();

    this->width = width;

    emit resolutionChanged(width, height);
    emit widthChanged(width);
    emit newVtfFileSizeNeeded();
}

void SpraymakerModel::setHeight(int height)
{
    if (suppress && this->height == height)
        return;

    emit progressInvalidated();

    this->height = height;

    emit resolutionChanged(width, height);
    emit heightChanged(height);
    emit newVtfFileSizeNeeded();
}

void SpraymakerModel::setResolution(int width, int height)
{
    setWidth(width);
    setHeight(height);
}

int SpraymakerModel::getWidth()
{ return width; }

int SpraymakerModel::getHeight()
{ return height; }

void SpraymakerModel::resizeVectors()
{
    images.resize(mipmaps);
    for(auto& mipmap:images)
        mipmap.resize(frames);

    previews.resize(mipmaps);
    for(auto& mipmap:previews)
        mipmap.resize(frames);

    files.resize(mipmaps);
    for(auto& mipmap:files)
        mipmap.resize(frames);
}

void SpraymakerModel::setImage(vips::VImage image, std::string file, int mipmap, int frame)
{
    if (mipmap >= mipmaps || frame >= frames)
        return; // Dimensions changed during import/generation process

    images[mipmap][frame] = image;
    files [mipmap][frame] = file;

    emit selectedImageChanged(mipmap, frame);
}

void SpraymakerModel::importImage(const ImageInfo& imageInfo, int mipmap, int frame)
{
    emit progressInvalidated();

    // Resize to new frame count if necessary
    if (imageInfo.frames + frame > frames)
        setFrameCount(imageInfo.frames + frame);

    // Preview generation
    // TODO: Thread this
    auto previewInfo = ImageManager::makePreview(imageInfo);

    // Add each frame individually
    for(int frameOffset = 0; auto const& imageFrame : imageInfo.image)
    {
        setImage(imageFrame, imageInfo.file, mipmap, frame + frameOffset);
        setPreview(previewInfo.pixmap.at(frameOffset), mipmap, frame + frameOffset);

        if (mipmapPropagationMode == MipmapPropagationMode::FILL
         || mipmapPropagationMode == MipmapPropagationMode::NO_OVERWRITE)
        {
            for (int mipmapIndex = mipmap + 1; mipmapIndex < mipmaps; mipmapIndex++)
            {
                if (mipmapPropagationMode == MipmapPropagationMode::NO_OVERWRITE
                    && images[mipmapIndex][frame + frameOffset].is_null() == false)
                    continue;

                setImage(imageFrame, imageInfo.file, mipmapIndex, frame + frameOffset);
                setPreview(previewInfo.pixmap.at(frameOffset), mipmapIndex, frame + frameOffset);
            }
        }
        frameOffset++;
    }
}

void SpraymakerModel::copyImage(int fromMipmap, int fromFrame, int toMipmap, int toFrame)
{
    if (suppress && (fromMipmap == toMipmap && fromFrame == toFrame))
        return;

    setImage(images[fromMipmap][fromFrame], files[fromMipmap][fromFrame], toMipmap, toFrame);
    setPreview(previews[fromMipmap][fromFrame], toMipmap, toFrame);
}

void SpraymakerModel::importImages(const std::list<ImageInfo>& imageInfos, int mipmap, int frame)
{
    for(int frameIndex = frame; const auto& imageInfo : imageInfos)
    {
        importImage(imageInfo, mipmap, frameIndex);

        // Adjust to the new position depending on input file's frame count
        frameIndex += imageInfo.frames;

        // Add a frame if needed
        if (frameIndex > frames)
            setFrameCount(frameIndex);
    }
}

const vips::VImage* SpraymakerModel::getImage(int mipmap, int frame)
{
    if (images[mipmap][frame].is_null())
        return nullptr;

    return &images[mipmap][frame];
}

void SpraymakerModel::setPreview(QPixmap preview, int mipmap, int frame)
{
    if (mipmap >= mipmaps || frame >= frames)
        return; // Dimensions changed during import/generation process

    previews[mipmap][frame] = preview;

    emit previewChanged(preview, mipmap, frame);
}

void SpraymakerModel::setVtfFileSize(int vtfFileSize)
{
    if (suppress && this->vtfFileSize == vtfFileSize)
        return;

    this->vtfFileSize = vtfFileSize;

    emit vtfFileSizeChanged(vtfFileSize);
}

int SpraymakerModel::getVtfFileSize()
{ return vtfFileSize; }

void SpraymakerModel::setMaxVtfFileSize(int maxVtfFileSize)
{
    if (suppress && this->maxVtfFileSize == maxVtfFileSize)
        return;

    this->maxVtfFileSize = maxVtfFileSize;

    emit maxVtfFileSizeChanged(maxVtfFileSize);
    emit newVtfFileSizeNeeded();
    emit newResolutionNeeded();
}

int SpraymakerModel::getMaxVtfFileSize()
{ return maxVtfFileSize; }

void SpraymakerModel::setImageFormat(ImageFormat imageFormat)
{
    if (suppress && this->imageFormat == imageFormat)
        return;

    emit progressInvalidated();

    this->imageFormat = imageFormat;

    emit imageFormatChanged(imageFormat);
    emit newVtfFileSizeNeeded();
    emit newResolutionNeeded();
}

SpraymakerModel::ImageFormat SpraymakerModel::getFormat()
{ return imageFormat; }

SpraymakerModel::Formats SpraymakerModel::mapFormat()
{
    for(const auto& format : EnumMapper)
    {
        if (format.format == imageFormat)
            return format;
    }

    return Formats{};
}

void SpraymakerModel::setMaxResolution(int maxResolution)
{
    if (suppress && this->maxResolution == maxResolution)
        return;

    this->maxResolution = maxResolution;
    emit maxResolutionChanged(maxResolution);
}

int SpraymakerModel::getMaxResolution()
{ return maxResolution; }

void SpraymakerModel::setUseSimpleFormatNames(bool useSimpleFormatNames)
{
    if (suppress && this->useSimpleFormatNames == useSimpleFormatNames)
        return;

    this->useSimpleFormatNames = useSimpleFormatNames;
    emit useSimpleFormatNamesChanged(useSimpleFormatNames);
}

bool SpraymakerModel::getUseSimpleFormatNames()
{ return useSimpleFormatNames; }

void SpraymakerModel::setFormatComboBoxMappers(
    std::unordered_map<int, ImageFormat> comboboxIndexToFormatMap,
    std::unordered_map<ImageFormat, int> formatToComboBoxIndexMap)
{
    this->comboBoxIndexToFormatMap = comboboxIndexToFormatMap;
    this->formatToComboBoxIndexMap = formatToComboBoxIndexMap;

    emit formatComboBoxMappersChanged();
}

SpraymakerModel::ImageFormat SpraymakerModel::getFormatFromComboBoxIndex(int index)
{ return comboBoxIndexToFormatMap.contains(index) ? comboBoxIndexToFormatMap.at(index) : ImageFormat::INVALID; }

int SpraymakerModel::getComboBoxIndexFromFormat(ImageFormat format)
{ return formatToComboBoxIndexMap.contains(format) ? formatToComboBoxIndexMap.at(format) : -1; }

void SpraymakerModel::setTextureSampleMode(TextureSampleMode textureSampleMode)
{
    if (suppress && this->textureSampleMode == textureSampleMode)
        return;

    this->textureSampleMode = textureSampleMode;
    emit textureSampleModeChanged(textureSampleMode);
}

SpraymakerModel::TextureSampleMode SpraymakerModel::getTextureSampleMode()
{ return textureSampleMode; }

void SpraymakerModel::setResolutionInputMode(ResolutionInputMode resolutionInputMode)
{
    if (suppress && this->resolutionInputMode == resolutionInputMode)
        return;

    this->resolutionInputMode = resolutionInputMode;
    emit resolutionInputModeChanged(resolutionInputMode);
    emit newResolutionNeeded();
}

SpraymakerModel::ResolutionInputMode SpraymakerModel::getResolutionInputMode()
{ return resolutionInputMode; }

void SpraymakerModel::invalidateProgress()
{ emit progressInvalidated(); }

const QPixmap& SpraymakerModel::getPreview(int mipmap, int frame)
{ return previews[mipmap][frame]; }

void SpraymakerModel::setMipmapPropagationMode(MipmapPropagationMode mipmapPropagationMode)
{
    if (suppress && this->mipmapPropagationMode == mipmapPropagationMode)
        return;

    this->mipmapPropagationMode = mipmapPropagationMode;
    emit mipmapPropagationModeChanged(mipmapPropagationMode);
}

SpraymakerModel::MipmapPropagationMode SpraymakerModel::getMipmapPropagationMode()
{ return mipmapPropagationMode; }

std::string SpraymakerModel::getFile(int mipmap, int frame)
{ return files[mipmap][frame]; }

void SpraymakerModel::setAutocropMode(AutocropMode autocropMode)
{
    if (suppress && this->autocropMode == autocropMode)
        return;

    this->autocropMode = autocropMode;
    emit autocropModeChanged(autocropMode);
}

SpraymakerModel::AutocropMode SpraymakerModel::getAutocropMode()
{ return autocropMode; }

void SpraymakerModel::setBackground(int r, int g, int b, int a)
{
    setBackgroundRed(r);
    setBackgroundGreen(g);
    setBackgroundBlue(b);
    setBackgroundAlpha(a);
}

void SpraymakerModel::setBackgroundRed(int value)
{
    if (suppress && this->backgroundRed == value)
        return;

    this->backgroundRed = value;

    emit backgroundRedChanged(backgroundRed);
    emit backgroundColourChanged(backgroundRed, backgroundGreen, backgroundBlue, backgroundAlpha);
}

void SpraymakerModel::setBackgroundGreen(int value)
{
    if (suppress && this->backgroundGreen == value)
        return;

    this->backgroundGreen = value;

    emit backgroundGreenChanged(backgroundGreen);
    emit backgroundColourChanged(backgroundRed, backgroundGreen, backgroundBlue, backgroundAlpha);
}

void SpraymakerModel::setBackgroundBlue(int value)
{
    if (suppress && this->backgroundBlue == value)
        return;

    this->backgroundBlue = value;

    emit backgroundBlueChanged(backgroundBlue);
    emit backgroundColourChanged(backgroundRed, backgroundGreen, backgroundBlue, backgroundAlpha);
}

void SpraymakerModel::setBackgroundAlpha(int value)
{
    if (suppress && this->backgroundAlpha == value)
        return;

    this->backgroundAlpha = value;

    emit backgroundAlphaChanged(backgroundAlpha);
    emit backgroundColourChanged(backgroundRed, backgroundGreen, backgroundBlue, backgroundAlpha);
}

int SpraymakerModel::getBackgroundRed()
{ return backgroundRed; }

int SpraymakerModel::getBackgroundGreen()
{ return backgroundGreen; }

int SpraymakerModel::getBackgroundBlue()
{ return backgroundBlue; }

int SpraymakerModel::getBackgroundAlpha()
{ return backgroundAlpha; }
