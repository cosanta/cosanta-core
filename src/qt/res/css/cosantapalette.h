// Copyright (c) 2026 The Cosanta Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RES_CSS_COSANTAPALETTE_H
#define BITCOIN_QT_RES_CSS_COSANTAPALETTE_H

#include <qt/guiutil.h>

#include <QColor>
#include <QString>

#include <map>

// Colors used by painters, models and rich text that bypass the widget stylesheet.
namespace GUIUtil::CosantaPalette {
inline const std::map<ThemedColor, QColor> colors{
    {ThemedColor::DEFAULT, QColor("#f7f4f6")},
    {ThemedColor::UNCONFIRMED, QColor("#b1a6ac")},
    {ThemedColor::BLUE, QColor("#ff2637")}, // Brand accent for icons and charts
    {ThemedColor::ORANGE, QColor("#bcbcbc")},
    {ThemedColor::RED, QColor("#ff2637")},
    {ThemedColor::GREEN, QColor("#f7f4f6")},
    {ThemedColor::BAREADDRESS, QColor("#ccc6cc")},
    {ThemedColor::TX_STATUS_OPENUNTILDATE, QColor("#bcbcbc")},
    {ThemedColor::BACKGROUND_WIDGET, QColor("#1d171c")},
    {ThemedColor::BORDER_WIDGET, QColor("#3a3339")},
    {ThemedColor::BACKGROUND_NETSTATS, QColor(29, 23, 28, 240)},
    {ThemedColor::BORDER_NETSTATS, QColor("#57434b")},
    {ThemedColor::TRAFFIC_RECEIVED, QColor(255, 38, 55, 72)},
    {ThemedColor::TRAFFIC_SENT, QColor(194, 194, 194, 0)},
    {ThemedColor::TRAFFIC_GRID, QColor("#625e60")},
    {ThemedColor::CONSOLE_ALERT, QColor("#ff2637")},
    {ThemedColor::QR_PIXEL, QColor("#ffffff")},
    {ThemedColor::ICON_ALTERNATIVE_COLOR, QColor("#856f78")},
};

// Status-bar icons use two grayscale masks. Keep status tints separate from financial text.
inline const std::map<ThemedColor, QColor> statusIconColors{
    {ThemedColor::GREEN, QColor("#eeeeee")}, // Ready
    {ThemedColor::ORANGE, QColor("#bcbcbc")}, // Waiting
    {ThemedColor::BLUE, QColor("#ff2637")}, // Governance highlight
    {ThemedColor::RED, QColor("#ff2637")}, // Attention
    {ThemedColor::ICON_ALTERNATIVE_COLOR, QColor("#686868")},
};

inline const std::map<ThemedStyle, QString> styles{
    {ThemedStyle::TS_INVALID, "border: 3px solid #ff2637;"},
    {ThemedStyle::TS_ERROR, "color: #ff2637;"},
    {ThemedStyle::TS_WARNING, "color: #ff2637;"},
    {ThemedStyle::TS_CONSOLE_COMMAND, "color: #e0e0e0;"},
    {ThemedStyle::TS_SUCCESS, "color: #f7f4f6;"},
    {ThemedStyle::TS_COMMAND, "color: #ff2637;"},
    {ThemedStyle::TS_PRIMARY, "color: #f7f4f6;"},
    {ThemedStyle::TS_SECONDARY, "color: #ccc6cc;"},
};
} // namespace GUIUtil::CosantaPalette

#endif // BITCOIN_QT_RES_CSS_COSANTAPALETTE_H
