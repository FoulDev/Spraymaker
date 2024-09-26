/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "sizedisplaylabel.h"

SizeDisplayLabel::SizeDisplayLabel()
{
    setAutoFillBackground(true);
    badPalette.setColor(QPalette::Window, Qt::red);
    setAlignment(Qt::AlignCenter);
}

void SizeDisplayLabel::setFileSize(int size)
{ fileSize = size; updateDisplay(); }

void SizeDisplayLabel::setMaxFileSize(int maxSize)
{ maxFileSize = maxSize; updateDisplay(); }

void SizeDisplayLabel::updateDisplay()
{
    setText(tr("Size %1 / %2").arg(fileSize).arg(maxFileSize));
    if (fileSize > maxFileSize)
        setPalette(badPalette);
    else
        setPalette(goodPalette);
}
