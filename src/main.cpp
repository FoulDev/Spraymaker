/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "spraymaker.h"
#include "spraymakerapplication.h"

// glib, used by libvips, has its own signals
#pragma push_macro("signals")
#undef signals
#include <vips/vips8>
#pragma pop_macro("signals")

#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    VIPS_INIT(argv[0]); // libvips

    SpraymakerApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "Spraymaker_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    auto w = Spraymaker::getInstance();

    {
        auto title = QString("Spraymaker %1.%2.%3")
                         .arg(SPRAYMAKER_VERSION_MAJOR)
                         .arg(SPRAYMAKER_VERSION_MINOR)
                         .arg(SPRAYMAKER_VERSION_PATCH);
#ifdef QT_DEBUG
        title += " DEBUG -- do not distribute";
#endif
        w->setWindowTitle(title);
    }

    w->show();
    return a.exec();
}
