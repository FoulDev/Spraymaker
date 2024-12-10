/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "spraymaker.h"
#include "dropimage.h"
#include "imagehelper.h"
#include "sizedisplaylabel.h"
#include "spraymakerexception.h"
#include "vtf_defs.h"
#include "gamespray.h"
#include "settings.h"

#include <crnlib.h>
#include <crnlib/crn_mipmapped_texture.h>

#include <iostream>
#include <fstream>

#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QProgressBar>
#include <QStyleFactory>
#include <QStringListModel>
#include <QLineEdit>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardItemModel>
#include <QInputDialog>
#include <QDir>
#include <QTreeWidget>
#include <QDirIterator>
#include <QSaveFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QPainterPath>
#include <QPainter>

extern "C"
{
#include <libavutil/version.h>
#include <libavcodec/version.h>
#include <libavformat/version.h>
#include <libavdevice/version.h>
#include <libavfilter/version.h>
#include <libswscale/version.h>
#include <libswresample/version.h>
#include <libpostproc/version.h>
}

#include "./ui_spraymaker.h"

Spraymaker* Spraymaker::instance = nullptr;

Spraymaker::Spraymaker(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Spraymaker)
    , spraymakerModel(new SpraymakerModel)
    , settings(Settings::getInstance())
{
    if (Spraymaker::instance != nullptr)
        throw std::runtime_error("Tried to create multiple instances of Spraymaker");
    Spraymaker::instance = this;

    setAttribute(Qt::WA_DeleteOnClose); // Call destructors on exit

    // Verify logodirs from settings still exist
    {
        auto logodirs = settings->getLogoDirs();

        for(const auto &logodir : std::as_const(logodirs))
        {
            auto status = tryAddGame(logodir);

            if (status == TryAddGameResult::LogosDirNotFound)
            {
                settings->removeLogoDir(logodir);
                qWarning() << "Failed adding logodir " + logodir + " from settings.";
            }
        }
    }

    ui->setupUi(this);

    spraymakerModel->beginSetup();
    spraymakerModel->setMaxResolution(crn_limits::cCRNMaxLevelResolution);
    spraymakerModel->setUseSimpleFormatNames(settings->getUseSimpleFormats());

    ui->dropImageTable->setModel(spraymakerModel);

    ImageManager::previewResolution = settings->getPreviewResolution();
    DropImageContainer::setup(settings->getPreviewResolution(), *ui->dropImageTable);

    // ========== Status bar progress meters ==========
    {
        QWidget *imageProcessingContainer = new QWidget();

        // Fusion style is set here as a bugfix for text not displaying inside the progress bar correctly
        imageProgressBar = new QProgressBar();
        imageProgressBar->setStyle(QStyleFactory::create("fusion"));

        imageProgressBar->setRange(0, 1);
        imageProgressBar->setValue(0);
        imageProgressBar->setFormat(tr("Image %v / %m"));

        encodingProgressBar = new QProgressBar();
        encodingProgressBar->setStyle(QStyleFactory::create("fusion"));

        encodingProgressBar->setRange(0, 100);
        encodingProgressBar->setValue(0);
        encodingProgressBar->setFormat(tr("Encoding: %p%"));

        QHBoxLayout *imageProcessingLayout = new QHBoxLayout();
        imageProcessingLayout->setContentsMargins(0, 0, 0, 0);
        imageProcessingLayout->addWidget(imageProgressBar, 0);
        imageProcessingLayout->addWidget(encodingProgressBar, 0);

        imageProcessingContainer->setLayout(imageProcessingLayout);

        ui->statusbar->addPermanentWidget(imageProcessingContainer);
    }

    // ========== Connections ==========

    // Spinboxes -> SpraymakerModel
    // Update model data from user inputs
    connect(ui->mipmapSpinbox, &QSpinBox::valueChanged,
            spraymakerModel,   &SpraymakerModel::setMipmapCount);
    connect(ui->frameSpinbox,  &QSpinBox::valueChanged,
            spraymakerModel,   &SpraymakerModel::setFrameCount);
    connect(ui->widthSpinBox,  &QSpinBox::valueChanged,
            spraymakerModel,   &SpraymakerModel::setWidth);
    connect(ui->heightSpinBox, &QSpinBox::valueChanged,
            spraymakerModel,   &SpraymakerModel::setHeight);

    // SpraymakerModel -> Spinboxes
    // Update spinbox displays when model's values change
    connect(spraymakerModel,   &SpraymakerModel::mipmapCountChanged,
            ui->mipmapSpinbox, &QSpinBox::setValue);
    connect(spraymakerModel,   &SpraymakerModel::frameCountChanged,
            ui->frameSpinbox,  &QSpinBox::setValue);
    connect(spraymakerModel,   &SpraymakerModel::widthChanged,
            ui->widthSpinBox,  &QSpinBox::setValue);
    connect(spraymakerModel,   &SpraymakerModel::heightChanged,
            ui->heightSpinBox, &QSpinBox::setValue);

    // Set maximum allowed resolution
    connect(spraymakerModel,   &SpraymakerModel::maxResolutionChanged,
            ui->widthSpinBox,  &QSpinBox::setMaximum);
    connect(spraymakerModel,   &SpraymakerModel::maxResolutionChanged,
            ui->heightSpinBox, &QSpinBox::setMaximum);

    // Set maximum number of mipmaps given the current resolution
    connect(spraymakerModel,   &SpraymakerModel::resolutionChanged,
            ui->mipmapSpinbox, [=, this](int width, int height){
        auto mipmaps = ImageHelper::getMaxMipmaps(width, height);
        spraymakerModel->setMaxMipmapCount(mipmaps);

        auto mipmapInputMode = spraymakerModel->getMipmapInputMode();

        if (mipmapInputMode == SpraymakerModel::MipmapInputMode::MAX_ONLY)
            spraymakerModel->setMipmapCount(mipmaps);
    });

    // Update the mipmap spinbox's maximum mipmap count
    connect(spraymakerModel,   &SpraymakerModel::maxMipmapCountChanged,
            ui->mipmapSpinbox, &QSpinBox::setMaximum);

    // MipmapsCheckbox -> Model
    connect(ui->mipmapsCheckbox, &QCheckBox::checkStateChanged,
            spraymakerModel,     [=, this](Qt::CheckState state){
        switch(state) {
        case Qt::CheckState::Unchecked:
            spraymakerModel->setMipmapInputMode(SpraymakerModel::MipmapInputMode::ONE_ONLY);
            break;
        case Qt::CheckState::PartiallyChecked:
            spraymakerModel->setMipmapInputMode(SpraymakerModel::MipmapInputMode::MAX_ONLY);
            break;
        case Qt::CheckState::Checked:
        default:
            spraymakerModel->setMipmapInputMode(SpraymakerModel::MipmapInputMode::USER_OPTION);
            break;
        }
    });

    // Change the MipmapCountSpinbox to match the input mode
    connect(spraymakerModel,   &SpraymakerModel::mipmapInputModeChanged,
            ui->mipmapSpinbox, [=, this](SpraymakerModel::MipmapInputMode mipmapInputMode) {
        switch(mipmapInputMode)
        {
        case SpraymakerModel::MipmapInputMode::MAX_ONLY:
            ui->mipmapSpinbox->setEnabled(false);
            spraymakerModel->setMipmapCount(spraymakerModel->getMaxMipmapCount());
            break;
        case SpraymakerModel::MipmapInputMode::ONE_ONLY:
            ui->mipmapSpinbox->setEnabled(false);
            spraymakerModel->setMipmapCount(1);
            break;
        case SpraymakerModel::MipmapInputMode::USER_OPTION:
        default:
            ui->mipmapSpinbox->setEnabled(true);
            break;
        }
    });

    // SpraymakerModel -> VTF size label
    {
        auto fileSizeLabel = new SizeDisplayLabel();

        connect(spraymakerModel, &SpraymakerModel::vtfFileSizeChanged,
                fileSizeLabel,   &SizeDisplayLabel::setFileSize);
        connect(spraymakerModel, &SpraymakerModel::maxVtfFileSizeChanged,
                fileSizeLabel,   &SizeDisplayLabel::setMaxFileSize);

        ui->statusbar->addWidget(fileSizeLabel);
    }

    // ========== Image format combobox ==========
    {
        auto imageFormatsModel = new QStringListModel();
        ui->imageFormatComboBox->setModel(imageFormatsModel);

        // Create bidirectional map between combo box index and image format enum.
        // TODO: Implement this with a ProxyModel/View instead?

        auto updateImageFormatsComboBoxMapper = [=, this](bool useSimpleFormats){
            QStringList formats;
            auto comboBoxIndexToFormatMap = std::unordered_map<int, SpraymakerModel::ImageFormat>();
            auto formatToComboBoxIndexMap = std::unordered_map<SpraymakerModel::ImageFormat, int>();
            int defaultIndex = 0;
            for(int i = 0; const auto& format : spraymakerModel->EnumMapper)
            {
                // Format isn't implemented
                if (format.format == SpraymakerModel::ImageFormat::INVALID)
                    continue;

                // Not implemented or isn't correct/good enough
                if (format.hide == true)
                    continue;

                if (useSimpleFormats)
                {
                    if (format.isSimple)
                    {
                        formats.append(format.simpleName);
                        comboBoxIndexToFormatMap.insert({i, format.format});
                        formatToComboBoxIndexMap.insert({format.format, i});
                        if (format.format == SpraymakerModel::ImageFormat::DXT1A)
                            defaultIndex = i;
                        i++;
                    }
                }
                else
                {
                    formats.append(format.realName);
                    comboBoxIndexToFormatMap.insert({i, format.format});
                    formatToComboBoxIndexMap.insert({format.format, i});
                    if (format.format == SpraymakerModel::ImageFormat::DXT1A)
                        defaultIndex = i;
                    i++;
                }
            }

            auto oldFormat = spraymakerModel->getFormatFromComboBoxIndex(ui->imageFormatComboBox->currentIndex());

            imageFormatsModel->setStringList(formats);
            spraymakerModel->setFormatComboBoxMappers(comboBoxIndexToFormatMap, formatToComboBoxIndexMap);

            int newIndex = spraymakerModel->getComboBoxIndexFromFormat(oldFormat);
            if (oldFormat != SpraymakerModel::ImageFormat::INVALID && newIndex >= 0)
            {
                defaultIndex = newIndex;
            }

            ui->imageFormatComboBox->setCurrentIndex(defaultIndex);
        };

        // Switch between simple/advanced mode format selection
        connect(spraymakerModel,         &SpraymakerModel::useSimpleFormatNamesChanged,
                ui->imageFormatComboBox, updateImageFormatsComboBoxMapper);

        // Set and save format mode setting
        connect(spraymakerModel,         &SpraymakerModel::useSimpleFormatNamesChanged,
                settings,                &Settings::setUseSimpleFormats);

        // Make the text not get truncated on the image format combobox dropdown menu
        {
            auto view = ui->imageFormatComboBox->view();
            view->setTextElideMode(Qt::TextElideMode::ElideNone);
            auto fixViewWidth = [=, this](){
                // Find the longest line of text
                int width = 32;
                for (int i = 0; i < ui->imageFormatComboBox->count(); i++)
                {
                    auto text = ui->imageFormatComboBox->itemText(i);
                    width = std::max(width, ui->imageFormatComboBox->fontMetrics().horizontalAdvance(text));
                }
                view->setMinimumWidth(width);
            };
            fixViewWidth();

            connect(imageFormatsModel,       &QStringListModel::modelReset,
                    ui->imageFormatComboBox, fixViewWidth);
        }
    }

    // Image Format CheckBox (advanced mode)
    connect(ui->imageFormatCheckBox, &QCheckBox::checkStateChanged,
            spraymakerModel,         [=, this](Qt::CheckState checkState){
        spraymakerModel->setUseSimpleFormatNames(checkState == Qt::CheckState::Unchecked);
    });

    // Update checkbox if model changes
    connect(spraymakerModel,         &SpraymakerModel::useSimpleFormatNamesChanged,
            ui->imageFormatCheckBox, [=, this](bool useSimpleFormatNames){
        ui->imageFormatCheckBox->setChecked(!useSimpleFormatNames);
    });

    // Mipmap fade mode combo box
    // Hardcoded index because simple.
    connect(ui->fadeModeComboBox, &QComboBox::currentIndexChanged,
            spraymakerModel,      [=, this](int index){
        SpraymakerModel::TextureSampleMode sampleMode;

        switch(index)
        {
        case 1:
            sampleMode = SpraymakerModel::TextureSampleMode::NONE;
            break;
        case 2:
            sampleMode = SpraymakerModel::TextureSampleMode::POINT_SAMPLE;
            break;
        case 0:
        default:
            sampleMode = SpraymakerModel::TextureSampleMode::ANISOTROPIC;
            break;
        }

        spraymakerModel->setTextureSampleMode(sampleMode);
    });

    connect(spraymakerModel,      &SpraymakerModel::textureSampleModeChanged,
            ui->fadeModeComboBox, [=, this](SpraymakerModel::TextureSampleMode textureSampleMode){
        int index;
        switch(textureSampleMode)
        {
        case SpraymakerModel::TextureSampleMode::NONE:
            index = 1;
            break;
        case SpraymakerModel::TextureSampleMode::POINT_SAMPLE:
            index = 2;
            break;
        default:
        case SpraymakerModel::TextureSampleMode::ANISOTROPIC:
            index = 0;
            break;
        }

        ui->fadeModeComboBox->setCurrentIndex(index);
    });

    // TODO: Maybe a separate "pixel art" checkbox which disables the combobox would make more sense
    // Disable POINT_SAMPLE aka pixel art texture sampling if mipmaps are enabled
    connect(spraymakerModel, &SpraymakerModel::mipmapCountChanged,
            ui->fadeModeComboBox, [=, this](){
        auto model = (QStandardItemModel*)ui->fadeModeComboBox->model();
        if(spraymakerModel->getMipmapCount() > 1)
        {
            if(spraymakerModel->getTextureSampleMode() == SpraymakerModel::TextureSampleMode::POINT_SAMPLE)
                spraymakerModel->setTextureSampleMode(SpraymakerModel::TextureSampleMode::ANISOTROPIC);

            model->item(2)->setEnabled(false);
        }
        else
        {
            model->item(2)->setEnabled(true);
        }
    });

    // Set selected image format based on combo box index
    connect(ui->imageFormatComboBox, &QComboBox::currentIndexChanged,
            spraymakerModel,         [=, this](int index){
        auto format = spraymakerModel->getFormatFromComboBoxIndex(index);
        spraymakerModel->setImageFormat(format);
    });

    // Set combo box index based on selected image format
    connect(spraymakerModel,         &SpraymakerModel::imageFormatChanged,
            ui->imageFormatComboBox, [=, this](SpraymakerModel::ImageFormat format){
        auto index = spraymakerModel->getComboBoxIndexFromFormat(format);
        ui->imageFormatComboBox->setCurrentIndex(index);
    });

    // Propagate dropped image(s) and frame(s)
    connect(ui->dropImageTable, &DropImageTable::imageDropped,
            spraymakerModel,    [=, this](std::list<std::string> files, int mipmap, int frame){
        std::list<ImageInfo> imageInfos;
        for (const auto& file : files)
        {
            auto imageInfo = ImageManager::load(file);
            imageInfos.push_back({imageInfo});
        }

        spraymakerModel->importImages(imageInfos, mipmap, frame);
    });

    // SpraymakerModel -> DropImageTable
    connect(spraymakerModel,    &SpraymakerModel::mipmapCountChanged,
            ui->dropImageTable, &DropImageTable::setMipmapCount);
    connect(spraymakerModel,    &SpraymakerModel::frameCountChanged,
            ui->dropImageTable, &DropImageTable::setFrameCount);

    // SpraymakerModel -> Progress bars
    // Update total image count to mipmaps*frames
    connect(spraymakerModel,  &SpraymakerModel::dimensionsChanged,
            imageProgressBar, [=, this](int mipmaps, int frames) {
        imageProgressBar->setMaximum(mipmaps * frames);
    });

    // Reset progress bars to 0 if any invalidating change is made
    connect(spraymakerModel,     &SpraymakerModel::progressInvalidated,
            imageProgressBar,    [=, this](){ imageProgressBar->setValue(0); });
    connect(spraymakerModel,     &SpraymakerModel::progressInvalidated,
            encodingProgressBar, [=, this](){ encodingProgressBar->setValue(0); });

    // Calculate VTF file size
    // Update the calculated total size of the resulting VTF file
    connect(spraymakerModel, &SpraymakerModel::newVtfFileSizeNeeded,
            spraymakerModel, [=, this]() {
        int vtfSize = ImageHelper::getImageDataSize(
            spraymakerModel->getFormat(),
            spraymakerModel->getWidth(),
            spraymakerModel->getHeight(),
            spraymakerModel->getMipmapCount(),
            spraymakerModel->getFrameCount());
        vtfSize += sizeof(VTF_HEADER_71);
        spraymakerModel->setVtfFileSize(vtfSize);
    });

    // Update resolution input mode to enable/disable setting the resolution automatically
    connect(ui->resolutionGroupBox, &QGroupBox::toggled,
            spraymakerModel,        [=, this](bool checked){
        // TODO: Tristate for ::FREE
        spraymakerModel->setResolutionInputMode(
            checked
                ? SpraymakerModel::ResolutionInputMode::MANUAL
                : SpraymakerModel::ResolutionInputMode::AUTOMATIC);
    });

    // Custom stepping setting
    {
        auto getCustomStepMode = [=, this]() {
            if (spraymakerModel->getResolutionInputMode() != SpraymakerModel::ResolutionInputMode::MANUAL)
                return CustomStepSpinBox::StepMode::SingleStep;

            if (ImageHelper::isDxt(spraymakerModel->getFormat()))
            {
                if (spraymakerModel->getMipmapCount() > 1)
                {
                    // Mipmapped DXT textures must be a power of two or will otherwise look glichty
                    return CustomStepSpinBox::StepMode::PowerOfTwo;
                }
                // All DXT textures must be a multiple of four
                return CustomStepSpinBox::StepMode::MultipleOfFour;
            }
            // Uncompressed textures can be any resolution
            return CustomStepSpinBox::StepMode::SingleStep;
        };

        // Update the resolution inputs' modes to fit the input mode
        connect(spraymakerModel, &SpraymakerModel::resolutionInputModeChanged,
                this,            [=, this](){
            ui->widthSpinBox->setCustomStep(getCustomStepMode());
            ui->heightSpinBox->setCustomStep(getCustomStepMode());
        });

        // Mipmap count affects the stepping mode for DXT images
        connect(spraymakerModel, &SpraymakerModel::mipmapCountChanged,
                this,            [=, this](){
            ui->widthSpinBox->setCustomStep(getCustomStepMode());
            ui->heightSpinBox->setCustomStep(getCustomStepMode());
        });

        // Changes to image format may change the stepping mode
        connect(spraymakerModel, &SpraymakerModel::imageFormatChanged,
                this,            [=, this](){
            ui->widthSpinBox->setCustomStep(getCustomStepMode());
            ui->heightSpinBox->setCustomStep(getCustomStepMode());
        });
    }

    // Update the width/height to the highest possible if set in automatic mode
    connect(spraymakerModel, &SpraymakerModel::newResolutionNeeded,
            spraymakerModel, [=, this](){
        if (spraymakerModel->getResolutionInputMode() != SpraymakerModel::ResolutionInputMode::AUTOMATIC)
            return;

        uint width;
        uint height;
        uint mipmaps = spraymakerModel->getMipmapCount();
        uint step = 1;
        bool square = false;
        bool powerOf2 = false;

        if (ImageHelper::isDxt(spraymakerModel->getFormat()))
        {
            step = 4;
            if (spraymakerModel->getMipmapCount() > 1)
            {
                powerOf2 = true;
                square = true; // Assuming 1024x512 is not a desired resolution
            }
        }

        if (spraymakerModel->getMipmapInputMode() == SpraymakerModel::MipmapInputMode::MAX_ONLY)
            mipmaps = 0; // Special case will recalculate resolution based on changing mipmap count

        ImageHelper::getMaxResForTargetSize(
            spraymakerModel->getFormat(),
            width, height,
            mipmaps,
            spraymakerModel->getFrameCount(),
            spraymakerModel->getMaxVtfFileSize() - sizeof(VTF_HEADER_71),
            step, square, powerOf2);

        spraymakerModel->setResolution(width, height);
    });

    // Toggle autocrop
    connect(spraymakerModel,      &SpraymakerModel::autocropModeChanged,
            ui->autocropCheckBox, [=, this](SpraymakerModel::AutocropMode autocropMode){
        // TODO: Is it worthwhile to implement the other autocrop modes user-facing?
        ui->autocropCheckBox->setChecked(autocropMode == SpraymakerModel::AutocropMode::AUTOMATIC);
    });

    connect(ui->autocropCheckBox, &QCheckBox::checkStateChanged,
            spraymakerModel,      [=, this](Qt::CheckState state){
        spraymakerModel->setAutocropMode(state == Qt::Checked
                                             ? SpraymakerModel::AutocropMode::AUTOMATIC
                                             : SpraymakerModel::AutocropMode::NONE);
    });

    // Background colour selectors
    connect(ui->redSlider,   &QSlider::sliderMoved,
            spraymakerModel, &SpraymakerModel::setBackgroundRed);
    connect(ui->greenSlider,  &QSlider::sliderMoved,
            spraymakerModel, &SpraymakerModel::setBackgroundGreen);
    connect(ui->blueSlider, &QSlider::sliderMoved,
            spraymakerModel, &SpraymakerModel::setBackgroundBlue);
    connect(ui->alphaSlider, &QSlider::sliderMoved,
            spraymakerModel, &SpraymakerModel::setBackgroundAlpha);

    connect(spraymakerModel, &SpraymakerModel::backgroundRedChanged,
            ui->redSlider,   &QSlider::setSliderPosition);
    connect(spraymakerModel, &SpraymakerModel::backgroundGreenChanged,
            ui->greenSlider, &QSlider::setSliderPosition);
    connect(spraymakerModel, &SpraymakerModel::backgroundBlueChanged,
            ui->blueSlider,  &QSlider::setSliderPosition);
    connect(spraymakerModel, &SpraymakerModel::backgroundAlphaChanged,
            ui->alphaSlider, &QSlider::setSliderPosition);

    // Background colour preview
    connect(spraymakerModel, &SpraymakerModel::backgroundColourChanged,
            ui->colourPreviewLabel, [=, this](int r, int g, int b, int a) {
        int w = 32;
        int h = 100;
        // TODO: This needs to happen after the UI is fully instantiated
        //int h = ui->redSlider->height() * 4 + ui->backgroundColourBox->layout()->spacing() * 3;

        auto pixmap = new QPixmap(w, h);
        QPainter painter(pixmap);

        int start = 77;
        for(int bx = 0; bx < w; bx += 8)
        {
            start = start == 77 ? 128 : 77;
            int flopper = start;
            for(int by = 0; by < h; by += 8)
            {
                flopper = flopper == 77 ? 128 : 77;
                painter.fillRect(bx, by, 8, 8, QColor(flopper, flopper, flopper, 255));
            }
        }

        QPainterPath bottomRight;
        bottomRight.setFillRule(Qt::FillRule::WindingFill);
        bottomRight.addPolygon(QPolygon(QList<QPoint> {{ w, 0 }, { w, h }, { 0, h }, { w, 0 } }));

        QPainterPath topLeft;
        topLeft.setFillRule(Qt::FillRule::WindingFill);
        topLeft.addPolygon(QPolygon(QList<QPoint>     {{ 0, 0 }, { w, 0 }, { 0, h }, { 0, 0 } }));

        painter.fillPath(bottomRight, QColor(r, g, b, a));
        painter.fillPath(topLeft,     QColor(r, g, b, 255));

        painter.end();

        ui->colourPreviewLabel->setPixmap(*pixmap);
    });

    // Save button
    connect(ui->saveSprayButton, &QPushButton::clicked,
            this,                &Spraymaker::saveSpray);

    auto saveEnableToggler = [=, this](){
        for(int mipmap = 0; mipmap < spraymakerModel->getMipmapCount(); mipmap++)
        {
            for(int frame = 0; frame < spraymakerModel->getFrameCount(); frame++)
            {
                bool filled = spraymakerModel->getImage(mipmap, frame) != nullptr;
                if (filled == false)
                {
                    ui->saveSprayButton->setEnabled(false);
                    return;
                }
            }
        }
        ui->saveSprayButton->setEnabled(true);
    };

    // Toggle the save button enabled/disabled
    connect(spraymakerModel, &SpraymakerModel::mipmapCountChanged,
            this,            saveEnableToggler);

    connect(spraymakerModel, &SpraymakerModel::frameCountChanged,
            this,            saveEnableToggler);

    connect(spraymakerModel, &SpraymakerModel::selectedImageChanged,
            this,            saveEnableToggler);

    connect(spraymakerModel, &SpraymakerModel::mipmapCountChanged,
            this,            saveEnableToggler);

    // Update the table headers to match new mipmap/frame/resolution
    connect(spraymakerModel,    &SpraymakerModel::mipmapCountChanged,
            ui->dropImageTable, &DropImageTable::updateHeaders);

    connect(spraymakerModel,    &SpraymakerModel::frameCountChanged,
            ui->dropImageTable, &DropImageTable::updateHeaders);

    connect(spraymakerModel,    &SpraymakerModel::resolutionChanged,
            ui->dropImageTable, &DropImageTable::updateHeaders);

    // About dialog box
    connect(ui->actionSpraymaker, &QAction::triggered,
            this,                 &Spraymaker::aboutDialog);

    // Make the model broadcast all the signals out
    spraymakerModel->finishSetup();
}

