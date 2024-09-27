/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "dropimage.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QHeaderView>
#include <QPixmap>
#include <QTableWidget>
#include <QGridLayout>
#include <QReadWriteLock>
#include <QPainter>
#include <QThread>

// ========== DropImageTable ==========

DropImageTable::DropImageTable(QWidget *parent): QTableWidget(parent)
{
    setAutoScroll(false);
    setCornerButtonEnabled(false);

    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    horizontalHeader()->setSectionsClickable(false);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
    verticalHeader()->setSectionsClickable(false);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFocusPolicy(Qt::NoFocus);
    setSelectionMode(QAbstractItemView::NoSelection);
}

void DropImageTable::setModel(SpraymakerModel *spraymakerModel)
{
    this->spraymakerModel = spraymakerModel;
}

void DropImageTable::setMipmapCount(int mipmaps)
{
    if (mipmaps <= 0 || mipmaps == rowCount())
        return;

    setRowCount(mipmaps);
    fillCells();
}

void DropImageTable::setFrameCount(int frames)
{
    if (frames <= 0 || frames == columnCount())
        return;

    setColumnCount(frames);
    fillCells();
}

void DropImageTable::setDimensions(int mipmaps, int frames)
{
    setMipmapCount(mipmaps);
    setFrameCount(frames);
}

void DropImageTable::updateDefaultImage()
{
    emit newDefaultImageAvailable();
    resizeColumnsToContents();
    resizeRowsToContents();
}

void DropImageTable::fillCells()
{
    int mipmaps = spraymakerModel->getMipmapCount();
    int frames = spraymakerModel->getFrameCount();

    for (int mipmap = 0; mipmap < mipmaps; mipmap++)
    {
        for (int frame = 0; frame < frames; frame++)
        {
            if (cellWidget(mipmap, frame) == nullptr)
            {
                DropImageContainer *container = new DropImageContainer(mipmap, frame);
                // Propagate changes upward from DropImageContainers to DropImageTable
                connect(container, &DropImageContainer::selectedImagesChanged,
                        this,      &DropImageTable::imageDropped);

                // Update the default image once available
                connect(this,      &DropImageTable::newDefaultImageAvailable,
                        container, &DropImageContainer::updateDefaultImage);

                // Update preview image once available
                connect(spraymakerModel, &SpraymakerModel::previewChanged,
                        container,       &DropImageContainer::setPreviewImage);

                setCellWidget(mipmap, frame, container);

                // Propagate mipmaps downward when adding new rows
                if (mipmap > 0
                && (   spraymakerModel->getMipmapPropagationMode() == SpraymakerModel::MipmapPropagationMode::FILL
                    || spraymakerModel->getMipmapPropagationMode() == SpraymakerModel::MipmapPropagationMode::NO_OVERWRITE)
                    )
                {
                    if (spraymakerModel->getImage(mipmap - 1, frame) != nullptr)
                        spraymakerModel->copyImage(mipmap - 1, frame, mipmap, frame);
                }
            }
        }
    }

    verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    updateHeaders();
    resizeColumnsToContents();
    resizeRowsToContents();
}

void DropImageTable::updateHeaders()
{
    int mipmaps = spraymakerModel->getMipmapCount();
    int frames = spraymakerModel->getFrameCount();

    int width = spraymakerModel->getWidth();
    int height = spraymakerModel->getHeight();

    QStringList mipmapSizes;
    for (int mipmap = 0; mipmap < mipmaps; mipmap++)
    {
        mipmapSizes.push_back(QString(tr("Mipmap\n%1\nx\n%2"))
                                  .arg(width >> mipmap)
                                  .arg(height >> mipmap));
    }

    QStringList frameCounts;
    for (int frame = 0; frame < frames; frame++)
    {
        frameCounts.push_back(tr("Frame %1").arg(frame + 1));
    }

    setVerticalHeaderLabels(mipmapSizes);
    setHorizontalHeaderLabels(frameCounts);
}

// ========== DropImageContainer ==========

// Must construct a QGuiApplication before a QPixmap
QPixmap* DropImageContainer::defaultImage = nullptr;
QReadWriteLock DropImageContainer::defaultImageMutex;
int DropImageContainer::previewResolution;

