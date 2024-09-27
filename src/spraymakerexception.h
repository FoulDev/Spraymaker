/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SPRAYMAKEREXCEPTION_H
#define SPRAYMAKEREXCEPTION_H

#include <stdexcept>
#include <QString>

struct SpraymakerException : public std::runtime_error
{
    bool hasDebugMessage = false;
    QString debugMessage;

    SpraymakerException(QString message)
        : std::runtime_error(message.toStdString())
    {};

    SpraymakerException(QString message, QString debugMessage)
        : std::runtime_error(message.toStdString())
        , debugMessage(debugMessage)
        , hasDebugMessage(true)
    {};
};

#endif // SPRAYMAKEREXCEPTION_H