Spraymaker::~Spraymaker()
{
    settings->save();
    delete spraymakerModel;
    delete ui;
}

Spraymaker* Spraymaker::getInstance()
{
    if (Spraymaker::instance == nullptr)
        Spraymaker::instance = new Spraymaker();

    return Spraymaker::instance;
}

bool Spraymaker::crnProgressCallback(uint percentage_complete, void* pUser_data_ptr)
{    
    Spraymaker::instance->encodingProgressBar->setValue(percentage_complete);
    return true;
}

Spraymaker::TryAddGameResult Spraymaker::tryAddGame(QString directory)
{
    try
    {
        GameSpray gameSpray(directory);
        gamesWithSprays.push_back(gameSpray);
        if (settings->addLogoDir(gameSpray.getLogosDirectory()))
            return TryAddGameResult::Success;
        else
            return TryAddGameResult::Duplicate;
    } catch (const SpraymakerException&)
    {
        return TryAddGameResult::LogosDirNotFound;
    }
}

void Spraymaker::aboutDialog()
{
    QDialog about(this);
    about.setModal(true);
    about.setSizeGripEnabled(false);
    about.setWindowTitle(tr("About Spraymaker"));

    auto licenseBox = [](const QString file){
        QFile reader(file);
        reader.open(QFile::ReadOnly);
        auto content = reader.readAll();
        reader.close();
        auto box = new QTextEdit();
        box->setReadOnly(true);
        box->setFontFamily("Monospace");
        box->setMinimumWidth(box->fontMetrics().horizontalAdvance(QString("X").repeated(120)) + 8);
        box->setMinimumHeight(box->fontMetrics().lineSpacing() * 20);
        box->setPlainText(content);
        box->setSizeAdjustPolicy(QTextEdit::SizeAdjustPolicy::AdjustToContents);
        box->adjustSize();
        return box;
    };

    auto licenseTab = [=, this](const QString headline, const QString licenseFile){
        auto tab = new QWidget();
        auto tabLayout = new QVBoxLayout();
        tabLayout->addWidget(new QLabel(headline));
        tabLayout->addWidget(licenseBox(licenseFile));
        tab->setLayout(tabLayout);
        return tab;
    };

    auto crnlibVersion = QString("%1.%2").arg(CRNLIB_VERSION / 100U).arg(CRNLIB_VERSION % 100U);

    auto ffmpegVersions = QString("libavutil %1.%2.%3")
                              .arg(LIBAVUTIL_VERSION_MAJOR)
                              .arg(LIBAVUTIL_VERSION_MINOR)
                              .arg(LIBAVUTIL_VERSION_MICRO);
    ffmpegVersions.append(QString("\nlibavcodec %1.%2.%3")
                              .arg(LIBAVCODEC_VERSION_MAJOR)
                              .arg(LIBAVCODEC_VERSION_MINOR)
                              .arg(LIBAVCODEC_VERSION_MICRO));
    ffmpegVersions.append(QString("\nlibavformat %1.%2.%3")
                              .arg(LIBAVFORMAT_VERSION_MAJOR)
                              .arg(LIBAVFORMAT_VERSION_MINOR)
                              .arg(LIBAVFORMAT_VERSION_MICRO));
    ffmpegVersions.append(QString("\nlibavdevice %1.%2.%3")
                              .arg(LIBAVDEVICE_VERSION_MAJOR)
                              .arg(LIBAVDEVICE_VERSION_MINOR)
                              .arg(LIBAVDEVICE_VERSION_MICRO));
    ffmpegVersions.append(QString("\nlibavfilter %1.%2.%3")
                              .arg(LIBAVFILTER_VERSION_MAJOR)
                              .arg(LIBAVFILTER_VERSION_MINOR)
                              .arg(LIBAVFILTER_VERSION_MICRO));
    ffmpegVersions.append(QString("\nlibswscale %1.%2.%3")
                              .arg(LIBSWSCALE_VERSION_MAJOR)
                              .arg(LIBSWSCALE_VERSION_MINOR)
                              .arg(LIBSWSCALE_VERSION_MICRO));
    ffmpegVersions.append(QString("\nlibswresample %1.%2.%3")
                              .arg(LIBSWRESAMPLE_VERSION_MAJOR)
                              .arg(LIBSWRESAMPLE_VERSION_MINOR)
                              .arg(LIBSWRESAMPLE_VERSION_MICRO));
    ffmpegVersions.append(QString("\nlibpostproc %1.%2.%3")
                              .arg(LIBPOSTPROC_VERSION_MAJOR)
                              .arg(LIBPOSTPROC_VERSION_MINOR)
                              .arg(LIBPOSTPROC_VERSION_MICRO));

    auto licenses = new QTabWidget();
    licenses->addTab(licenseTab(QString("Spraymaker %1.%2.%3")
                                    .arg(SPRAYMAKER_VERSION_MAJOR)
                                    .arg(SPRAYMAKER_VERSION_MINOR)
                                    .arg(SPRAYMAKER_VERSION_PATCH),
                                ":/assets/licenses/spraymaker.txt"),
                     "Spraymaker");
    licenses->addTab(licenseTab(QString("Qt %1").arg(qVersion()),
                                ":/assets/licenses/qt.txt"),
                     "Qt");
    licenses->addTab(licenseTab(QString("crnlib %1").arg(crnlibVersion),
                                ":/assets/licenses/crnlib.txt"),
                     "crnlib");
    licenses->addTab(licenseTab(QString("libvips %1").arg(vips_version_string()),
                                ":/assets/licenses/libvips.txt"),
                     "libvips");
    licenses->addTab(licenseTab(ffmpegVersions,
                                ":/assets/licenses/ffmpeg.md"),
                     "FFmpeg");

    auto aboutLayout = new QVBoxLayout();
    aboutLayout->addWidget(licenses);

    about.setLayout(aboutLayout);

    about.exec();
}

