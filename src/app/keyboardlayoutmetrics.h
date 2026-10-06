// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace Tastra
{

struct KeyboardLayoutMetrics
{
    double contentWidthRatio;
    double maxContentWidth;
    double keyHeight;
    double keyGap;
    double outerMargin;
    double topPadding;
    double bottomPadding;
    double keyRadius;
    double fontSize;
    double popupHeight;

    double panelHeight() const
    {
        constexpr int rows = 4;
        constexpr int gapsBetweenRows = 3;

        return topPadding
            + bottomPadding
            + keyHeight * rows
            + keyGap * gapsBetweenRows;
    }

    static KeyboardLayoutMetrics portrait();
    static KeyboardLayoutMetrics landscape();
};

}
