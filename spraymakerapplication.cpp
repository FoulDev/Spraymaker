/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "spraymakerapplication.h"
#include "spraymakerexception.h"
#include "spraymaker.h"

#include <QMessageBox>
#include <QGridLayout>
#include <QTextEdit>

SpraymakerApplication::SpraymakerApplication(int &argc, char **argv, int flags)
    : QApplication(argc, argv, flags)
{ }

bool SpraymakerApplication::notify(QObject * receiver, QEvent * event)
{
    bool status = true;

    try {
        status = QApplication::notify(receiver, event);
    } catch (const SpraymakerException &e) {
        auto spraymaker = Spraymaker::getInstance();

        if (e.hasDebugMessage)
        {
            QMessageBox warning;
            warning.setWindowTitle(tr("Error"));
            warning.setText(e.what());
            warning.setIcon(QMessageBox::Icon::Warning);
            warning.setStandardButtons(QMessageBox::StandardButton::Ok);
            warning.setDefaultButton(QMessageBox::StandardButton::Ok);

            auto debugText = new QTextEdit();
            debugText->setReadOnly(true);
            debugText->setPlainText(e.debugMessage);

            auto layout = (QGridLayout*)warning.layout();
            layout->addWidget(debugText, 1, 2);

            warning.exec();
        }
        else
        {
            QMessageBox::warning(spraymaker, tr("Error"), e.what(), QMessageBox::Ok, QMessageBox::Ok);
        }
    }

    return status;
}