void DropImageContainer::setup(int previewResolution, DropImageTable& dropImageTable)
{
    DropImageContainer::previewResolution = previewResolution;

    auto generateDefaultImage = [previewResolution, &dropImageTable]()
    {
        int r = previewResolution; // Square resolution
        int t = 8; // Border thickness
        int fontSize = 2;
        auto text = tr("DROP\nIMAGE\nHERE");
        auto newDefaultImage = new QPixmap(r, r);

        QPainter painter(newDefaultImage);
        painter.setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::TextAntialiasing);
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(newDefaultImage->rect(), Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.fillRect(newDefaultImage->rect(), QGradient::ShyRainbow);
        painter.fillRect(newDefaultImage->rect().adjusted(t, t, -t, -t), QGradient::WitchDance);

        // QFontMetrics doesn't consider newlines, so we must calculate it manually
        auto font = QFont("System", fontSize, QFont::Bold);
        QPixmap textOverlay(r, r);
        QPainter textPainter(&textOverlay);
        textPainter.setRenderHints(QPainter::RenderHint::Antialiasing | QPainter::RenderHint::TextAntialiasing);

        const auto lines = text.split("\n");
        while (true)
        {
            textPainter.setFont(font);
            auto height = textPainter.fontMetrics().height()
                        + textPainter.fontMetrics().lineSpacing() * (lines.count() - 1);

            bool hitBounds = false;
            for(const auto& line : lines)
            {
                auto width = textPainter.fontMetrics().horizontalAdvance(line);

                hitBounds |= width  >= textOverlay.rect().width()  - t*3
                          || height >= textOverlay.rect().height() - t*3;
            }

            if (hitBounds)
            {
                font.setPixelSize(--fontSize);
                break;
            }

            if (fontSize > r)
            {
                font.setPixelSize(32);
                break;
            }

            font.setPixelSize(++fontSize);
        }

        textPainter.setCompositionMode(QPainter::CompositionMode_Source);
        textPainter.fillRect(textOverlay.rect(), Qt::transparent);
        textPainter.setFont(font);
        textPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        textPainter.drawText(textOverlay.rect(), Qt::AlignCenter, text);
        textPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        textPainter.fillRect(textOverlay.rect(), QGradient::NightCall);
        textPainter.end();

        painter.drawImage(textOverlay.rect(), textOverlay.toImage());
        painter.end();

        asyncSetDefaultImage(*newDefaultImage);
        dropImageTable.updateDefaultImage();
    };

    auto thread = new QThread();
    auto threadObject = new QObject();

    threadObject->moveToThread(thread);

    connect(thread,       &QThread::started,
            threadObject, generateDefaultImage);

    connect(thread,       &QThread::finished,
            threadObject, &QObject::deleteLater);

    connect(thread,       &QThread::finished,
            thread,       &QThread::deleteLater);

    thread->start();
}

void DropImageContainer::asyncSetDefaultImage(QPixmap& newDefaultImage)
{
    bool lock = defaultImageMutex.tryLockForWrite(3000);
    if (lock)
    {
        defaultImage = &newDefaultImage;
        defaultImageMutex.unlock();
    }
    else
    {
        qDebug() << "Failed to obtain write lock on defaultImageMutex";
    }
}

QPixmap* DropImageContainer::asyncGetDefaultImage()
{
    QPixmap* result = nullptr;
    bool lock = defaultImageMutex.tryLockForRead(150);
    if (lock)
    {
        result = defaultImage;
        defaultImageMutex.unlock();
    }
    else
    {
        qDebug() << "Failed to obtain read lock on defaultImageMutex";
    }

    return result;
}

DropImageContainer::DropImageContainer(int mipmap, int frame, QWidget *parent)
    : QWidget(parent)
    , mipmap(mipmap)
    , frame(frame)
{
    setAcceptDrops(true);
    updateDefaultImage();

    QGridLayout *layout = new QGridLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(&image);
    setLayout(layout);
};

void DropImageContainer::updateDefaultImage()
{
    setFixedSize(DropImageContainer::previewResolution,
                 DropImageContainer::previewResolution);

    auto _defaultImage = asyncGetDefaultImage();
    if (_defaultImage != nullptr)
        image.setPixmap(*_defaultImage);
}

void DropImageContainer::dragEnterEvent(QDragEnterEvent *event)
{
    isValidDrop = false;

    if (event->mimeData()->urls().count() == 0)
        return;

    foreach(const QUrl url, event->mimeData()->urls())
    {
        if (url.isValid() == false || url.isLocalFile() == false)
            return;
    }

    isValidDrop = true;
    event->acceptProposedAction();
}

void DropImageContainer::dropEvent(QDropEvent *event)
{
    if (isValidDrop == false)
        return;

    event->acceptProposedAction();

    std::list<std::string> files;
    const auto urls = event->mimeData()->urls();
    for (const auto& file : urls)
    {
        files.push_back(file.toLocalFile().toStdString());
    }

    emit selectedImagesChanged(files, mipmap, frame);
}

void DropImageContainer::dragLeaveEvent(QDragLeaveEvent *event)
{
    isValidDrop = false;
}

void DropImageContainer::dragMoveEvent(QDragMoveEvent *event)
{
    if (isValidDrop)
        event->acceptProposedAction();
}

void DropImageContainer::setPreviewImage(const QPixmap &image, int mipmap, int frame)
{
    // Only accept previews intended for self
    if (this->mipmap == mipmap && this->frame == frame)
        this->image.setPixmap(image);
}
