/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "gamespray.h"

GameSpray::GameSpray(QString directory)
{
    QDir dir(directory);
    dir.makeAbsolute();
    // Only look for directories, and ignore case sensitivity
    // Note: Source engine on Linux handles filesystem case sensitivity inconsistently
    dir.setFilter(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot);

    if (dir.exists() == false)
        throw SpraymakerException(QObject::tr("Directory doesn't exist:\n%1").arg(directory));

    if (dir.path().endsWith("/materials/vgui/logos/custom", Qt::CaseInsensitive)
     || dir.path().endsWith("/materials/vgui/logos/ui",     Qt::CaseInsensitive))
        dir.cdUp();

    bool found = dir.path().endsWith("/materials/vgui/logos", Qt::CaseInsensitive);

    if (found)
    {
        vtfDir = dir.path();
        logosDir = vtfDir;
    }

    if (found == false)
    {
        QDirIterator findLogosDir(dir, QDirIterator::Subdirectories);
        while(findLogosDir.hasNext())
        {
            auto path = findLogosDir.next();
            if (path.endsWith("/materials/vgui/logos", Qt::CaseInsensitive))
            {
                dir.setPath(path);
                vtfDir = path;
                logosDir = vtfDir;
                found = true;
                break;
            }
        }
    }

    if (found)
    {
        found = false;
        // .exists() is case sensitive
        QDirIterator findUiDir(dir);
        while(findUiDir.hasNext())
        {
            auto path = findUiDir.next();
            if (path.endsWith("/materials/vgui/logos/ui", Qt::CaseInsensitive))
            {
                uiDir = path;
                found = true;
                break;
            }
        }
    }

    if (found == false)
        throw SpraymakerException(QObject::tr("Failed to find a materials/vgui/logos/ directory within:\n%1").arg(dir.path()));

    // .exists() is case sensitive
    QDirIterator findCustomDir(dir);
    while(findCustomDir.hasNext())
    {
        auto path = findCustomDir.next();
        if (path.endsWith("/materials/vgui/logos/custom", Qt::CaseInsensitive))
        {
            vtfDir = path;
            custom = true;
            break;
        }
    }
}

std::list<QString> GameSpray::getOutputFiles(QString sprayName) const
{
    std::list<QString> fileList =
        {
            getVtfFilename(sprayName),
            getVmtFilename(sprayName),
            getUiVmtFilename(sprayName),
        };
    return fileList;
}

QString GameSpray::getVtfFilename(QString sprayName) const
{ return vtfDir + "/" + sprayName + ".vtf"; }

QString GameSpray::getVmtFilename(QString sprayName) const
{ return vtfDir + "/" + sprayName + ".vmt"; }

QString GameSpray::getUiVmtFilename(QString sprayName) const
{ return uiDir  + "/" + sprayName + ".vmt"; }

QString GameSpray::getLogosDirectory() const
{ return logosDir; }

bool GameSpray::outputExists(QString vtfFile) const
{
    auto files = getOutputFiles(vtfFile);

    for(const auto& file : files)
    {
        if (QFile::exists(file))
            return true;
    }

    return false;
}

bool GameSpray::installSpray(QString inputVtfPath, QString sprayName, bool overwrite) const
{
    if (overwrite == false)
    {
        if (outputExists(sprayName))
            return true;
    }

    QString sprayVmt = custom
                           ? QString::fromStdString(sprayCustomVmtArg).arg(sprayName)
                           : QString::fromStdString(sprayVmtArg).arg(sprayName);

    QString uiVmt = custom
                        ? QString::fromStdString(uiCustomVmtArg).arg(sprayName)
                        : QString::fromStdString(uiVmtArg).arg(sprayName);

    auto vmtPath = getVmtFilename(sprayName);
    auto uiVmtPath = getUiVmtFilename(sprayName);
    auto vtfPath = getVtfFilename(sprayName);

    bool status = true;
    if (overwrite)
    {
        if (QFile::exists(vmtPath))
            status &= QFile::remove(vmtPath);

        if (QFile::exists(uiVmtPath))
            status &= QFile::remove(uiVmtPath);

        if (QFile::exists(vtfPath))
            status &= QFile::remove(vtfPath);
    }

    if (status == false) return false;

    QSaveFile saveFile;
    saveFile.setFileName(vmtPath);
    status &= saveFile.open(QSaveFile::OpenModeFlag::WriteOnly);
    saveFile.write(sprayVmt.toUtf8());
    status &= saveFile.commit();

    saveFile.setFileName(uiVmtPath);
    status &= saveFile.open(QSaveFile::OpenModeFlag::WriteOnly);
    saveFile.write(uiVmt.toUtf8());
    status &= saveFile.commit();

    status &= QFile::copy(inputVtfPath, vtfPath);

    return status;
}

