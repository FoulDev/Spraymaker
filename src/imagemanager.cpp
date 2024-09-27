/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "imagemanager.h"
#include "spraymakerexception.h"
#include "imageloader_ffmpeg.h"

int ImageManager::previewResolution = 128;

const ImageInfo ImageManager::load(std::string file)
{
    std::string errors;

    // Attmept loading input with libvips
    try
    {
        return vipsLoad(file);
    }
    catch (const vips::VError &error)
    {
        errors.append("libvips:\n");
        errors.append(error.what());
    }

    // Attempt loading input with ffmpeg
    try
    {
        return ffmpegLoad(file);
    }
    catch (const std::exception& error)
    {
        errors.append("\nffmpeg:\n");
        errors.append(error.what());
    }

    // Failed to load, not a supported filetype.
    throw SpraymakerException(tr("%1 isn't a supported file type.")
                                  .arg(QString::fromStdString(file)),
                              QString::fromStdString(errors));
}

const PreviewInfo ImageManager::makePreview(const ImageInfo& imageInfo)
{
    std::vector<QPixmap> pixmaps;

    for(const auto& frame : imageInfo.image)
    {
        auto thumbnail =
            frame.thumbnail_image(ImageManager::previewResolution,
                                  vips::VImage::option()
                                      ->set("height", ImageManager::previewResolution)
                                      ->set("size", VipsSize::VIPS_SIZE_BOTH));

        const auto qimage = QImage((uchar*)thumbnail.data(),
                                   thumbnail.width(), thumbnail.height(), QImage::Format_RGBA8888);
        const auto pixmap = QPixmap(QPixmap::fromImage(qimage));

        pixmaps.push_back(pixmap);
    }

    return PreviewInfo(imageInfo.file, pixmaps);
}

const ImageInfo ImageManager::vipsLoad(std::string file)
{
    auto image = vips::VImage::new_from_file(
        file.c_str(),
        vips::VImage::option()
            ->set("access", VIPS_ACCESS_RANDOM)
            ->set("n", -1)
            ->set("autorotate",  true));

    bool hasFrames = image.get_typeof("page-height") != 0 && image.get_typeof("n-pages") != 0;

    // Ensure RGBA pixel format
    image = image.colourspace(VipsInterpretation::VIPS_INTERPRETATION_sRGB);
    if (image.bands() == 3)
        image = image.bandjoin(255);

    if (hasFrames == false)
    {
        auto images = std::vector<vips::VImage>{image};
        return ImageInfo(file, images);
    }

    auto width = image.width();
    auto pageHeight = image.get_int("page-height");
    auto frames = image.height() / pageHeight;
    auto images = std::vector<vips::VImage>();

    for(int frame = 0; frame < frames; frame++)
    {
        images.push_back(image.crop(0, frame * pageHeight, width, pageHeight));
    }

    return ImageInfo(file, images);
}

const ImageInfo ImageManager::ffmpegLoad(std::string file)
{
    ImageLoaderFfmpeg ihav(file.c_str());
    auto frames = ihav.getFrames();
    auto images = std::vector<vips::VImage>();

    for(RGBAFrame frame : frames)
    {
        auto image = vips::VImage::new_from_memory(
            frame.buffer, frame.size, frame.width, frame.height, 4,
            VipsBandFormat::VIPS_FORMAT_UCHAR);
        images.push_back(image);
    }

    if (images.empty())
        throw SpraymakerException(tr("File contained no image data."));

    return ImageInfo(file, images);
}
