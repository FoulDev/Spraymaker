/*
 * This file is part of Spraymaker.
 * Spraymaker is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * Spraymaker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Spraymaker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CUSTOMSTEPSPINBOX_H
#define CUSTOMSTEPSPINBOX_H

#include <QWidget>
#include <QSpinBox>

class CustomStepSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit CustomStepSpinBox(QWidget *parent = nullptr);

    enum class StepMode : int {
        INVALID = -1,

        SingleStep = 0,
        MultipleOfFour,
        PowerOfTwo,

        _MAX = PowerOfTwo,
        _COUNT,
    };
    Q_ENUM(StepMode);

    virtual void stepBy(int steps) override;

protected:
    StepMode customStep;

public slots:
    void setCustomStep(StepMode customStep);
    void revalidateValue();
};

#endif // CUSTOMSTEPSPINBOX_H
