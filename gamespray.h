/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GAMESPRAY_H
#define GAMESPRAY_H

#include "spraymakerexception.h"

#include <QObject>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QDirIterator>

class GameSpray
{
private:
    QString vtfDir;
    QString uiDir;
    QString logosDir;
    QString uiVmt;
    QString sprayVmt;
    bool custom = false;

    // TODO: Confirm if the differences here matter. Can the UI VMT point to the Spray VTF without a VMT of its own?
    // Might be able to condense this into one or two VMTs total.
    const std::string uiVmtArg = R"("UnlitGeneric"
{
    "$translucent"   1
    "$basetexture"   "vgui/logos/%1"
    "$vertexcolor"   1
    "$vertexalpha"   1
    "$no_fullbright" 1
    "$ignorez"       1
})";

    const std::string uiCustomVmtArg = R"("UnlitGeneric"
{
    "$translucent"   1
    "$basetexture"   "vgui/logos/custom/%1"
    "$vertexcolor"   1
    "$vertexalpha"   1
    "$no_fullbright" 1
    "$ignorez"       1
})";

    const std::string sprayVmtArg = R"("UnlitGeneric"
{
    "$basetexture" "vgui/logos/%1"
    "$translucent" "1"
    "$ignorez"     "1"
    "$vertexcolor" "1"
    "$vertexalpha" "1"
})";

    const std::string sprayCustomVmtArg = R"(LightmappedGeneric
{
    "$basetexture" "vgui/logos/custom/%1"
    "$translucent" "1"
    "$decal"       "1"
    "$decalscale"  "0.250"
})";

public:
    GameSpray(QString directory);

    std::list<QString> getOutputFiles(QString sprayName) const;
    QString getVtfFilename(QString sprayName) const;
    QString getVmtFilename(QString sprayName) const;
    QString getUiVmtFilename(QString sprayName) const;
    QString getLogosDirectory() const;
    bool outputExists(QString vtfFile) const;
    bool installSpray(QString vtfFilePath, QString sprayName, bool overwrite) const;
};

#endif // GAMESPRAY_H
