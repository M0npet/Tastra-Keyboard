// SPDX-License-Identifier: GPL-3.0-or-later

#include "keyboardlayoutmetrics.h"

namespace V3Keyboard
{

KeyboardLayoutMetrics KeyboardLayoutMetrics::portrait()
{
    return {
        .contentWidthRatio = 0.97,
        .maxContentWidth = 980.0,
        .keyHeight = 70.0,
        .keyGap = 7.0,
        .outerMargin = 12.0,
        .topPadding = 12.0,
        .bottomPadding = 14.0,
        .keyRadius = 15.0,
        .fontSize = 27.0,
        .popupHeight = 58.0,
    };
}

KeyboardLayoutMetrics KeyboardLayoutMetrics::landscape()
{
    return {
        .contentWidthRatio = 0.84,
        .maxContentWidth = 1380.0,
        .keyHeight = 58.0,
        .keyGap = 7.0,
        .outerMargin = 18.0,
        .topPadding = 10.0,
        .bottomPadding = 12.0,
        .keyRadius = 14.0,
        .fontSize = 24.0,
        .popupHeight = 54.0,
    };
}

}
