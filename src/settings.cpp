/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "settings.h"
#include <QCoreApplication>
#include <QSettings>
#include <crnlib.h>

#include <thread>

Settings::Settings()
{
    init();
    settings = new QSettings(QSettings::IniFormat, QSettings::SystemScope, "Spraymaker");

    auto threads = std::clamp(std::thread::hardware_concurrency(), 4U, (uint)cCRNMaxHelperThreads);

    logodirs = settings->value("logodirs", QList<QString>()).toStringList();
    crnHelperThreads = std::clamp(settings->value("crn_helper_threads", threads).toInt(), 0, (int)cCRNMaxHelperThreads);
    useSimpleFormats = settings->value("simple_formats", true).toBool();
    previewResolution = std::clamp(settings->value("preview_resolution", 128).toInt(), 64, 1024);
    alphaThreshold = std::clamp(settings->value("alpha_threshold", 128).toInt(), -1, 256);

    save();
}

Settings* Settings::getInstance()
{
    static Settings instance;
    return &instance;
}

void Settings::save()
{
    settings->setValue("logodirs", logodirs);
    settings->setValue("crn_helper_threads", crnHelperThreads);
    settings->setValue("simple_formats", useSimpleFormats);
    settings->setValue("preview_resolution", previewResolution);
    settings->setValue("alpha_threshold", alphaThreshold);
    settings->sync();
}

void Settings::init()
{
    QCoreApplication::setApplicationName("Spraymaker");
    QCoreApplication::setOrganizationName("Spraymaker");
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, QCoreApplication::applicationDirPath());
}

QList<QString> Settings::getLogoDirs()
{ return logodirs; }

bool Settings::addLogoDir(QString logodir)
{
    if (hasLogoDir(logodir))
        return false;

    logodirs.push_back(logodir);

    save();

    return true;
}

void Settings::removeLogoDir(QString logodir)
{
    auto newLogodirs = QList<QString>();

    for(const auto & _logodir : std::as_const(logodirs))
    {
        if(logodir != _logodir)
            newLogodirs.push_back(_logodir);
    }

    logodirs = newLogodirs;

    save();
}

bool Settings::hasLogoDir(QString logodir)
{
    return logodirs.contains(logodir);
}

int Settings::getCrnHelperThreads()
{ return crnHelperThreads; }

void Settings::setCrnHelperThreads(int helperThreadCount)
{
    crnHelperThreads = helperThreadCount;
    save();
}

bool Settings::getUseSimpleFormats()
{ return useSimpleFormats; }

void Settings::setUseSimpleFormats(bool simpleFormats)
{
    this->useSimpleFormats = simpleFormats;
    save();
}

int Settings::getPreviewResolution()
{ return previewResolution; }

void Settings::setPreviewResolution(int previewResolution)
{
    this->previewResolution = previewResolution;
    save();
}

int Settings::getAlphaThreshold()
{ return alphaThreshold; }

void Settings::setAlphaThreshold(int alphaThreshold)
{
    this->alphaThreshold = alphaThreshold;
    save();
}