QString Spraymaker::sprayNamePrompt()
{
    QDir dir;
    if (dir.mkpath("./sprays") == false)
        throw SpraymakerException(tr("Failed to create ./sprays directory."));

    auto lineEdit = new QLineEdit();
    lineEdit->setPlaceholderText(tr("Accepts: a-z, 0-9, -, _"));
    lineEdit->setMinimumWidth(lineEdit->fontMetrics().horizontalAdvance("xxxxxxxx00000000____----"));

    auto saveButton = new QPushButton(tr("Save"));
    saveButton->setEnabled(false);
    saveButton->setDefault(true);
    auto overwriteCheckbox = new QCheckBox(tr("Overwrite"));
    auto cancelButton = new QPushButton(tr("Cancel"));
    auto addButton = new QPushButton(tr("Add game"));

    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(saveButton,   0, Qt::AlignCenter);
    buttonsLayout->addWidget(cancelButton, 0, Qt::AlignCenter);
    buttonsLayout->addWidget(addButton,    0, Qt::AlignCenter);

    auto outputListWidget = new QTreeWidget();
    outputListWidget->setColumnCount(1);
    outputListWidget->setHeaderHidden(true);
    outputListWidget->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);

    QDialog filePrompt(this);
    filePrompt.setModal(true);
    filePrompt.setSizeGripEnabled(false);

    QString sprayName = "";
    connect(saveButton,   &QPushButton::clicked,
            &filePrompt,  [&sprayName, lineEdit](){
        sprayName = lineEdit->text();
    });

    connect(saveButton,   &QPushButton::clicked,
            &filePrompt,  &QDialog::accept);

    connect(cancelButton, &QPushButton::clicked,
            &filePrompt,  &QDialog::reject);

    auto updateFileList = [=, this](QString sprayName, bool &fileExists){
        QList<QTreeWidgetItem *> filenames;

        auto localSprayFile = "sprays/" + sprayName + ".vtf";
        auto localSprayFileItem = new QTreeWidgetItem(QStringList(localSprayFile));
        filenames.append(localSprayFileItem);
        if (QFile::exists(localSprayFile))
        {
            fileExists = true;
            auto icon = overwriteCheckbox->isChecked()
                            ? QIcon::ThemeIcon::DialogWarning
                            : QIcon::ThemeIcon::DialogError;
            localSprayFileItem->setIcon(0, QIcon::fromTheme(icon));
            localSprayFileItem->setToolTip(0, tr("File exists"));
        }

        for(const auto& gameSpray : gamesWithSprays)
        {
            auto outputs = gameSpray.getOutputFiles(sprayName);
            auto fullPath = gameSpray.getLogosDirectory();
            auto entry = new QTreeWidgetItem(QStringList(fullPath));

            for(const auto& outfile : outputs)
            {
                auto truncated = outfile.split(fullPath + "/")[1];
                auto child = new QTreeWidgetItem(QStringList(truncated));
                entry->addChild(child);

                if (QFile::exists(outfile))
                {
                    fileExists = true;
                    auto icon = overwriteCheckbox->isChecked()
                                    ? QIcon::ThemeIcon::DialogWarning
                                    : QIcon::ThemeIcon::DialogError;
                    entry->setIcon(0, QIcon::fromTheme(icon));
                    entry->setToolTip(0, tr("File exists"));

                    child->setIcon(0, QIcon::fromTheme(icon));
                    child->setToolTip(0, tr("File exists"));
                }
            }

            filenames.append(entry);
        }
        outputListWidget->clear();
        outputListWidget->insertTopLevelItems(0, filenames);
        outputListWidget->expandAll();
        outputListWidget->resizeColumnToContents(0);
    };

    connect(addButton,    &QPushButton::clicked,
            this,         [=, this](){
        QFileDialog addGameDialog(this);
        addGameDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
        addGameDialog.setFileMode(QFileDialog::FileMode::Directory);
        addGameDialog.setOption(QFileDialog::Option::ShowDirsOnly);
        addGameDialog.setOption(QFileDialog::Option::DontConfirmOverwrite);

        connect(&addGameDialog, &QFileDialog::fileSelected,
                this,           [=, this](const QString &file){
            auto status = tryAddGame(file);
            if (status == TryAddGameResult::LogosDirNotFound)
                QMessageBox::critical(this,
                                      tr("Not found"),
                                      tr("Unable to find a logos directory within:\n%1").arg(file));
            else if (status == TryAddGameResult::Duplicate)
                QMessageBox::critical(this,
                                      tr("Duplicate"),
                                      tr("Directory is already in the games list:\n%1").arg(file));
        });

        addGameDialog.exec();

        {
            bool _;
            updateFileList(lineEdit->text(), _);
        }
    });

    connect(overwriteCheckbox, &QCheckBox::checkStateChanged,
            this,              [=, this](Qt::CheckState state){
        bool fileExists = false;
        bool hasText = lineEdit->text().length() > 0;
        updateFileList(lineEdit->text(), fileExists);
        if (state == Qt::CheckState::Checked)
            saveButton->setEnabled(hasText);
        else
            saveButton->setEnabled(hasText && fileExists == false);
    });

    auto filePromptLayout = new QVBoxLayout();
    filePromptLayout->setAlignment(Qt::AlignCenter);
    filePromptLayout->setSizeConstraint(QLayout::SetFixedSize);

    filePrompt.setLayout(filePromptLayout);
    filePromptLayout->addWidget(new QLabel(tr("Spray name")));
    filePromptLayout->addWidget(lineEdit);
    filePromptLayout->addWidget(overwriteCheckbox, Qt::AlignLeft);
    filePromptLayout->addWidget(outputListWidget);
    filePromptLayout->addLayout(buttonsLayout);

    {
        bool _;
        updateFileList("", _);
    }

    connect(lineEdit, &QLineEdit::textChanged,
            this,     [=, this]() {
        // Ensure filename only contains: a-z, 0-9, -, _
        // Uppercase characters don't work on case sensitive filesystems
        static auto regex = QRegularExpression("[^a-z0-9\\-_]");
        auto filteredFilename = lineEdit->text().replace(" ", "_").toLower().remove(regex);
        filteredFilename.truncate(99);
        lineEdit->setText(filteredFilename);

        bool hasText = filteredFilename.length() > 0;
        bool fileExists = false;

        updateFileList(filteredFilename, fileExists);

        if (overwriteCheckbox->isChecked())
            saveButton->setEnabled(hasText);
        else
            saveButton->setEnabled(hasText && fileExists == false);
    });

    auto result = filePrompt.exec();

    if (result == QDialog::Rejected)
        return "";

    return sprayName;
}

