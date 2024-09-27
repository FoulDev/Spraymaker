/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings* getInstance();

    QList<QString> getLogoDirs();
    bool hasLogoDir(QString logodir);

    int getCrnHelperThreads();
    int getPreviewResolution();
    bool getUseSimpleFormats();
    int getAlphaThreshold();

    static void init();

public slots:
    bool addLogoDir(QString logodir);
    void removeLogoDir(QString logodir);
    void setCrnHelperThreads(int helperThreadCount);
    void setUseSimpleFormats(bool simpleFormats);
    void setPreviewResolution(int previewResolution);
    void setAlphaThreshold(int alphaThreshold);
    void save();

private:
    explicit Settings();

    QSettings* settings;
    QList<QString> logodirs;

    int crnHelperThreads;
    int previewResolution;
    int alphaThreshold;
    bool useSimpleFormats;
};

#endif // SETTINGS_H
