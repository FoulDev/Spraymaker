/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QPixmap>
#include <QObject>

// glib, used by libvips, has its own signals
#pragma push_macro("signals")
#undef signals
#include <vips/vips8>
#pragma pop_macro("signals")

// ========== Return structs ==========

struct ImageInfo
{
    friend class ImageManager;

    int width;
    int height;
    int frames;
    std::string file;
    std::vector<vips::VImage> image;

protected:
    ImageInfo(std::string file,
              std::vector<vips::VImage> image)
        : width(image.front().width())
        , height(image.front().height())
        , frames(image.size())
        , file(file)
        , image(image)
    {}
};

struct PreviewInfo
{
    friend class ImageManager;

    std::string file;
    std::vector<QPixmap> pixmap;

protected:
    PreviewInfo(std::string file, std::vector<QPixmap> pixmap)
        : file(file)
        , pixmap(pixmap)
    {}
};

// ========== ImageManager ==========

class ImageManager : public QObject
{
    Q_OBJECT
public:
    static int previewResolution;

public slots:
    static const ImageInfo load(std::string file);
    static const PreviewInfo makePreview(const ImageInfo& imageInfo);

protected:
    static const ImageInfo vipsLoad(std::string file);
    static const ImageInfo ffmpegLoad(std::string file);
};

#endif // IMAGEMANAGER_H