void Spraymaker::saveSpray()
{
    spraymakerModel->invalidateProgress();

    auto sprayName = sprayNamePrompt();

    if(sprayName.length() == 0)
        return;

    auto width   = spraymakerModel->getWidth();
    auto height  = spraymakerModel->getHeight();
    auto mipmaps = spraymakerModel->getMipmapCount();
    auto frames  = spraymakerModel->getFrameCount();
    auto format  = spraymakerModel->getFormat();

    auto buffer = std::make_unique<uchar[]>(spraymakerModel->getVtfFileSize());
    uchar* pos = buffer.get();

    new(pos) VTF_HEADER_71
        {
        .signature  = {'V', 'T', 'F', 0},
        .version    = {7, 1},
        .headerSize = 64,
        .width      = (ushort)width,
        .height     = (ushort)height,
        .flags =
            VTF_FLAGS::CLAMPS | VTF_FLAGS::CLAMPT | VTF_FLAGS::CLAMPU | VTF_FLAGS::NOLOD | VTF_FLAGS::ALL_MIPS
             | (mipmaps == 1                          ? VTF_FLAGS::NOMIP         : VTF_FLAGS::NONE)
             | (ImageHelper::hasOneBitAlpha(format)   ? VTF_FLAGS::ONEBITALPHA   : VTF_FLAGS::NONE)
             | (ImageHelper::hasMultiBitAlpha(format) ? VTF_FLAGS::EIGHTBITALPHA : VTF_FLAGS::NONE)
             | (mipmaps == 1
                && spraymakerModel->getTextureSampleMode() == SpraymakerModel::TextureSampleMode::POINT_SAMPLE
                ? VTF_FLAGS::POINTSAMPLE : VTF_FLAGS::NONE)
             | (spraymakerModel->getTextureSampleMode() == SpraymakerModel::TextureSampleMode::ANISOTROPIC
                ? VTF_FLAGS::ANISOTROPIC : VTF_FLAGS::NONE)
             | (spraymakerModel->getTextureSampleMode() == SpraymakerModel::TextureSampleMode::TRILINEAR
                ? VTF_FLAGS::TRILINEAR   : VTF_FLAGS::NONE)
        ,
        .frames             = (ushort)frames,
        .firstFrame         = 0,
        .padding0           = {'C', 'M', 'C', '3'},
        .reflectivity       = {0.5, 0.5, 0.5},
        .padding1           = {'B', 'F', 'F', '!'},
        .bumpmapScale       = 1.0,
        .highResImageFormat = spraymakerModel->mapFormat().vtfFormat,
        .mipmapCount        = (uchar)mipmaps,
        .lowResImageFormat  = VTF_IMAGE_FORMAT::NONE,
        .lowResImageWidth   = 0,
        .lowResImageHeight  = 0,
        .padding2           = 20,
        };

    pos += sizeof(VTF_HEADER_71);

    ImageHelper::PixelAlphaMode pixelAlphaMode = ImageHelper::PixelAlphaMode::INVALID;

    if(ImageHelper::hasOneBitAlpha(format) || ImageHelper::hasAlpha(format) == false)
        pixelAlphaMode = ImageHelper::PixelAlphaMode::THRESHOLD;

    if(ImageHelper::hasMultiBitAlpha(format))
        pixelAlphaMode = ImageHelper::PixelAlphaMode::FULL;

    int alphaThreshold = settings->getAlphaThreshold();

    bool boundedAutocrop = false;
    bool forceBoundedAutocrop = false;
    bool autocrop = false;

    switch(spraymakerModel->getAutocropMode())
    {
    case SpraymakerModel::AutocropMode::BOUNDINGBOX:
        forceBoundedAutocrop = true;
    case SpraymakerModel::AutocropMode::AUTOMATIC:
        boundedAutocrop = true;
    case SpraymakerModel::AutocropMode::INDIVIDUAL:
        autocrop = true;
        break;
    case SpraymakerModel::AutocropMode::NONE:
    default:
        break;
    }

    // VTF mipmaps are ordered smallest to largest
    for(int mipmap = mipmaps - 1; mipmap >= 0; mipmap--)
    {
        auto mipWidth  = std::max(1, spraymakerModel->getWidth()  >> mipmap);
        auto mipHeight = std::max(1, spraymakerModel->getHeight() >> mipmap);

        // ========== Find bounding box for autocropping animations ==========
        uint lastWidth = 0;
        uint lastHeight = 0;
        auto bb = ImageHelper::BoundingBox();
        if (boundedAutocrop)
        for(int frame = 0; frame < frames; frame++)
        {
            const auto img = spraymakerModel->getImage(mipmap, frame);

            if (frame == 0)
            {
                lastWidth  = img->width();
                lastHeight = img->height();
            }

            if (forceBoundedAutocrop == false
                && (img->width() != lastWidth || img->height() != lastHeight))
            {
                boundedAutocrop = false;
                break;
            }

            bb += ImageHelper::getImageBorders(img->data(), img->width(), img->height(),
                                               pixelAlphaMode, alphaThreshold);

            lastWidth  = img->width();
            lastHeight = img->height();
        }
        // ========== / Find bounding box for autocropping animations ==========

        for(int frame = 0; frame < frames; frame++)
        {
            auto img = spraymakerModel->getImage(mipmap, frame)->copy();

            // Apply appropriate autocrop method
            if (forceBoundedAutocrop || boundedAutocrop)
            {
                img = img.crop(bb.left, bb.top, bb.width, bb.height);
            }
            else if (autocrop)
            {
                auto bb = ImageHelper::getImageBorders(img.data(), img.width(), img.height(),
                                                       pixelAlphaMode, alphaThreshold);
                img = img.crop(bb.left, bb.top, bb.width, bb.height);
            }

            // Should the user be able to set scale mode between fit, fill, stretch, none?
            // TODO: Proper scale method for pixel art
            img = img.thumbnail_image(mipWidth,
                                      vips::VImage::option()
                                          ->set("height", mipHeight)
                                          ->set("size", VipsSize::VIPS_SIZE_BOTH));

            img = img.gravity(VipsCompassDirection::VIPS_COMPASS_DIRECTION_CENTRE,
                              mipWidth, mipHeight,
                              vips::VImage::option()
                                  ->set("background", std::vector<double>{
                                                          (double)spraymakerModel->getBackgroundRed(),
                                                          (double)spraymakerModel->getBackgroundGreen(),
                                                          (double)spraymakerModel->getBackgroundBlue(),
                                                          (double)spraymakerModel->getBackgroundAlpha(),
                                                      })
                                  ->set("extend", VIPS_EXTEND_BACKGROUND));

            // ========== Fix transparency for 1-bit and nonalpha targets ==========
            if (ImageHelper::hasOneBitAlpha(format) || ImageHelper::hasAlpha(format) == false)
            {
                auto replaceEffectivePixel = [=, this](uchar* ptr, uint x, uint y, uint width,
                                                       ImageHelper::PixelAlphaMode pixelAlphaMode, int alphaThreshold) {
                    uchar* pixelPtr = (uchar*)ptr + (x*4 + y*width*4);
                    uint pixel = (pixelPtr[0] << 0 )
                               | (pixelPtr[1] << 8 )
                               | (pixelPtr[2] << 16);

                    switch(pixelAlphaMode)
                    {
                    case ImageHelper::PixelAlphaMode::FULL:
                        pixel |= pixelPtr[3] << 24;
                        break;
                    case ImageHelper::PixelAlphaMode::THRESHOLD:
                        if (pixelPtr[3] < alphaThreshold)
                            pixel = spraymakerModel->getBackgroundRed()   << 0
                                  | spraymakerModel->getBackgroundGreen() << 8
                                  | spraymakerModel->getBackgroundBlue()  << 16;
                        else
                            pixel |= 0xff000000;
                        break;
                    case ImageHelper::PixelAlphaMode::NONE:
                    default:
                        break;
                    }

                    for(int c = 0; c < 4; c++)
                    {
                        pixelPtr[c] = pixel >> (c*8);
                    }
                };

                for(uint x = 0; x < img.width(); x++)
                {
                    for(uint y = 0; y < img.height(); y++)
                    {
                        replaceEffectivePixel(
                            (uchar*)img.data(), x, y, img.width(),
                            ImageHelper::PixelAlphaMode::THRESHOLD, alphaThreshold);
                    }
                }
            }
            // ========== / Fix transparency for 1-bit and nonalpha targets ==========

            // ========== crnlib conversion ==========
            auto crnStyleImage = new crnlib::image_u8((crnlib::color_quad_u8*)img.data(), mipWidth, mipHeight);

            crnlib::mipmapped_texture mipTex;
            mipTex.init(mipWidth, mipHeight, 1, 1, crnlib::PIXEL_FMT_A8R8G8B8, "", crnlib::cDefaultOrientationFlags);
            mipTex.assign(crnStyleImage, crnlib::PIXEL_FMT_A8R8G8B8);

            auto params = crnlib::dxt_image::pack_params();
            params.m_pProgress_callback = Spraymaker::crnProgressCallback;
            params.m_num_helper_threads = settings->getCrnHelperThreads();

            if (mipTex.convert(spraymakerModel->mapFormat().crnFormat, params) == false)
            {
                throw SpraymakerException(tr("crnlib error:\n%1").arg(mipTex.get_last_error().c_str()));
            }
            // ========== / crnlib conversion ==========

            // ========== Buffer copying and pixel alignment ==========
            if (ImageHelper::isDxt(format))
            {
                auto dataVec = mipTex.get_level(0, 0)->get_dxt_image()->get_element_vec();
                for(const auto& element : dataVec)
                {
                    for(int i = 0; i < 8; i++)
                    {
                        // TODO: Benchmark vs. memcpy
                        *pos = element.m_bytes[i];
                        pos++;
                    }
                }
            }
            else
            {
                auto dataPtr = mipTex.get_level(0, 0)->get_image()->get_ptr();
                auto count = img.width()*img.height();
                ImageHelper::convertPixelFormat(dataPtr, pos, count, mipTex.get_format(), format, alphaThreshold);
                pos += count;
            }
            // ========== / Buffer copying and pixel alignment ==========

            imageProgressBar->setValue(imageProgressBar->value() + 1);
        }
    }

    auto filePath = ("./sprays/" + sprayName + ".vtf").toUtf8();
    const char* file = filePath;
    std::ofstream(file, std::ios::binary).write((char*)buffer.get(), spraymakerModel->getVtfFileSize());

    for (const auto& gameSpray : gamesWithSprays)
    {
        gameSpray.installSpray(filePath, sprayName, true);
    }
}
