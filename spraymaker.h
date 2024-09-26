/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SPRAYMAKER_H
#define SPRAYMAKER_H

#include "settings.h"
#include "spraymakermodel.h"
#include "gamespray.h"

#include <QMainWindow>
#include <QProgressBar>

#include <crunch/inc/crnlib.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class Spraymaker;
}
QT_END_NAMESPACE

class Spraymaker : public QMainWindow
{
    Q_OBJECT

public:
    Spraymaker(QWidget *parent = nullptr);
    ~Spraymaker();
    static Spraymaker* getInstance();

private slots:
    void saveSpray();
    void aboutDialog();

private:
    enum class TryAddGameResult
    {
        INVALID = -1,

        Success = 0,
        LogosDirNotFound,
        Duplicate,

        _MAX = Duplicate,
        _COUNT,
    };

    static Spraymaker *instance;
    Ui::Spraymaker *ui;
    SpraymakerModel *spraymakerModel;

    Settings* settings;
    std::list<GameSpray> gamesWithSprays;
    TryAddGameResult tryAddGame(QString directory);

    QProgressBar *imageProgressBar;
    QProgressBar *encodingProgressBar;

    QString sprayNamePrompt();

    static bool crnProgressCallback(uint percentage_complete, void* pUser_data_ptr);
};
#endif // SPRAYMAKER_H
