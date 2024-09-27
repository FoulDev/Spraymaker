/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DROPIMAGE_H
#define DROPIMAGE_H

#include "spraymakermodel.h"

#include <QTableWidget>
#include <QMetaEnum>
#include <QLabel>
#include <QReadWriteLock>

// ========== DropImageTable ==========

class DropImageTable : public QTableWidget
{
    Q_OBJECT
private:
    void fillCells();

    SpraymakerModel *spraymakerModel = nullptr;

public:
    explicit DropImageTable(QWidget *parent = nullptr);
    void setModel(SpraymakerModel *spraymakerModel);

public slots:
    void setMipmapCount(int mipmaps);
    void setFrameCount(int frames);
    void setDimensions(int mipmaps, int frames);
    void updateDefaultImage();
    void updateHeaders();

signals:
    void imageDropped(std::list<std::string> files, int mipmap, int frame);
    void newDefaultImageAvailable();
};

// ========== DropImageContainer ==========

class DropImageContainer : public QWidget
{
    Q_OBJECT
public:
    explicit DropImageContainer(int mipmap, int frame, QWidget *parent = nullptr);
    static void setup(int previewResolution, DropImageTable& dropImageTable);

public slots:
    void setPreviewImage(const QPixmap &image, int mipmap, int frame);
    void updateDefaultImage();

signals:
    void selectedImagesChanged(std::list<std::string> files, int mipmap, int frame);

protected:
    static QPixmap* defaultImage;
    static QReadWriteLock defaultImageMutex;
    static int previewResolution;

    static void asyncSetDefaultImage(QPixmap& newDefaultImage);
    static QPixmap* asyncGetDefaultImage();

    QLabel image;

    const int mipmap;
    const int frame;

    bool isValidDrop = false;
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
};

#endif // DROPIMAGE_H
