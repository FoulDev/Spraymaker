/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#include "customstepspinbox.h"

#include <QWidget>
#include <QSpinBox>
#include <QLineEdit>

CustomStepSpinBox::CustomStepSpinBox(QWidget *parent)
    : QSpinBox{parent}
{
    setMinimum(4);
    setMaximum(4096);
    setCustomStep(StepMode::MultipleOfFour);

    // Prevent manual text entry
    lineEdit()->setReadOnly(true);
    lineEdit()->setFocusPolicy(Qt::NoFocus);

    // Stop pointless text highlighting
    connect(lineEdit(), &QLineEdit::selectionChanged,
            lineEdit(), [=, this](){ lineEdit()->setSelection(0, 0); });
}

void CustomStepSpinBox::stepBy(int steps)
{
    int newValue = value();
    if (steps > 0)
    {
        switch(customStep)
        {
        case StepMode::MultipleOfFour:
            newValue += 4;
            break;
        case StepMode::PowerOfTwo:
            newValue <<= 1;
            break;
        case StepMode::SingleStep:
        default:
            newValue += 1;
            break;
        }
    }
    else
    {
        switch(customStep)
        {
        case StepMode::MultipleOfFour:
            newValue -= 4;
            break;
        case StepMode::PowerOfTwo:
            newValue >>= 1;
            break;
        case StepMode::SingleStep:
        default:
            newValue -= 1;
            break;
        }
    }

    newValue = std::clamp(newValue, minimum(), maximum());
    setValue(newValue);
}

void CustomStepSpinBox::setCustomStep(StepMode customStep)
{
    this->customStep = customStep;
    revalidateValue();
}

void CustomStepSpinBox::revalidateValue()
{
    int newValue = value();

    switch(customStep)
    {
    case StepMode::MultipleOfFour:
    {
        int rem = newValue % 4;
        if (rem == 0)
            break;

        newValue += 4 - rem;
        break;
    }
    case StepMode::PowerOfTwo:
        for(uint i = 0; i < 16; i++)
        {
            uint power = 1U << i;
            if (power >= value())
            {
                newValue = power;
                break;
            }
        }
        break;
    case StepMode::SingleStep:
    default:
        break;
    }

    newValue = std::clamp(newValue, minimum(), maximum());
    setValue(newValue);
}
