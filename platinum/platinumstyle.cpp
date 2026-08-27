/****************************************************************************
**
** Copyright (C) 2026 Qize Huang <hgoldfish@gmail.com>
**
** This file is part of qtstyles, a collection of retro Qt widget styles.
**
** This library is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library.  If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "platinumstyle.h"
#include "qtstyles_palette.h"
#include "qstylehelper_p.h"

#include <QtWidgets/qabstractbutton.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qprogressbar.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qstylefactory.h>
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qtabbar.h>
#include <QtWidgets/qwidget.h>

#include <QtGui/qpainter.h>
#include <QtGui/qpolygon.h>
#include <QtGui/qtransform.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qtimer.h>

#include <chrono>
#include <climits>

// Qt 6 renamed QStyleOptionMenuItem::tabWidth to reservedShortcutWidth.
static inline int menuItemTabWidth(const QStyleOptionMenuItem *mi)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return mi->reservedShortcutWidth;
#else
    return mi->tabWidth;
#endif
}

// 单调毫秒时钟。busy 动画的扫动相位由它算出：wall-clock 的
// msecsSinceStartOfDay 在午夜回绕会让相位在午夜跳变；单调钟没有这个问题。
static qint64 steadyMs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

PlatinumStyle::PlatinumStyle(bool forceClassicPalette)
    : QProxyStyle(QStyleFactory::create(QStringLiteral("windows"))),
      m_forceClassicPalette(forceClassicPalette)
{
    // busy 动画用 QTimer 而不是 startTimer()/timerEvent() 驱动：QProxyStyle
    // 的 event() 在 Qt 5 会把定时器事件转发给基类样式，timerEvent() 永不
    // 被调用；QTimer 的 timeout 信号绕开 QStyle::event()，Qt 5/6 行为一致。
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &PlatinumStyle::animateProgressBars);
}

PlatinumStyle::~PlatinumStyle() = default;

/*!
    \reimp

    Returns the classic Macintosh "Platinum" palette: warm beige/gray
    surfaces with a dark navy selection color. The chroma is fixed so the
    retro look is preserved regardless of the host theme. This is a
    suggestion only -- Qt does not adopt it automatically; callers may apply
    it with QApplication::setPalette().
*/
QPalette PlatinumStyle::standardPalette() const
{
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0xd4, 0xd0, 0xc6));
    palette.setColor(QPalette::WindowText, QColor(0x1a, 0x1a, 0x1a));
    palette.setColor(QPalette::Base, QColor(0xfb, 0xfa, 0xf6));
    palette.setColor(QPalette::AlternateBase, QColor(0xe8, 0xe4, 0xd8));
    palette.setColor(QPalette::ToolTipBase, QColor(0xfb, 0xfa, 0xf6));
    palette.setColor(QPalette::ToolTipText, QColor(0x1a, 0x1a, 0x1a));
    palette.setColor(QPalette::Text, QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::Button, QColor(0xdc, 0xd8, 0xcc));
    palette.setColor(QPalette::ButtonText, QColor(0x00, 0x00, 0x00));
    palette.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::Light, QColor(0xf2, 0xef, 0xe6));
    palette.setColor(QPalette::Midlight, QColor(0xe7, 0xe3, 0xd7));
    palette.setColor(QPalette::Mid, QColor(0xb8, 0xb3, 0xa5));
    palette.setColor(QPalette::Dark, QColor(0x8f, 0x8a, 0x7c));
    palette.setColor(QPalette::Shadow, QColor(0x57, 0x53, 0x4a));
    palette.setColor(QPalette::Highlight, QColor(0x00, 0x00, 0x7b));
    palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::PlaceholderText, QColor(0x8f, 0x8a, 0x7c));
    palette.setColor(QPalette::Link, QColor(0x00, 0x00, 0x7b));
    palette.setColor(QPalette::LinkVisited, QColor(0x55, 0x1a, 0x8b));

    QtStyles::applyClassicDisabled(&palette);
    return palette;
}

void PlatinumStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (qobject_cast<QAbstractButton *>(widget) || qobject_cast<QSlider *>(widget)
        || qobject_cast<QScrollBar *>(widget) || qobject_cast<QComboBox *>(widget))
        widget->setAttribute(Qt::WA_Hover);

    // 事件过滤器跟踪 QProgressBar 的可见性与范围，busy（min==max）的条由
    // QTimer 按 animationFps 驱动重绘。
    if (qobject_cast<QProgressBar *>(widget))
        widget->installEventFilter(this);
}

void PlatinumStyle::polish(QPalette &palette)
{
    // "platinum-classic" 在安装时强制套用标准配色，保证与经典观感一致，
    // 不受宿主主题影响。
    if (m_forceClassicPalette)
        palette = standardPalette();
    QProxyStyle::polish(palette);
}

void PlatinumStyle::unpolish(QWidget *widget)
{
    if (QProgressBar *bar = qobject_cast<QProgressBar *>(widget)) {
        widget->removeEventFilter(this);
        stopProgressAnimation(bar);
    }
    if (qobject_cast<QAbstractButton *>(widget) || qobject_cast<QSlider *>(widget)
        || qobject_cast<QScrollBar *>(widget) || qobject_cast<QComboBox *>(widget))
        widget->setAttribute(Qt::WA_Hover, false);
    QProxyStyle::unpolish(widget);
}

bool PlatinumStyle::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::StyleChange:
    case QEvent::Paint:
    case QEvent::Show:
        if (QProgressBar *bar = qobject_cast<QProgressBar *>(watched)) {
            // busy（min 与 max 相等）的条由时钟驱动；确定性范围的条不参与。
            if (bar->minimum() == bar->maximum())
                startProgressAnimation(bar);
            else
                stopProgressAnimation(bar);
        }
        break;
    case QEvent::Hide:
    case QEvent::Destroy:
        // 对象正在销毁时没有类型信息，但过滤器只装在 QProgressBar 上。
        stopProgressAnimation(reinterpret_cast<QProgressBar *>(watched));
        break;
    default:
        break;
    }
    return QProxyStyle::eventFilter(watched, event);
}

void PlatinumStyle::startProgressAnimation(QProgressBar *bar)
{
    if (!animatedBars.contains(bar)) {
        animatedBars << bar;
        if (!animationTimer->isActive()) {
            Q_ASSERT(animationFps > 0);
            animationTimer->start(1000 / animationFps);
        }
    }
}

void PlatinumStyle::stopProgressAnimation(QProgressBar *bar)
{
    if (!animatedBars.isEmpty()) {
        animatedBars.removeOne(bar);
        if (animatedBars.isEmpty())
            animationTimer->stop();
    }
}

// animateProgressBars()
// ---------------------
// 时钟滴答一次就让所有可见的 busy 条重绘；扫动相位在
// drawProgressBarContents 里由单调钟算出，这里只负责触发重绘。
void PlatinumStyle::animateProgressBars()
{
    for (QProgressBar *bar : animatedBars) {
        if (bar->isVisible())
            bar->update();
    }
}

void PlatinumStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case PE_PanelButtonCommand: {
        // Qt 3 PE_ButtonCommand: a square face with softly rounded (4 px)
        // corners -- not a full pill.
        const bool sunken = option->state & (State_Sunken | State_On);
        drawCommandButton(painter, option->rect, option->palette, sunken);
        return;
    }
    case PE_PanelButtonBevel: {
        // Qt 3 PE_ButtonBevel.
        const bool sunken = option->state & State_Sunken;
        drawBevel(painter, option->rect, option->palette, sunken,
                  QBrush(option->palette.color(QPalette::Mid)));
        return;
    }
    case PE_PanelButtonTool: {
        // Qt 3 PE_ButtonTool: tool buttons keep their face color when
        // pressed; a checked tool button renders as a sunken bevel.
        const bool sunken = option->state & (State_Sunken | State_On);
        drawBevel(painter, option->rect, option->palette, sunken,
                  QBrush(option->palette.color(QPalette::Button)));
        return;
    }
    case PE_IndicatorCheckBox: {
        drawCheckBox(painter, option);
        return;
    }
    case PE_IndicatorRadioButton: {
        drawRadioButton(painter, option);
        return;
    }
    case PE_IndicatorProgressChunk: {
        // Appearance Manager: progress indicators take the accent/variation
        // color (same family as scroll thumbs), inset 3 px top/bottom and
        // 2 px right so the groove shows between blocks.
        const QRect r = option->rect;
        painter->fillRect(r.x(), r.y() + 3, r.width() - 2, r.height() - 6,
                          accentColor(option->palette));
        return;
    }
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        // Qt 3 QWindowsStyle PE_Arrow*: the three-line arrow used by spin
        // boxes, tool buttons and menu indicators as well.
        drawArrowGlyph(painter, element, option->rect, option->palette,
                       option->state & State_Enabled,
                       option->palette.color(QPalette::ButtonText));
        return;
    }
    case PE_IndicatorHeaderArrow: {
        // Mac OS 9 list headers use a small filled triangle, not the large
        // Windows three-line glyph. Qt maps SortUp → tip down, SortDown → tip up.
        const QStyleOptionHeader *header =
            qstyleoption_cast<const QStyleOptionHeader *>(option);
        if (!header || header->sortIndicator == QStyleOptionHeader::None)
            return;
        const bool tipDown = header->sortIndicator == QStyleOptionHeader::SortUp;
        const QRect r = option->rect;
        const int cx = r.x() + r.width() / 2;
        const int cy = r.y() + r.height() / 2;
        QPolygon tri;
        if (tipDown) {
            tri << QPoint(cx - 3, cy - 2) << QPoint(cx + 3, cy - 2)
                << QPoint(cx, cy + 2);
        } else {
            tri << QPoint(cx - 3, cy + 2) << QPoint(cx + 3, cy + 2)
                << QPoint(cx, cy - 2);
        }
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(option->palette.color(QPalette::ButtonText));
        painter->drawPolygon(tri);
        painter->restore();
        return;
    }
    case PE_FrameFocusRect: {
        drawPlatinumFocusRect(painter, option->rect, option->palette);
        return;
    }
    case PE_FrameTabBarBase: {
        // Qt 3 has no separate tab bar base line: the bottom edge of every
        // tab is drawn by CE_TabBarTabShape, and the selected tab covers it
        // with the window color. The Qt 6 base line would double the bevel.
        return;
    }
    case PE_PanelMenuBar: {
        // Flat Button face with a 1 px Dark bottom edge.
        const QRect r = option->rect;
        painter->fillRect(r, option->palette.brush(QPalette::Button));
        painter->setPen(option->palette.color(QPalette::Dark));
        painter->drawLine(r.bottomLeft(), r.bottomRight());
        return;
    }
    case PE_PanelToolBar: {
        // Light leading edge + Dark trailing edge (horizontal or vertical).
        const QRect r = option->rect;
        painter->fillRect(r, option->palette.brush(QPalette::Button));
        painter->setPen(option->palette.color(QPalette::Light));
        if (option->state & State_Horizontal) {
            painter->drawLine(r.topLeft(), r.topRight());
            painter->setPen(option->palette.color(QPalette::Dark));
            painter->drawLine(r.bottomLeft(), r.bottomRight());
        } else {
            painter->drawLine(r.topLeft(), r.bottomLeft());
            painter->setPen(option->palette.color(QPalette::Dark));
            painter->drawLine(r.topRight(), r.bottomRight());
        }
        return;
    }
    case PE_IndicatorToolBarHandle: {
        // Same riffle texture as scroll thumbs; orientation is inverted
        // relative to CE_ScrollBarSlider because a horizontal toolbar's
        // handle is a vertical strip decorated with horizontal lines.
        const QRect r = option->rect.adjusted(2, 2, -2, -2);
        painter->fillRect(option->rect, option->palette.brush(QPalette::Button));
        drawRiffles(painter, r, option->palette,
                    !(option->state & State_Horizontal));
        return;
    }
    case PE_IndicatorToolBarSeparator: {
        // Mac OS 8 HIG separator: 2 px engraved line (Dark + Light).
        const QRect r = option->rect;
        painter->save();
        if (option->state & State_Horizontal) {
            const int x = r.center().x();
            painter->setPen(option->palette.color(QPalette::Dark));
            painter->drawLine(x, r.top() + 3, x, r.bottom() - 3);
            painter->setPen(option->palette.color(QPalette::Light));
            painter->drawLine(x + 1, r.top() + 3, x + 1, r.bottom() - 3);
        } else {
            const int y = r.center().y();
            painter->setPen(option->palette.color(QPalette::Dark));
            painter->drawLine(r.left() + 3, y, r.right() - 3, y);
            painter->setPen(option->palette.color(QPalette::Light));
            painter->drawLine(r.left() + 3, y + 1, r.right() - 3, y + 1);
        }
        painter->restore();
        return;
    }
    case PE_IndicatorBranch: {
        drawBranch(painter, option);
        return;
    }
    case PE_FrameGroupBox: {
        // Mac OS primary group box: raised 2 px frame (Light/Midlight TL,
        // Mid/Dark BR). Secondary groups are not distinguished here.
        const QRect r = option->rect;
        const QPalette &pal = option->palette;
        painter->save();
        painter->setPen(pal.color(QPalette::Light));
        painter->drawLine(r.left(), r.top(), r.right(), r.top());
        painter->drawLine(r.left(), r.top(), r.left(), r.bottom());
        painter->setPen(pal.color(QPalette::Midlight));
        painter->drawLine(r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1);
        painter->drawLine(r.left() + 1, r.top() + 1, r.left() + 1, r.bottom() - 1);
        painter->setPen(pal.color(QPalette::Dark));
        painter->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        painter->drawLine(r.right(), r.top(), r.right(), r.bottom());
        painter->setPen(pal.color(QPalette::Mid));
        painter->drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
        painter->drawLine(r.right() - 1, r.top() + 1, r.right() - 1, r.bottom() - 1);
        painter->restore();
        return;
    }
    case PE_FrameMenu:
    case PE_PanelMenu: {
        // Raised Button face + Shadow outline — classic Platinum popup.
        const QRect r = option->rect;
        painter->fillRect(r, option->palette.brush(QPalette::Button));
        painter->setPen(option->palette.color(QPalette::Light));
        painter->drawLine(r.topLeft(), r.topRight());
        painter->drawLine(r.topLeft(), r.bottomLeft());
        painter->setPen(option->palette.color(QPalette::Shadow));
        painter->drawLine(r.bottomLeft(), r.bottomRight());
        painter->drawLine(r.topRight(), r.bottomRight());
        painter->setPen(option->palette.color(QPalette::Mid));
        painter->drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
        painter->drawLine(r.right() - 1, r.top() + 1, r.right() - 1, r.bottom() - 1);
        return;
    }
    default:
        break;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void PlatinumStyle::drawControl(ControlElement element, const QStyleOption *option,
                                QPainter *painter, const QWidget *widget) const
{
    switch (element) {
    case CE_HeaderSection: {
        // Mac OS 9 list headers are flatter than command bevels: Button face
        // with a single-pixel Light/Dark frame (pressed inverts the edges).
        // drawBevel's 2–3 px ramp looks chunky on short header strips.
        const QRect r = option->rect;
        const QPalette &pal = option->palette;
        const bool sunken = option->state & State_Sunken;
        painter->fillRect(r, pal.brush(QPalette::Button));
        painter->setPen(sunken ? pal.color(QPalette::Dark) : pal.color(QPalette::Light));
        painter->drawLine(r.topLeft(), r.topRight());
        painter->drawLine(r.topLeft(), r.bottomLeft());
        painter->setPen(sunken ? pal.color(QPalette::Light) : pal.color(QPalette::Dark));
        painter->drawLine(r.bottomLeft(), r.bottomRight());
        painter->drawLine(r.topRight(), r.bottomRight());
        return;
    }
    case CE_HeaderEmptyArea: {
        // Match section chrome on the shared horizontal edges; skip the
        // verticals so we don't fight the last section's Dark right edge.
        const QRect r = option->rect;
        const QPalette &pal = option->palette;
        painter->fillRect(r, pal.brush(QPalette::Button));
        painter->setPen(pal.color(QPalette::Light));
        painter->drawLine(r.topLeft(), r.topRight());
        painter->setPen(pal.color(QPalette::Dark));
        painter->drawLine(r.bottomLeft(), r.bottomRight());
        return;
    }
    case CE_PushButton: {
        // Qt 3 CE_PushButton: toggle buttons (and small icon buttons) use
        // the bevel face, everything else the command button.
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const QRect r = btn->rect;
            const bool flat = btn->features & QStyleOptionButton::Flat;
            const bool down = btn->state & State_Sunken;
            const bool on = btn->state & State_On;
            const bool isDefault = btn->features & QStyleOptionButton::DefaultButton;

            const QAbstractButton *ab = qobject_cast<const QAbstractButton *>(widget);
            const bool toggle = ab && ab->isCheckable();
            const bool smallIcon = !btn->icon.isNull()
                && (r.width() * r.height() < 1600 || qAbs(r.width() - r.height()) < 10);
            const bool useBevel = toggle || smallIcon;

            // Qt 3: auto-default buttons (the keyboard-pressed push buttons
            // of a dialog) and default buttons are indented by the
            // default-button indicator.
            const QPushButton *pb = qobject_cast<const QPushButton *>(widget);
            const bool autoDefault = pb && pb->autoDefault();

            QRect rect = r;
            if (isDefault) {
                // Qt 3: a default button gets a 1-pixel outer ring. The ring
                // of a command button is drawn in the Mid face color.
                const QRect outerRect = r.adjusted(1, 1, -1, -1);
                if (useBevel) {
                    drawBevel(painter, outerRect, btn->palette, false,
                              QBrush(btn->palette.color(QPalette::Button)));
                } else {
                    QPalette outer = btn->palette;
                    outer.setColor(QPalette::Button, btn->palette.color(QPalette::Mid));
                    drawCommandButton(painter, outerRect, outer, false);
                }
                rect = outerRect;
            }
            if (isDefault || autoDefault)
                rect = rect.adjusted(3, 3, -3, -3);

            if (!flat || on || down) {
                if (useBevel) {
                    QBrush fill(btn->palette.color(QPalette::Button));
                    if (down) {
                        fill = QBrush(btn->palette.color(QPalette::Dark));
                    } else if (on) {
                        // Qt 3: a checked toggle button gets a dotted mid
                        // face instead of a plain color.
                        fill = QBrush(btn->palette.color(QPalette::Mid), Qt::Dense4Pattern);
                    }
                    drawBevel(painter, rect, btn->palette, down || on, fill);
                } else {
                    drawCommandButton(painter, rect, btn->palette, down || on);
                }
            }

            if (btn->state & State_HasFocus) {
                QStyleOptionFocusRect focus;
                focus.QStyleOption::operator=(*btn);
                focus.rect = subElementRect(SE_PushButtonFocusRect, btn, widget);
                drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
            }
            if (btn->features & QStyleOptionButton::HasMenu) {
                // Qt 3 CE_PushButtonLabel: the menu indicator (a down arrow)
                // sits on the right side of the button, merged with the face
                // rather than framed as a separate button.
                const int dx = pixelMetric(PM_MenuButtonIndicator, btn, widget);
                // A pressed/checked face is Dark, so the arrow flips to
                // bright text like CE_PushButtonLabel does.
                const QColor arrowColor = (down || on)
                    ? btn->palette.color(QPalette::BrightText)
                    : btn->palette.color(QPalette::ButtonText);
                drawArrowGlyph(painter, PE_IndicatorArrowDown,
                               QRect(r.right() - dx, r.y() + 3, dx, r.height() - 6),
                               btn->palette, btn->state & State_Enabled, arrowColor);
            }
            QProxyStyle::drawControl(CE_PushButtonLabel, btn, painter, widget);
        }
        return;
    }
    case CE_PushButtonLabel: {
        // Qt 3: a pressed or checked button gets the dark face, so its label
        // flips to bright text to stay readable.
        if (const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            if (btn->state & (State_Sunken | State_On)) {
                QStyleOptionButton sub = *btn;
                sub.palette.setColor(QPalette::ButtonText, sub.palette.color(QPalette::BrightText));
                QProxyStyle::drawControl(CE_PushButtonLabel, &sub, painter, widget);
            } else {
                QProxyStyle::drawControl(CE_PushButtonLabel, btn, painter, widget);
            }
        }
        return;
    }
    case CE_ScrollBarSubLine:
    case CE_ScrollBarAddLine: {
        // Qt 3 PE_ScrollBarAddLine/SubLine: beveled square button, a shadow
        // outline and a Windows-style three-line arrow.
        const QRect r = option->rect;
        const bool sunken = option->state & State_Sunken;
        drawBevel(painter, r, option->palette, sunken,
                  QBrush(option->palette.color(QPalette::Mid)));
        painter->setPen(option->palette.color(QPalette::Shadow));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);

        PrimitiveElement arrow = PE_IndicatorArrowDown;
        if (element == CE_ScrollBarAddLine) {
            if (option->state & State_Horizontal)
                arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowRight
                                                             : PE_IndicatorArrowLeft;
            else
                arrow = PE_IndicatorArrowDown;
        } else {
            if (option->state & State_Horizontal)
                arrow = option->direction == Qt::LeftToRight ? PE_IndicatorArrowLeft
                                                             : PE_IndicatorArrowRight;
            else
                arrow = PE_IndicatorArrowUp;
        }
        // Mac OS 8 HIG: active scroll arrows are solid black; disabled get
        // the Qt 3 Light etch under Mid.
        drawArrowGlyph(painter, arrow, r.adjusted(2, 2, -2, -2), option->palette,
                       option->state & State_Enabled, QColor(Qt::black));
        return;
    }
    case CE_ScrollBarAddPage:
    case CE_ScrollBarSubPage: {
        drawScrollBarPage(painter, option->rect, option->palette,
                          option->state & State_Horizontal);
        return;
    }
    case CE_ScrollBarSlider: {
        // Appearance Manager: the scroll indicator takes the accent/
        // variation color, with Light/Dark riffles on top.
        const QRect r = option->rect;
        const QColor accent = accentColor(option->palette);
        QPalette face = option->palette;
        face.setColor(QPalette::Button, accent);
        face.setColor(QPalette::Light, accent.lighter(135));
        face.setColor(QPalette::Midlight, accent.lighter(120));
        face.setColor(QPalette::Mid, accent.darker(115));
        face.setColor(QPalette::Dark, accent.darker(130));
        drawBevel(painter, r, face, false, QBrush(accent));
        drawRiffles(painter, r, accent.lighter(160), accent.darker(140),
                    option->state & State_Horizontal);
        painter->setPen(option->palette.color(QPalette::Shadow));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);
        if (option->state & State_HasFocus)
            drawPlatinumFocusRect(painter, r.adjusted(2, 2, -3, -3), option->palette);
        return;
    }
    case CE_TabBarTabShape: {
        // Qt 3 QWindowsStyle::drawControl(CE_TabBarTab), RoundedAbove/Below:
        // tabs are outlined with light/midlight/dark/shadow lines instead of
        // the Qt 6 raised panels. The selected tab merges into the pane by
        // covering its bottom (or top) edge with the window color.
        if (const QStyleOptionTab *tab =
                qstyleoption_cast<const QStyleOptionTab *>(option)) {
            const QRect r = tab->rect;
            const QPalette &pal = tab->palette;
            const bool selected = tab->state & State_Selected;
            const bool firstTab = tab->position == QStyleOptionTab::Beginning
                || tab->position == QStyleOptionTab::OnlyOneTab;
            const bool lastTab = tab->position == QStyleOptionTab::End
                || tab->position == QStyleOptionTab::OnlyOneTab;

            switch (tab->shape) {
            case QTabBar::RoundedNorth: {
                QRect r2 = r;
                painter->setPen(pal.midlight().color());
                painter->drawLine(r2.left(), r2.bottom(), r2.right(), r2.bottom());
                painter->setPen(pal.light().color());
                painter->drawLine(r2.left(), r2.bottom() - 1, r2.right(),
                                  r2.bottom() - 1);
                if (r2.left() == 0) {
                    if (const QTabBar *tb = qobject_cast<const QTabBar *>(widget))
                        painter->drawPoint(tb->rect().bottomLeft());
                }

                if (selected) {
                    painter->fillRect(QRect(r2.left() + 1, r2.bottom() - 1,
                                            r2.width() - 3, 2),
                                      pal.brush(QPalette::Window));
                    painter->setPen(pal.window().color());
                    painter->drawLine(r2.left() + 1, r2.bottom(), r2.left() + 1,
                                      r2.top() + 2);
                    painter->setPen(pal.light().color());
                } else {
                    painter->setPen(pal.light().color());
                }

                int x1 = r2.left();
                int x2 = r2.right() - 2;
                painter->drawLine(x1, r2.bottom() - 1, x1, r2.top() + 2);
                x1++;
                painter->drawPoint(x1, r2.top() + 1);
                x1++;
                painter->drawLine(x1, r2.top(), x2, r2.top());
                if (r2.left() > 0)
                    painter->setPen(pal.midlight().color());
                x1 = r2.left();
                painter->drawPoint(x1, r2.bottom());

                painter->setPen(pal.midlight().color());
                x1++;
                painter->drawLine(x1, r2.bottom(), x1, r2.top() + 2);
                x1++;
                painter->drawLine(x1, r2.top() + 1, x2, r2.top() + 1);

                painter->setPen(pal.dark().color());
                x2 = r2.right() - 1;
                painter->drawLine(x2, r2.top() + 2, x2, r2.bottom() - 1
                                     + (selected ? 1 : -1));
                painter->setPen(pal.shadow().color());
                painter->drawPoint(x2, r2.top() + 1);
                painter->drawPoint(x2, r2.top() + 1);
                x2++;
                painter->drawLine(x2, r2.top() + 2, x2, r2.bottom()
                                     - (selected ? (lastTab ? 0 : 1) : 2));
                return;
            }
            case QTabBar::RoundedSouth: {
                const bool rightAligned =
                    styleHint(SH_TabBar_Alignment, tab, widget) == Qt::AlignRight;
                QRect r2 = r;
                if (selected) {
                    painter->fillRect(QRect(r2.left() + 1, r2.top(),
                                            r2.width() - 3, 1),
                                      pal.brush(QPalette::Window));
                    painter->setPen(pal.window().color());
                    painter->drawLine(r2.left() + 1, r2.top(), r2.left() + 1,
                                      r2.bottom() - 2);
                    painter->setPen(pal.dark().color());
                } else {
                    painter->setPen(pal.shadow().color());
                    painter->drawLine(r2.left() + (rightAligned && firstTab ? 0 : 1),
                                      r2.top() + 1, r2.right() - (lastTab ? 0 : 2),
                                      r2.top() + 1);
                    if (rightAligned && lastTab)
                        painter->drawPoint(r2.right(), r2.top());
                    painter->setPen(pal.dark().color());
                    painter->drawLine(r2.left(), r2.top(), r2.right() - 1, r2.top());
                }

                painter->drawLine(r2.right() - 1, r2.top() + (selected ? 0 : 2),
                                  r2.right() - 1, r2.bottom() - 2);
                painter->drawPoint(r2.right() - 2, r2.bottom() - 2);
                painter->drawLine(r2.right() - 2, r2.bottom() - 1,
                                  r2.left() + 1, r2.bottom() - 1);

                painter->setPen(pal.midlight().color());
                painter->drawLine(r2.left() + 1, r2.bottom() - 2, r2.left() + 1,
                                  r2.top() + (selected ? 0 : 2));

                painter->setPen(pal.shadow().color());
                painter->drawLine(r2.right(),
                                  (r2.top() + (lastTab && rightAligned && selected)) ? 0 : 1,
                                  r2.right(), r2.bottom() - 1);
                painter->drawPoint(r2.right() - 1, r2.bottom() - 1);
                painter->drawLine(r2.right() - 1, r2.bottom(), r2.left() + 2,
                                  r2.bottom());

                painter->setPen(pal.light().color());
                painter->drawLine(r2.left(), r2.top() + (selected ? 0 : 2),
                                  r2.left(), r2.bottom() - 2);
                return;
            }
            case QTabBar::RoundedWest:
            case QTabBar::RoundedEast: {
                // Qt 3 had no west/east tabs; the north shape is transposed
                // by 90 degrees so the base line runs along the outer edge.
                const bool east = tab->shape == QTabBar::RoundedEast;
                QRect r2 = r;
                painter->setPen(pal.midlight().color());
                painter->drawLine(east ? r.left() : r.right(),
                                  r.top(), east ? r.left() : r.right(), r.bottom());
                painter->setPen(pal.light().color());
                painter->drawLine(east ? r.left() + 1 : r.right() - 1,
                                  r.top(), east ? r.left() + 1 : r.right() - 1,
                                  r.bottom());
                if (r.top() == 0) {
                    if (const QTabBar *tb = qobject_cast<const QTabBar *>(widget))
                        painter->drawPoint(east ? tb->rect().topLeft()
                                                : tb->rect().topRight());
                }

                if (selected) {
                    painter->fillRect(east ? QRect(r.left(), r.top() + 1, 2,
                                                   r.height() - 3)
                                           : QRect(r.right() - 1, r.top() + 1, 2,
                                                   r.height() - 3),
                                      pal.brush(QPalette::Window));
                    painter->setPen(pal.window().color());
                    painter->drawLine(east ? r.left() + 1 : r.right() - 1,
                                      r.top() + 1, east ? r.right() - 2 : r.left() + 2,
                                      r.top() + 1);
                    painter->setPen(pal.light().color());
                } else {
                    painter->setPen(pal.light().color());
                }

                const int y1 = r2.top();
                const int y2 = r2.bottom() - 2;
                painter->drawLine(r.left() + 1, y1, r.right() - 2, y1);
                painter->drawPoint(r.top() + 1, y1 + 1);
                painter->drawLine(r.top(), y1 + 2, r.top(), y2);
                if (r2.top() > 0)
                    painter->setPen(pal.midlight().color());
                painter->drawPoint(r.bottom(), y1);
                painter->setPen(pal.midlight().color());
                painter->drawLine(r.bottom(), y1 + 1, r.top() + 2, y1 + 1);
                painter->drawLine(r.top() + 1, y1 + 2, r.top() + 1, y2);

                painter->setPen(pal.dark().color());
                painter->drawLine(r.top() + 2, y2, r.bottom() - 1
                                     + (selected ? 1 : -1), y2);
                painter->setPen(pal.shadow().color());
                painter->drawPoint(r.top() + 1, y2);
                painter->drawPoint(r.top() + 1, y2);
                painter->drawLine(r.top() + 2, y2 + 1, r.bottom()
                                     - (selected ? (lastTab ? 0 : 1) : 2), y2 + 1);
                return;
            }
            default:
                break;
            }
        }
        break;
    }
    case CE_ProgressBarContents: {
        if (const QStyleOptionProgressBar *pb =
                qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            drawProgressBarContents(painter, pb, widget);
            return;
        }
        break;
    }
    case CE_DockWidgetTitle: {
        drawDockWidgetTitle(painter, option);
        return;
    }
    case CE_SizeGrip: {
        drawSizeGrip(painter, option);
        return;
    }
    case CE_Splitter: {
        // Mid face with centered riffles (same grip texture as scroll thumbs).
        const QRect r = option->rect;
        painter->fillRect(r, option->palette.brush(QPalette::Mid));
        drawRiffles(painter, r.adjusted(1, 1, -1, -1), option->palette,
                    !(option->state & State_Horizontal));
        return;
    }
    case CE_MenuBarEmptyArea: {
        drawPrimitive(PE_PanelMenuBar, option, painter, widget);
        return;
    }
    case CE_MenuBarItem: {
        if (const QStyleOptionMenuItem *mi =
                qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const bool selected = mi->state & (State_Selected | State_Sunken);
            if (selected) {
                // Active menu title: sunken Mid bevel, like a pressed Mac
                // menu bar item. Text stays ButtonText (black on Mid).
                drawBevel(painter, mi->rect, mi->palette, true,
                          QBrush(mi->palette.color(QPalette::Mid)));
            } else {
                // Items are shorter than the bar (VMargin). Painting
                // PE_PanelMenuBar here would put a Dark edge on the item
                // bottom *and* EmptyArea would put another on the bar
                // bottom — a double rule. Face only; EmptyArea owns the
                // single Dark baseline (it also fills the margin strips).
                painter->fillRect(mi->rect, mi->palette.brush(QPalette::Button));
            }
            drawItemText(painter, mi->rect, Qt::AlignCenter | Qt::TextShowMnemonic,
                         mi->palette, mi->state & State_Enabled, mi->text,
                         QPalette::ButtonText);
        }
        return;
    }
    case CE_MenuItem: {
        if (const QStyleOptionMenuItem *mi =
                qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const QRect r = mi->rect;
            const QPalette &pal = mi->palette;
            if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
                const int y = r.center().y();
                painter->setPen(pal.color(QPalette::Dark));
                painter->drawLine(r.left() + 2, y, r.right() - 2, y);
                painter->setPen(pal.color(QPalette::Light));
                painter->drawLine(r.left() + 2, y + 1, r.right() - 2, y + 1);
                return;
            }
            const bool selected = (mi->state & State_Selected) && (mi->state & State_Enabled);
            if (selected)
                painter->fillRect(r, accentColor(pal));
            else
                painter->fillRect(r, pal.brush(QPalette::Button));

            const int checkCol = qMax(mi->maxIconWidth, 16);
            QRect checkRect(r.x() + 2, r.y() + 1, checkCol, r.height() - 2);
            if (!mi->icon.isNull()) {
                const QIcon::Mode mode = !(mi->state & State_Enabled) ? QIcon::Disabled
                    : (selected ? QIcon::Active : QIcon::Normal);
                const int smallIconSize = pixelMetric(PM_SmallIconSize, mi, widget);
                const QSize iconSize(smallIconSize, smallIconSize);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                const QPixmap pm = mi->icon.pixmap(iconSize, QStyleHelper::getDpr(painter), mode,
                                                   mi->checked ? QIcon::On : QIcon::Off);
#else
                const QPixmap pm = mi->icon.pixmap(iconSize, mode,
                                                   mi->checked ? QIcon::On : QIcon::Off);
#endif
                QRect ir(0, 0, pm.width() / pm.devicePixelRatio(),
                         pm.height() / pm.devicePixelRatio());
                ir.moveCenter(checkRect.center());
                painter->drawPixmap(ir.topLeft(), pm);
            } else if (mi->checked
                       && mi->checkType != QStyleOptionMenuItem::NotCheckable) {
                // drawCheckMark is a fixed 13×13 glyph — center it in the column.
                QRect mark(0, 0, 13, 13);
                mark.moveCenter(checkRect.center());
                drawCheckMark(painter, mark,
                              selected ? pal.color(QPalette::HighlightedText)
                                       : pal.color(QPalette::Text),
                              selected ? accentColor(pal).darker(120)
                                       : pal.color(QPalette::Dark));
            }

            const int tab = menuItemTabWidth(mi);
            const bool hasSubMenu = mi->menuItemType == QStyleOptionMenuItem::SubMenu;
            const int arrowSpace = hasSubMenu ? 12 : 0;
            const int rightReserve = 8 + (tab > 0 ? tab : 0) + arrowSpace;
            QRect textRect = r.adjusted(checkCol + 6, 1, -rightReserve, -1);
            int flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip
                | Qt::TextSingleLine;
            if (!styleHint(SH_UnderlineShortcut, mi, widget))
                flags |= Qt::TextHideMnemonic;
            QString text = mi->text;
            const int tabPos = text.indexOf(QLatin1Char('\t'));
            const QPalette::ColorRole textRole = selected
                ? QPalette::HighlightedText : QPalette::ButtonText;
            if (tabPos >= 0 && tab > 0) {
                // Shortcut strip sits left of the submenu arrow, if any.
                const QRect shortcutRect(r.right() - arrowSpace - tab - 4, r.y(),
                                         tab, r.height());
                drawItemText(painter, shortcutRect, flags | Qt::AlignRight, pal,
                             mi->state & State_Enabled, text.mid(tabPos + 1), textRole);
                text = text.left(tabPos);
            }
            drawItemText(painter, textRect, flags, pal, mi->state & State_Enabled,
                         text, textRole);

            if (hasSubMenu) {
                const QColor arrowColor = selected
                    ? pal.color(QPalette::HighlightedText)
                    : pal.color(QPalette::ButtonText);
                drawArrowGlyph(painter,
                               mi->direction == Qt::RightToLeft ? PE_IndicatorArrowLeft
                                                                : PE_IndicatorArrowRight,
                               QRect(r.right() - 12, r.y() + 2, 10, r.height() - 4),
                               pal, mi->state & State_Enabled, arrowColor);
            }
        }
        return;
    }
    case CE_ToolBar: {
        drawPrimitive(PE_PanelToolBar, option, painter, widget);
        return;
    }
    default:
        break;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void PlatinumStyle::drawProgressBarContents(QPainter *painter,
                                            const QStyleOptionProgressBar *pb,
                                            const QWidget *widget) const
{
    // Qt 3 CE_ProgressBarContents, busy branch: a bar with no steps shows a
    // string of highlight chunks (the same dotted-block style as the
    // deterministic chunk grid) sweeping back and forth across the trough,
    // instead of the Qt 6 chunk animation.
    if (pb->minimum == 0 && pb->maximum == 0) {
        const bool vertical = !(pb->state & QStyle::State_Horizontal);
        QRect rect = pb->rect;
        QTransform m;
        if (vertical) {
            rect = QRect(rect.y(), rect.x(), rect.height(), rect.width());
            m.rotate(90);
            m.translate(0, -(rect.height() + rect.y() * 2));
        }
        bool reverse = (!vertical && pb->direction == Qt::RightToLeft) || vertical;
        if (pb->invertedAppearance)
            reverse = !reverse;

        // 扫动相位由单调钟驱动（2400ms 一个完整来回），与 bluecurve 一致。
        // 动画本身由 QTimer 负责触发重绘，这里只算位置。
        const int t = int(steadyMs() % 2400);

        // 与正常进度条的 chunk 网格同款：每块宽 PM_ProgressBarChunkWidth、
        // 高内缩 3px、块间留 2px 缝。整串整体滑动。
        const int cw = pixelMetric(PM_ProgressBarChunkWidth, pb, widget);
        const int gap = 2;
        // 串宽取槽宽的 1/4（至少 1 块），但绝不超槽宽，否则超窄条会溢出。
        const int maxBlocks = qMax(1, (rect.width() + gap) / (cw + gap));
        const int n = qMin(maxBlocks, qMax(1, rect.width() / 4 / (cw + gap)));
        const int sw = n * cw + (n - 1) * gap;
        const int remains = qMax(rect.width() - sw, 1);
        const int period = 2400;
        int x = (t < period / 2) ? t * remains / (period / 2)
                                 : (period - t) * remains / (period / 2);
        x = reverse ? rect.right() - x - sw : x + rect.x();

        painter->save();
        painter->setTransform(m, true);
        painter->setBrush(accentColor(pb->palette));
        painter->setPen(Qt::NoPen);
        for (int i = 0; i < n; ++i) {
            const int cx = x + i * (cw + gap);
            // 与 PE_IndicatorProgressChunk 同款：上下内缩 3px、右侧留 2px。
            painter->drawRect(cx, rect.y() + 3, cw - 2, rect.height() - 6);
        }
        painter->restore();
        return;
    }

    // Deterministic bars go through the base style, which feeds the Qt 3
    // chunk grid through PE_IndicatorProgressChunk. The event filter stops
    // the busy timer as soon as a bar gets a deterministic range.
    QProxyStyle::drawControl(CE_ProgressBarContents, pb, painter, widget);
}

void PlatinumStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                       QPainter *painter, const QWidget *widget) const
{
    if (control == CC_GroupBox) {
        drawGroupBox(painter, option, widget);
        return;
    }
    if (control == CC_ComboBox) {
        if (const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            const QRect r = cmb->rect;
            const QPalette &pal = cmb->palette;
            const int x = r.x(), y = r.y(), w = r.width(), h = r.height();
            const QColor button = pal.color(QPalette::Button);
            const QColor light = pal.color(QPalette::Light);
            const QColor mid = pal.color(QPalette::Mid);
            const QColor dark = pal.color(QPalette::Dark);
            const QColor shadow = pal.color(QPalette::Shadow);
            const QColor background = pal.color(QPalette::Window);

            painter->save();
            // Qt 3 combo panel: a small square bevel with corner dots.
            painter->fillRect(x + 2, y + 2, w - 4, h - 4, button);
            painter->setPen(shadow);
            painter->drawLine(x, y, x + w - 1, y);
            painter->drawLine(x, y, x, y + h - 1);
            painter->setPen(light);
            painter->drawLine(x + 1, y + 1, x + w - 2, y + 1);
            painter->drawLine(x + 1, y + 1, x + 1, y + h - 2);
            painter->setPen(mid);
            painter->drawLine(x + 2, y + h - 2, x + w - 2, y + h - 2);
            painter->drawLine(x + w - 2, y + 2, x + w - 2, y + h - 2);
            painter->setPen(shadow);
            painter->drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
            painter->drawLine(x + w - 1, y, x + w - 1, y + h - 1);
            // top-left corner
            painter->setPen(background);
            painter->drawPoint(x, y);
            painter->drawPoint(x + 1, y);
            painter->drawPoint(x, y + 1);
            painter->setPen(shadow);
            painter->drawPoint(x + 1, y + 1);
            painter->setPen(Qt::white);
            painter->drawPoint(x + 3, y + 3);
            // bottom-left corner
            painter->setPen(background);
            painter->drawPoint(x, y + h - 1);
            painter->drawPoint(x + 1, y + h - 1);
            painter->drawPoint(x, y + h - 2);
            painter->setPen(shadow);
            painter->drawPoint(x + 1, y + h - 2);
            // top-right corner
            painter->setPen(background);
            painter->drawPoint(x + w - 1, y);
            painter->drawPoint(x + w - 2, y);
            painter->drawPoint(x + w - 1, y + 1);
            painter->setPen(shadow);
            painter->drawPoint(x + w - 2, y + 1);
            // bottom-right corner
            painter->setPen(background);
            painter->drawPoint(x + w - 1, y + h - 1);
            painter->drawPoint(x + w - 2, y + h - 1);
            painter->drawPoint(x + w - 1, y + h - 2);
            painter->setPen(shadow);
            painter->drawPoint(x + w - 2, y + h - 2);
            painter->setPen(dark);
            painter->drawPoint(x + w - 3, y + h - 3);

            if (cmb->subControls & SC_ComboBoxArrow) {
                const QRect ar = subControlRect(CC_ComboBox, cmb, SC_ComboBoxArrow, widget);
                if (ar.isValid() && ar.width() > 4 && ar.height() > 4) {
                    const int xx = ar.x(), yy = ar.y(), ww = ar.width(), hh = ar.height();
                    // separator and the beveled arrow button
                    painter->setPen(mid);
                    painter->drawLine(xx, yy + 2, xx, yy + hh - 3);
                    painter->setPen(button);
                    painter->drawLine(xx + 1, yy + 1, xx + ww - 2, yy + 1);
                    painter->drawLine(xx + 1, yy + 1, xx + 1, yy + hh - 2);
                    painter->setPen(light);
                    painter->drawLine(xx + 2, yy + 2, xx + 2, yy + hh - 2);
                    painter->drawLine(xx + 2, yy + 2, xx + ww - 2, yy + 2);
                    painter->setPen(mid);
                    painter->drawLine(xx + 3, yy + hh - 3, xx + ww - 3, yy + hh - 3);
                    painter->drawLine(xx + ww - 3, yy + 3, xx + ww - 3, yy + hh - 3);
                    painter->setPen(dark);
                    painter->drawLine(xx + 2, yy + hh - 2, xx + ww - 2, yy + hh - 2);
                    painter->drawLine(xx + ww - 2, yy + 2, xx + ww - 2, yy + hh - 2);
                    painter->setPen(shadow);
                    painter->drawLine(xx + 1, yy + hh - 1, xx + ww - 1, yy + hh - 1);
                    painter->drawLine(xx + ww - 1, yy, xx + ww - 1, yy + hh - 1);
                    // top-right corner
                    painter->setPen(background);
                    painter->drawPoint(xx + ww - 1, yy);
                    painter->drawPoint(xx + ww - 2, yy);
                    painter->drawPoint(xx + ww - 1, yy + 1);
                    painter->setPen(shadow);
                    painter->drawPoint(xx + ww - 2, yy + 1);
                    // bottom-right corner
                    painter->setPen(background);
                    painter->drawPoint(xx + ww - 1, yy + hh - 1);
                    painter->drawPoint(xx + ww - 2, yy + hh - 1);
                    painter->drawPoint(xx + ww - 1, yy + hh - 2);
                    painter->setPen(shadow);
                    painter->drawPoint(xx + ww - 2, yy + hh - 2);
                    painter->setPen(dark);
                    painter->drawPoint(xx + ww - 3, yy + hh - 3);
                    painter->setPen(mid);
                    painter->drawPoint(xx + ww - 4, yy + hh - 4);

                    // Qt 3: two stacked three-line arrows (up and down).
                    // The up arrow is centered at (cx, cy-3), the down arrow
                    // at (cx, cy+2). The three shaft lines get shorter towards
                    // the tip and the tip itself is a single point.
                    const QColor fg = pal.color(QPalette::Text);
                    painter->setPen(fg);
                    const int cx = xx + ww / 2;
                    const int cy = yy + hh / 2;
                    painter->drawLine(cx - 3, cy - 2, cx + 3, cy - 2);
                    painter->drawLine(cx - 2, cy - 3, cx + 2, cy - 3);
                    painter->drawLine(cx - 1, cy - 4, cx + 1, cy - 4);
                    painter->drawPoint(cx, cy - 5);
                    painter->drawLine(cx - 3, cy + 1, cx + 3, cy + 1);
                    painter->drawLine(cx - 2, cy + 2, cx + 2, cy + 2);
                    painter->drawLine(cx - 1, cy + 3, cx + 1, cy + 3);
                    painter->drawPoint(cx, cy + 4);
                }
            }
            painter->restore();

            if (cmb->subControls & SC_ComboBoxEditField) {
                const QRect er = subControlRect(CC_ComboBox, cmb, SC_ComboBoxEditField, widget);
                QStyleOptionComboBox sub = *cmb;
                sub.subControls = SC_ComboBoxEditField;
                if (!cmb->editable && cmb->state & State_HasFocus) {
                    painter->fillRect(er.adjusted(1, 1, -1, -1), pal.color(QPalette::Highlight));
                    sub.palette.setColor(QPalette::ButtonText, pal.color(QPalette::HighlightedText));
                    // Qt 3: a focused, non-editable combo also gets the focus
                    // frame around the edit field.
                    QStyleOptionFocusRect focus;
                    focus.QStyleOption::operator=(*cmb);
                    focus.rect = subElementRect(SE_ComboBoxFocusRect, cmb, widget);
                    drawPrimitive(PE_FrameFocusRect, &focus, painter, widget);
                }
                if (cmb->editable) {
                    // Qt 3: the edit field is a recessed panel (qDrawShadePanel,
                    // sunken, 2 pixels).
                    painter->fillRect(er, pal.color(QPalette::Window));
                    drawSunkenPanel(painter, er, pal);
                }
                QProxyStyle::drawControl(CE_ComboBoxLabel, &sub, painter, widget);
            }
        }
        return;
    }
    if (control == CC_Slider) {
        if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const QPalette &pal = slider->palette;
            const bool horizontal = slider->orientation == Qt::Horizontal;
            const int thickness = pixelMetric(PM_SliderControlThickness, slider, widget);
            const int len = pixelMetric(PM_SliderLength, slider, widget);

            if (slider->subControls & SC_SliderGroove) {
                const QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
                if (groove.isValid() && groove.width() > 2 && groove.height() > 2) {
                    painter->fillRect(groove, pal.color(QPalette::Window));

                    // Qt 3: a 7-pixel recessed slot, offset by the tick
                    // marks, drawn in Dark with bevel shading.
                    int mid = thickness / 2;
                    if (slider->tickPosition & QSlider::TicksAbove)
                        mid += len / 8;
                    if (slider->tickPosition & QSlider::TicksBelow)
                        mid -= len / 8;

                    const QRect trough = horizontal
                        ? QRect(0, groove.y() + mid - 3, slider->rect.width(), 7)
                        : QRect(groove.x() + mid - 3, 0, 7, slider->rect.height());
                    const int x = trough.x(), y = trough.y(), tw = trough.width(),
                              th = trough.height();
                    painter->fillRect(trough, pal.color(QPalette::Dark));
                    painter->setPen(pal.color(QPalette::Dark));
                    painter->drawLine(x, y, x + tw - 1, y);
                    painter->drawLine(x, y, x, y + th - 1);
                    painter->setPen(pal.color(QPalette::Shadow));
                    painter->drawLine(x + 1, y + 1, x + tw - 2, y + 1);
                    painter->drawLine(x + 1, y + 1, x + 1, y + th - 2);
                    painter->setPen(pal.color(QPalette::Shadow));
                    painter->drawLine(x + 1, y + th - 2, x + tw - 2, y + th - 2);
                    painter->drawLine(x + tw - 2, y + 1, x + tw - 2, y + th - 2);
                    painter->setPen(pal.color(QPalette::Light));
                    painter->drawLine(x, y + th - 1, x + tw - 1, y + th - 1);
                    painter->drawLine(x + tw - 1, y, x + tw - 1, y + th - 1);
                    // corner dots
                    painter->setPen(pal.color(QPalette::Window));
                    painter->drawPoint(x, y);
                    painter->drawPoint(x + 1, y);
                    painter->drawPoint(x, y + 1);
                    painter->setPen(pal.color(QPalette::Shadow));
                    painter->drawPoint(x + 1, y + 1);
                    painter->setPen(pal.color(QPalette::Window));
                    painter->drawPoint(x, y + th - 1);
                    painter->drawPoint(x + 1, y + th - 1);
                    painter->drawPoint(x, y + th - 2);
                    painter->setPen(pal.color(QPalette::Light));
                    painter->drawPoint(x + 1, y + th - 2);
                    painter->setPen(pal.color(QPalette::Window));
                    painter->drawPoint(x + tw - 1, y);
                    painter->drawPoint(x + tw - 2, y);
                    painter->drawPoint(x + tw - 1, y + 1);
                    painter->setPen(pal.color(QPalette::Dark));
                    painter->drawPoint(x + tw - 2, y + 1);
                    painter->setPen(pal.color(QPalette::Window));
                    painter->drawPoint(x + tw - 1, y + th - 1);
                    painter->drawPoint(x + tw - 2, y + th - 1);
                    painter->drawPoint(x + tw - 1, y + th - 2);
                    painter->setPen(pal.color(QPalette::Light));
                    painter->drawPoint(x + tw - 2, y + th - 2);
                    painter->setPen(pal.color(QPalette::Dark));
                    painter->drawPoint(x + tw - 3, y + th - 3);

                    if (slider->state & State_HasFocus)
                        drawPlatinumFocusRect(painter, groove, pal);
                }
            }

            if (slider->subControls & SC_SliderHandle) {
                const QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle, widget);
                if (handle.isValid()) {
                    // Accent-colored hexagonal handle (Appearance Manager
                    // variation), shadow outline, light top-left highlight,
                    // riffles in the middle.
                    const QColor accent = accentColor(pal);
                    const int x1 = handle.x(), y1 = handle.y();
                    const int x2 = handle.x() + handle.width() - 1;
                    const int y2 = handle.y() + handle.height() - 1;
                    const int mx = handle.width() / 2;
                    const int my = handle.height() / 2;

                    painter->save();
                    if (horizontal) {
                        QPolygon hex;
                        hex << QPoint(x2 - 1, y1 + 1) << QPoint(x2 - 1, y2 - mx + 2)
                            << QPoint(x2 - mx + 1, y2 - 1) << QPoint(x1 + mx - 1, y2 - 1)
                            << QPoint(x1 + 1, y2 - mx + 2) << QPoint(x1 + 1, y1 + 1);
                        painter->setPen(Qt::NoPen);
                        painter->setBrush(accent);
                        painter->drawPolygon(hex);
                        painter->setPen(pal.color(QPalette::Shadow));
                        painter->drawLine(x1 + 1, y1, x2 - 1, y1);
                        painter->drawLine(x1, y2 - mx + 2, x1 + mx - 2, y2);
                        painter->drawLine(x2, y2 - mx + 2, x1 + mx + 2, y2);
                        painter->drawLine(x1 + mx - 2, y2, x1 + mx + 2, y2);
                        painter->drawLine(x1, y1 + 1, x1, y2 - mx + 2);
                        painter->drawLine(x2, y1 + 1, x2, y2 - mx + 2);
                        painter->setPen(accent.lighter(150));
                        painter->drawLine(x1 + 1, y1 + 1, x2 - 1, y1 + 1);
                        painter->drawLine(x1 + 1, y1 + 1, x1 + 1, y2 - mx + 2);
                        painter->setPen(accent.darker(130));
                        painter->drawLine(x2 - 1, y1 + 1, x2 - 1, y2 - mx + 2);
                        painter->drawLine(x1 + 1, y2 - mx + 2, x1 + mx - 2, y2 - 1);
                        painter->drawLine(x2 - 1, y2 - mx + 2, x1 + mx + 2, y2 - 1);
                        painter->drawLine(x1 + mx - 2, y2 - 1, x1 + mx + 2, y2 - 1);
                        drawRiffles(painter,
                                    QRect(handle.x() + 2, handle.y(), handle.width() - 4,
                                          handle.height() - 5),
                                    accent.lighter(160), accent.darker(140), false);
                    } else {
                        QPolygon hex;
                        hex << QPoint(x1 + 1, y1 + 1) << QPoint(x2 - my + 2, y1 + 1)
                            << QPoint(x2 - 1, y1 + my - 1) << QPoint(x2 - 1, y2 - my + 1)
                            << QPoint(x2 - my + 2, y2 - 1) << QPoint(x1 + 1, y2 - 1);
                        painter->setPen(Qt::NoPen);
                        painter->setBrush(accent);
                        painter->drawPolygon(hex);
                        painter->setPen(pal.color(QPalette::Shadow));
                        painter->drawLine(x1, y1 + 1, x1, y2 - 1);
                        painter->drawLine(x2 - my + 2, y1, x2, y1 + my - 2);
                        painter->drawLine(x2 - my + 2, y2, x2, y1 + my + 2);
                        painter->drawLine(x2, y1 + my - 2, x2, y1 + my + 2);
                        painter->drawLine(x1 + 1, y1, x2 - my + 2, y1);
                        painter->drawLine(x1 + 1, y2, x2 - my + 2, y2);
                        painter->setPen(accent.lighter(150));
                        painter->drawLine(x1 + 1, y1 + 2, x1 + 1, y2 - 2);
                        painter->drawLine(x1 + 1, y1 + 1, x2 - my + 2, y1 + 1);
                        painter->drawLine(x2 - my + 2, y1 + 1, x2 - 1, y1 + my - 2);
                        painter->setPen(accent.darker(130));
                        painter->drawLine(x2 - 1, y1 + my - 2, x2 - 1, y1 + my + 2);
                        painter->drawLine(x2 - my + 2, y2 - 1, x2 - 1, y1 + my + 2);
                        painter->drawLine(x1 + 1, y2 - 1, x2 - my + 2, y2 - 1);
                        drawRiffles(painter,
                                    QRect(handle.x(), handle.y() + 2, handle.width() - 3,
                                          handle.height() - 4),
                                    accent.lighter(160), accent.darker(140), true);
                    }
                    painter->restore();
                }
            }

            if (slider->subControls & SC_SliderTickmarks) {
                QStyleOptionSlider sub = *slider;
                sub.subControls = SC_SliderTickmarks;
                QProxyStyle::drawComplexControl(CC_Slider, &sub, painter, widget);
            }
        }
        return;
    }
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

QRect PlatinumStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                    SubControl subControl, const QWidget *widget) const
{
    if (control == CC_ScrollBar) {
        // Qt 3 QPlatinumStyle layout: both arrow buttons sit side by side at
        // the trailing end of the bar.
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = sb->orientation == Qt::Horizontal;
            const int sbextent = pixelMetric(PM_ScrollBarExtent, sb, widget);
            const QRect r = sb->rect;
            const int len = horizontal ? r.width() : r.height();
            const int maxlen = len - 2 * sbextent;

            int sliderlen;
            const qint64 range = qint64(sb->maximum) - qint64(sb->minimum);
            if (range > 0) {
                sliderlen = int(sb->pageStep * qint64(maxlen) / (range + sb->pageStep));
                const int slidermin = pixelMetric(PM_ScrollBarSliderMin, sb, widget);
                if (sliderlen < slidermin || range > qint64(INT_MAX) / 2)
                    sliderlen = slidermin;
                if (sliderlen > maxlen)
                    sliderlen = maxlen;
            } else {
                sliderlen = maxlen;
            }
            const int sliderStart = QStyle::sliderPositionFromValue(
                sb->minimum, sb->maximum, sb->sliderPosition, maxlen - sliderlen,
                sb->upsideDown);

            QRect ret;
            switch (subControl) {
            case SC_ScrollBarSubLine:
                if (horizontal) {
                    const int buttonw = qMin(len / 2, sbextent);
                    ret = QRect(r.x() + len - 2 * buttonw, r.y(), buttonw, sbextent);
                } else {
                    const int buttonh = qMin(len / 2, sbextent);
                    ret = QRect(r.x(), r.y() + len - 2 * buttonh, sbextent, buttonh);
                }
                break;
            case SC_ScrollBarAddLine:
                if (horizontal) {
                    const int buttonw = qMin(len / 2, sbextent);
                    ret = QRect(r.x() + len - buttonw, r.y(), buttonw, sbextent);
                } else {
                    const int buttonh = qMin(len / 2, sbextent);
                    ret = QRect(r.x(), r.y() + len - buttonh, sbextent, buttonh);
                }
                break;
            case SC_ScrollBarSubPage:
                if (horizontal)
                    ret = QRect(r.x() + 1, r.y(), sliderStart, sbextent);
                else
                    ret = QRect(r.x(), r.y() + 1, sbextent, sliderStart);
                break;
            case SC_ScrollBarAddPage:
                if (horizontal)
                    ret = QRect(r.x() + sliderStart + sliderlen, r.y(),
                                maxlen - sliderStart - sliderlen, sbextent);
                else
                    ret = QRect(r.x(), r.y() + sliderStart + sliderlen, sbextent,
                                maxlen - sliderStart - sliderlen);
                break;
            case SC_ScrollBarGroove:
                if (horizontal)
                    ret = QRect(r.x() + 1, r.y(), len - 2 * sbextent, r.height());
                else
                    ret = QRect(r.x(), r.y() + 1, r.width(), len - 2 * sbextent);
                break;
            case SC_ScrollBarSlider:
                if (horizontal)
                    ret = QRect(r.x() + sliderStart, r.y(), sliderlen, sbextent);
                else
                    ret = QRect(r.x(), r.y() + sliderStart, sbextent, sliderlen);
                break;
            default:
                return QProxyStyle::subControlRect(control, option, subControl, widget);
            }
            return visualRect(sb->direction, sb->rect, ret);
        }
    }
    if (control == CC_ComboBox && subControl == SC_ComboBoxArrow) {
        // Qt 3: a fixed 20-pixel arrow button at the trailing end.
        const QRect r = option->rect;
        const QRect ret(option->direction == Qt::RightToLeft
                            ? QRect(r.x(), r.y(), 20, r.height())
                            : QRect(r.x() + r.width() - 20, r.y(), 20, r.height()));
        return visualRect(option->direction, r, ret);
    }
    return QProxyStyle::subControlRect(control, option, subControl, widget);
}

QRect PlatinumStyle::subElementRect(SubElement element, const QStyleOption *option,
                                    const QWidget *widget) const
{
    if (element == SE_ComboBoxFocusRect) {
        // Qt 3 SR_ComboBoxFocusRect: inset 4 px with 16 px carved off the
        // trailing edge for the arrow button.
        const QRect r = option->rect;
        return QRect(r.x() + 4, r.y() + 4, r.width() - 8 - 16, r.height() - 8);
    }
    return QProxyStyle::subElementRect(element, option, widget);
}

int PlatinumStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                               const QWidget *widget) const
{
    switch (metric) {
    case PM_ButtonDefaultIndicator:
        return qRound(QStyleHelper::dpiScaled(3, option));
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        // Mac buttons do not shift their label when pressed; the face
        // darkening is feedback enough.
        return 0;
    case PM_IndicatorWidth:
        // Qt 3: 15 x 13 check box indicator.
        return qRound(QStyleHelper::dpiScaled(15, option));
    case PM_IndicatorHeight:
        return qRound(QStyleHelper::dpiScaled(13, option));
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        return qRound(QStyleHelper::dpiScaled(15, option));
    case PM_SliderLength:
        return qRound(QStyleHelper::dpiScaled(17, option));
    case PM_SliderControlThickness:
        // Qt 3 QWindowsStyle: the business part of the slider is the whole
        // thickness when there are no ticks, otherwise 6 px plus a share of
        // the remaining space.
        if (const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int space = sl->orientation == Qt::Horizontal ? sl->rect.height()
                                                          : sl->rect.width();
            const int ticks = sl->tickPosition;
            int n = 0;
            if (ticks & QSlider::TicksAbove)
                ++n;
            if (ticks & QSlider::TicksBelow)
                ++n;
            if (!n)
                return space;
            int thick = qRound(QStyleHelper::dpiScaled(6, option));
            if (ticks != QSlider::TicksBothSides && ticks != QSlider::NoTicks)
                thick += pixelMetric(PM_SliderLength, option, widget) / 4;
            space -= thick;
            if (space > 0)
                thick += (space * 2) / (n + 2);
            return thick;
        }
        return qRound(QStyleHelper::dpiScaled(11, option));
    case PM_ScrollBarSliderMin:
        return qRound(QStyleHelper::dpiScaled(25, option));
    case PM_HeaderMarkSize:
        // Mac OS 9 sort mark is a compact filled triangle (~7 px).
        return qRound(QStyleHelper::dpiScaled(9, option));
    case PM_HeaderMargin:
        return qRound(QStyleHelper::dpiScaled(4, option));
    case PM_ToolBarHandleExtent:
        return qRound(QStyleHelper::dpiScaled(10, option));
    case PM_ToolBarSeparatorExtent:
        return qRound(QStyleHelper::dpiScaled(8, option));
    case PM_DockWidgetTitleBarButtonMargin:
        return qRound(QStyleHelper::dpiScaled(2, option));
    case PM_MaximumDragDistance:
        return -1;
    default:
        break;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
}

void PlatinumStyle::drawBevel(QPainter *p, const QRect &r, const QPalette &palette,
                              bool sunken, const QBrush &fill) const
{
    const int x = r.x(), y = r.y(), w = r.width(), h = r.height();
    if (w < 2 || h < 2)
        return;
    const QColor button = palette.color(QPalette::Button);
    const QColor light = palette.color(QPalette::Light);
    const QColor midlight = palette.color(QPalette::Midlight);
    const QColor mid = palette.color(QPalette::Mid);
    const QColor dark = palette.color(QPalette::Dark);

    p->save();
    if (w * h < 1600 || qAbs(w - h) > 10) {
        // Qt 3 small bevel.
        if (!sunken) {
            p->fillRect(x + 2, y + 2, w - 4, h - 4, button);
            p->setPen(dark);
            p->drawLine(x, y, x + w - 1, y);
            p->drawLine(x, y, x, y + h - 1);
            p->setPen(light);
            p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
            p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
            p->setPen(mid);
            p->drawLine(x + 2, y + h - 2, x + w - 2, y + h - 2);
            p->drawLine(x + w - 2, y + 2, x + w - 2, y + h - 3);
            p->setPen(dark.darker());
            p->drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
            p->drawLine(x + w - 1, y + 1, x + w - 1, y + h - 2);
        } else {
            p->fillRect(x + 2, y + 2, w - 4, h - 4, fill);
            p->setPen(dark.darker());
            p->drawLine(x, y, x + w - 1, y);
            p->drawLine(x, y, x, y + h - 1);
            p->setPen(mid.darker());
            p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
            p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
            p->setPen(button);
            p->drawLine(x + 1, y + h - 2, x + w - 2, y + h - 2);
            p->drawLine(x + w - 2, y + 1, x + w - 2, y + h - 2);
            p->setPen(dark);
            p->drawLine(x, y + h - 1, x + w - 1, y + h - 1);
            p->drawLine(x + w - 1, y, x + w - 1, y + h - 1);
        }
    } else {
        // Qt 3 large bevel.
        if (!sunken) {
            p->fillRect(x + 3, y + 3, w - 6, h - 6, button);
            p->setPen(button.darker());
            p->drawLine(x, y, x + w - 1, y);
            p->drawLine(x, y, x, y + h - 1);
            p->setPen(button);
            p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
            p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
            p->setPen(light);
            p->drawLine(x + 2, y + 2, x + 2, y + h - 2);
            p->drawLine(x + 2, y + 2, x + w - 2, y + 2);
            p->setPen(mid);
            p->drawLine(x + 3, y + h - 3, x + w - 3, y + h - 3);
            p->drawLine(x + w - 3, y + 3, x + w - 3, y + h - 3);
            p->setPen(dark);
            p->drawLine(x + 2, y + h - 2, x + w - 2, y + h - 2);
            p->drawLine(x + w - 2, y + 2, x + w - 2, y + h - 2);
            p->setPen(dark.darker());
            p->drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
            p->drawLine(x + w - 1, y + 1, x + w - 1, y + h - 1);
            // corner dots
            p->setPen(mixedColor(dark.darker().darker(), dark));
            p->drawPoint(x, y + h - 1);
            p->drawPoint(x + w - 1, y);
            p->setPen(mixedColor(dark.darker(), midlight));
            p->drawPoint(x + 1, y + h - 2);
            p->drawPoint(x + w - 2, y + 1);
            p->setPen(mixedColor(mid.darker(), button));
            p->drawPoint(x + 2, y + h - 3);
            p->drawPoint(x + w - 3, y + 2);
        } else {
            p->fillRect(x + 3, y + 3, w - 6, h - 6, fill);
            p->setPen(dark.darker().darker());
            p->drawLine(x, y, x + w - 1, y);
            p->drawLine(x, y, x, y + h - 1);
            p->setPen(dark.darker());
            p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
            p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
            p->setPen(mid.darker());
            p->drawLine(x + 2, y + 2, x + 2, y + h - 2);
            p->drawLine(x + 2, y + 2, x + w - 2, y + 2);
            p->setPen(button);
            p->drawLine(x + 2, y + h - 3, x + w - 3, y + h - 3);
            p->drawLine(x + w - 3, y + 3, x + w - 3, y + h - 3);
            p->setPen(midlight);
            p->drawLine(x + 1, y + h - 2, x + w - 2, y + h - 2);
            p->drawLine(x + w - 2, y + 1, x + w - 2, y + h - 2);
            p->setPen(dark);
            p->drawLine(x, y + h - 1, x + w - 1, y + h - 1);
            p->drawLine(x + w - 1, y, x + w - 1, y + h - 1);
            // corner dots
            p->setPen(mixedColor(dark.darker().darker(), dark));
            p->drawPoint(x, y + h - 1);
            p->drawPoint(x + w - 1, y);
            p->setPen(mixedColor(dark.darker(), midlight));
            p->drawPoint(x + 1, y + h - 2);
            p->drawPoint(x + w - 2, y + 1);
            p->setPen(mixedColor(mid.darker(), button));
            p->drawPoint(x + 2, y + h - 3);
            p->drawPoint(x + w - 3, y + 2);
        }
    }
    p->restore();
}

void PlatinumStyle::drawCommandButton(QPainter *p, const QRect &r, const QPalette &palette,
                                      bool sunken) const
{
    const int x = r.x(), y = r.y(), w = r.width(), h = r.height();
    if (w < 5 || h < 5) {
        p->fillRect(r, palette.color(sunken ? QPalette::Dark : QPalette::Button));
        return;
    }
    const QColor button = palette.color(QPalette::Button);
    const QColor light = palette.color(QPalette::Light);
    const QColor mid = palette.color(QPalette::Mid);
    const QColor dark = palette.color(QPalette::Dark);
    const QColor shadow = palette.color(QPalette::Shadow);
    const QColor background = palette.color(QPalette::Window);

    p->save();
    if (!sunken) {
        p->fillRect(x + 3, y + 3, w - 6, h - 6, button);
        // the bright side
        p->setPen(shadow);
        p->drawLine(x, y, x + w - 1, y);
        p->drawLine(x, y, x, y + h - 1);
        p->setPen(button);
        p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
        p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
        p->setPen(light);
        p->drawLine(x + 2, y + 2, x + 2, y + h - 2);
        p->drawLine(x + 2, y + 2, x + w - 2, y + 2);
        // the dark side
        p->setPen(mid);
        p->drawLine(x + 3, y + h - 3, x + w - 3, y + h - 3);
        p->drawLine(x + w - 3, y + 3, x + w - 3, y + h - 3);
        p->setPen(dark);
        p->drawLine(x + 2, y + h - 2, x + w - 2, y + h - 2);
        p->drawLine(x + w - 2, y + 2, x + w - 2, y + h - 2);
        p->setPen(shadow);
        p->drawLine(x + 1, y + h - 1, x + w - 1, y + h - 1);
        p->drawLine(x + w - 1, y, x + w - 1, y + h - 1);
        // top-left corner
        p->setPen(background);
        p->drawPoint(x, y);
        p->drawPoint(x + 1, y);
        p->drawPoint(x, y + 1);
        p->setPen(shadow);
        p->drawPoint(x + 1, y + 1);
        p->setPen(button);
        p->drawPoint(x + 2, y + 2);
        p->setPen(Qt::white);
        p->drawPoint(x + 3, y + 3);
        // bottom-left corner
        p->setPen(background);
        p->drawPoint(x, y + h - 1);
        p->drawPoint(x + 1, y + h - 1);
        p->drawPoint(x, y + h - 2);
        p->setPen(shadow);
        p->drawPoint(x + 1, y + h - 2);
        p->setPen(dark);
        p->drawPoint(x + 2, y + h - 3);
        // top-right corner
        p->setPen(background);
        p->drawPoint(x + w - 1, y);
        p->drawPoint(x + w - 2, y);
        p->drawPoint(x + w - 1, y + 1);
        p->setPen(shadow);
        p->drawPoint(x + w - 2, y + 1);
        p->setPen(dark);
        p->drawPoint(x + w - 3, y + 2);
        // bottom-right corner
        p->setPen(background);
        p->drawPoint(x + w - 1, y + h - 1);
        p->drawPoint(x + w - 2, y + h - 1);
        p->drawPoint(x + w - 1, y + h - 2);
        p->setPen(shadow);
        p->drawPoint(x + w - 2, y + h - 2);
        p->setPen(dark);
        p->drawPoint(x + w - 3, y + h - 3);
        p->setPen(mid);
        p->drawPoint(x + w - 4, y + h - 4);
    } else {
        p->fillRect(x + 2, y + 2, w - 4, h - 4, dark);
        // the dark side
        p->setPen(shadow);
        p->drawLine(x, y, x + w - 1, y);
        p->drawLine(x, y, x, y + h - 1);
        p->setPen(dark.darker());
        p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
        p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
        // the bright side
        p->setPen(button);
        p->drawLine(x + 1, y + h - 2, x + w - 2, y + h - 2);
        p->drawLine(x + w - 2, y + 1, x + w - 2, y + h - 2);
        p->setPen(dark);
        p->drawLine(x, y + h - 1, x + w - 1, y + h - 1);
        p->drawLine(x + w - 1, y, x + w - 1, y + h - 1);
        // top-left corner
        p->setPen(background);
        p->drawPoint(x, y);
        p->drawPoint(x + 1, y);
        p->drawPoint(x, y + 1);
        p->setPen(shadow);
        p->drawPoint(x + 1, y + 1);
        p->setPen(dark.darker());
        p->drawPoint(x + 3, y + 3);
        // bottom-left corner
        p->setPen(background);
        p->drawPoint(x, y + h - 1);
        p->drawPoint(x + 1, y + h - 1);
        p->drawPoint(x, y + h - 2);
        p->setPen(shadow);
        p->drawPoint(x + 1, y + h - 2);
        // top-right corner
        p->setPen(background);
        p->drawPoint(x + w - 1, y);
        p->drawPoint(x + w - 2, y);
        p->drawPoint(x + w - 1, y + 1);
        p->setPen(shadow);
        p->drawPoint(x + w - 2, y + 1);
        // bottom-right corner
        p->setPen(background);
        p->drawPoint(x + w - 1, y + h - 1);
        p->drawPoint(x + w - 2, y + h - 1);
        p->drawPoint(x + w - 1, y + h - 2);
        p->setPen(shadow);
        p->drawPoint(x + w - 2, y + h - 2);
        p->setPen(dark);
        p->drawPoint(x + w - 3, y + h - 3);
        p->setPen(mid);
        p->drawPoint(x + w - 4, y + h - 4);
    }
    p->restore();
}

void PlatinumStyle::drawScrollBarPage(QPainter *p, const QRect &r, const QPalette &palette,
                                      bool horizontal) const
{
    if (r.width() < 3 || r.height() < 3) {
        p->fillRect(r, palette.color(QPalette::Mid));
        p->setPen(palette.color(QPalette::Shadow));
        p->setBrush(Qt::NoBrush);
        p->drawRect(r);
        return;
    }
    const int x = r.x(), y = r.y(), w = r.width(), h = r.height();
    const QColor dark = palette.color(QPalette::Dark);
    const QColor shadow = palette.color(QPalette::Shadow);
    const QColor mid = palette.color(QPalette::Mid);
    const QColor button = palette.color(QPalette::Button);

    p->save();
    if (horizontal) {
        p->fillRect(x + 2, y + 2, w - 2, h - 4, mid);
        // the dark side
        p->setPen(dark.darker());
        p->drawLine(x, y, x + w - 1, y);
        p->setPen(shadow);
        p->drawLine(x, y, x, y + h - 1);
        p->setPen(mid.darker());
        p->drawLine(x + 1, y + 1, x + w - 1, y + 1);
        p->drawLine(x + 1, y + 1, x + 1, y + h - 2);
        // the bright side
        p->setPen(button);
        p->drawLine(x + 1, y + h - 2, x + w - 1, y + h - 2);
        p->setPen(shadow);
        p->drawLine(x, y + h - 1, x + w - 1, y + h - 1);
    } else {
        p->fillRect(x + 2, y + 2, w - 4, h - 2, mid);
        // the dark side
        p->setPen(dark.darker());
        p->drawLine(x, y, x + w - 1, y);
        p->setPen(shadow);
        p->drawLine(x, y, x, y + h - 1);
        p->setPen(mid.darker());
        p->drawLine(x + 1, y + 1, x + w - 2, y + 1);
        p->drawLine(x + 1, y + 1, x + 1, y + h - 1);
        // the bright side
        p->setPen(button);
        p->drawLine(x + w - 2, y + 1, x + w - 2, y + h - 1);
        p->setPen(shadow);
        p->drawLine(x + w - 1, y, x + w - 1, y + h - 1);
    }
    p->restore();
}

void PlatinumStyle::drawSunkenPanel(QPainter *p, const QRect &rect, const QPalette &palette) const
{
    const int x = rect.x(), y = rect.y(), w = rect.width(), h = rect.height();
    if (w <= 0 || h <= 0)
        return;
    p->save();
    // Qt 3 qDrawShadePanel(sunken, lineWidth = 2): a Dark top/left border
    // and a Light bottom/right border, laid out as corner-to-corner stairs.
    const QColor shade = palette.color(QPalette::Dark);
    const QColor light = palette.color(QPalette::Light);
    p->setPen(shade);
    p->drawLine(x, y, x + w - 2, y);
    p->drawLine(x, y + 1, x + w - 3, y + 1);
    p->drawLine(x, y + h - 2, x, y + 2);
    p->drawLine(x + 1, y + h - 2, x + 1, y + 1);
    p->setPen(light);
    p->drawLine(x, y + h - 1, x + w - 1, y + h - 1);
    p->drawLine(x + 1, y + h - 2, x + w - 1, y + h - 2);
    p->drawLine(x + w - 1, y, x + w - 1, y + h - 3);
    p->drawLine(x + w - 2, y + 1, x + w - 2, y + h - 3);
    p->restore();
}

void PlatinumStyle::drawPlatinumFocusRect(QPainter *p, const QRect &rect,
                                          const QPalette &palette) const
{
    // Appearance Manager focus ring: a solid 2 px accent outline (not the
    // Windows dashed black rect).
    if (rect.width() < 2 || rect.height() < 2)
        return;
    p->save();
    p->setPen(QPen(accentColor(palette), 2));
    p->setBrush(Qt::NoBrush);
    p->drawRect(rect.adjusted(1, 1, -1, -1));
    p->restore();
}

void PlatinumStyle::drawRiffles(QPainter *p, const QRect &rect, const QPalette &palette,
                                bool horizontal) const
{
    drawRiffles(p, rect, palette.color(QPalette::Light),
                palette.color(QPalette::Dark), horizontal);
}

void PlatinumStyle::drawRiffles(QPainter *p, const QRect &rect, const QColor &light,
                                const QColor &dark, bool horizontal) const
{
    // The riffle pattern: pairs of light/dark parallel lines spaced two
    // pixels apart, clamped to a 20-pixel band in the middle of the handle.
    int x = rect.x(), y = rect.y(), w = rect.width(), h = rect.height();

    if (horizontal) {
        // A handle that moves horizontally is decorated with vertical lines.
        if (w > 20) {
            x += (w - 20) / 2;
            w = 20;
        }
        if (w > 8) {
            const int n = w / 4;
            const int mx = x + w / 2 - n;
            p->setPen(light);
            for (int i = 0; i < n; ++i)
                p->drawLine(mx + 2 * i, y + 3, mx + 2 * i, y + h - 5);
            p->setPen(dark);
            for (int i = 0; i < n; ++i)
                p->drawLine(mx + 2 * i + 1, y + 4, mx + 2 * i + 1, y + h - 4);
        }
    } else {
        // A handle that moves vertically is decorated with horizontal lines.
        if (h > 20) {
            y += (h - 20) / 2;
            h = 20;
        }
        if (h > 8) {
            const int n = h / 4;
            const int my = y + h / 2 - n;
            p->setPen(light);
            for (int i = 0; i < n; ++i)
                p->drawLine(x + 3, my + 2 * i, x + w - 5, my + 2 * i);
            p->setPen(dark);
            for (int i = 0; i < n; ++i)
                p->drawLine(x + 4, my + 2 * i + 1, x + w - 4, my + 2 * i + 1);
        }
    }
}

QColor PlatinumStyle::accentColor(const QPalette &palette)
{
    // Appearance Manager "variation" color drives scroll thumbs, slider
    // tabs and focus rings. Very dark Highlight (classic navy #00007b)
    // is lifted into the periwinkle range so Light/Dark riffles stay
    // readable — matching the default Blue variation on Mac OS 8/9.
    const QColor h = palette.color(QPalette::Highlight);
    if (qGray(h.rgb()) < 80)
        return QColor(0x6c, 0x6c, 0xcc);
    return h;
}

void PlatinumStyle::drawRacingStripes(QPainter *p, const QRect &rect,
                                      const QPalette &palette, bool vertical) const
{
    // Mac OS 8/9 active title bar: alternating 1 px Light / Dark lines
    // ("racing stripes"). Fill Light once, then stamp every other Dark
    // line — same look as per-pixel pens, half the strokes.
    if (rect.width() < 1 || rect.height() < 1)
        return;

    const QColor light = palette.color(QPalette::Light);
    const QColor dark = palette.color(QPalette::Dark);
    p->fillRect(rect, light);
    p->setPen(dark);
    if (vertical) {
        for (int x = rect.left() + 1; x <= rect.right(); x += 2)
            p->drawLine(x, rect.top(), x, rect.bottom());
    } else {
        for (int y = rect.top() + 1; y <= rect.bottom(); y += 2)
            p->drawLine(rect.left(), y, rect.right(), y);
    }
}

void PlatinumStyle::drawSizeGrip(QPainter *p, const QStyleOption *option) const
{
    // Mac OS grow box: four diagonal Light/Dark ridge pairs in the corner.
    const QRect r = option->rect;
    const QPalette &pal = option->palette;
    const QColor light = pal.color(QPalette::Light);
    const QColor dark = pal.color(QPalette::Dark);
    const bool rtl = option->direction == Qt::RightToLeft;
    const int sw = qMin(r.width(), r.height()) - 1;
    if (sw < 3)
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    if (rtl) {
        int x2 = r.x() + sw;
        int y = r.bottom() - sw;
        for (int n = 0; n < 4; ++n) {
            p->setPen(dark);
            p->drawLine(r.x(), y, x2, r.bottom());
            p->setPen(light);
            p->drawLine(r.x(), y + 1, x2 - 1, r.bottom());
            y += 3;
            x2 -= 3;
        }
    } else {
        int x = r.right() - sw;
        int y = r.bottom() - sw;
        for (int n = 0; n < 4; ++n) {
            p->setPen(dark);
            p->drawLine(x, r.bottom(), r.right(), y);
            p->setPen(light);
            p->drawLine(x + 1, r.bottom(), r.right(), y + 1);
            x += 3;
            y += 3;
        }
    }
    p->restore();
}

void PlatinumStyle::drawDockWidgetTitle(QPainter *p, const QStyleOption *option) const
{
    // Approximates a Mac OS 8/9 Platinum window title bar: active titles
    // fill with racing stripes; the title text sits in a solid Button gap
    // that interrupts the stripes. Inactive titles are a flat Button face.
    const QStyleOptionDockWidget *dw =
            qstyleoption_cast<const QStyleOptionDockWidget *>(option);
    if (!dw)
        return;

    const QRect r = dw->rect;
    const QPalette &pal = dw->palette;
    const bool active = dw->state & State_Active;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const bool vertical = dw->verticalTitleBar;
#else
    const bool vertical = false;
#endif

    // Stripes already paint every pixel; only fill when inactive.
    if (active)
        drawRacingStripes(p, r, pal, vertical);
    else
        p->fillRect(r, pal.brush(QPalette::Button));

    if (!dw->title.isEmpty()) {
        // Reserve space for the close/float buttons: along the trailing
        // edge for horizontal titles, along the bottom for vertical ones.
        QRect textArea = r.adjusted(4, 0, -4, 0);
        if (dw->closable)
            textArea.adjust(0, 0, vertical ? 0 : -16, vertical ? -16 : 0);
        if (dw->floatable)
            textArea.adjust(0, 0, vertical ? 0 : -16, vertical ? -16 : 0);

        const int textLength = vertical ? textArea.height() : textArea.width();
        const QString text = p->fontMetrics().elidedText(dw->title, Qt::ElideRight,
                                                         qMax(textLength, 1));
        const QSize textSize = p->fontMetrics().size(Qt::TextShowMnemonic, text);

        if (vertical) {
            // Punch a solid gap centered in the text area, then rotate.
            const int gapH = textSize.width() + 8;
            const QRect gap(r.x(), textArea.center().y() - gapH / 2, r.width(), gapH);
            p->fillRect(gap.intersected(textArea), pal.brush(QPalette::Button));
            p->save();
            p->translate(textArea.center());
            p->rotate(-90);
            const QRect vr(-textArea.height() / 2, -textArea.width() / 2,
                           textArea.height(), textArea.width());
            drawItemText(p, vr, Qt::AlignCenter, pal, dw->state & State_Enabled,
                         text, QPalette::WindowText);
            p->restore();
        } else {
            // Gap must share the text's center (textArea), not the full
            // title bar — otherwise close/float buttons shift the label
            // left while the gap stays window-centered.
            const int gapW = textSize.width() + 12;
            const QRect gap(textArea.center().x() - gapW / 2, r.y(), gapW, r.height());
            p->fillRect(gap.intersected(textArea), pal.brush(QPalette::Button));
            drawItemText(p, textArea, Qt::AlignCenter, pal,
                         dw->state & State_Enabled, text, QPalette::WindowText);
        }
    }

    // Bottom (or trailing) edge so the title bar reads as a raised strip.
    p->setPen(pal.color(QPalette::Dark));
    if (vertical)
        p->drawLine(r.topRight(), r.bottomRight());
    else
        p->drawLine(r.bottomLeft(), r.bottomRight());
}

void PlatinumStyle::drawBranch(QPainter *p, const QStyleOption *option) const
{
    // Finder-style disclosure triangle (no Windows +/- box) plus Mid guide
    // lines to the row / siblings.
    const QRect r = option->rect;
    const QPalette &pal = option->palette;
    const int cx = r.center().x();
    const int cy = r.center().y();

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);

    int spacer = 0;
    if (option->state & State_Children) {
        p->setPen(Qt::NoPen);
        p->setBrush(pal.color(QPalette::Text));
        if (option->state & State_Open) {
            // Pointing down.
            p->drawPolygon(QPolygon() << QPoint(cx - 4, cy - 2)
                                      << QPoint(cx + 4, cy - 2)
                                      << QPoint(cx, cy + 3));
        } else if (option->direction == Qt::RightToLeft) {
            p->drawPolygon(QPolygon() << QPoint(cx + 2, cy - 4)
                                      << QPoint(cx + 2, cy + 4)
                                      << QPoint(cx - 3, cy));
        } else {
            // Pointing right.
            p->drawPolygon(QPolygon() << QPoint(cx - 2, cy - 4)
                                      << QPoint(cx - 2, cy + 4)
                                      << QPoint(cx + 3, cy));
        }
        spacer = 5;
    }

    p->setPen(pal.color(QPalette::Mid));
    if (option->state & State_Item) {
        if (option->direction == Qt::RightToLeft)
            p->drawLine(r.left(), cy, cx - spacer, cy);
        else
            p->drawLine(cx + spacer, cy, r.right(), cy);
    }
    if (option->state & State_Sibling)
        p->drawLine(cx, cy + spacer, cx, r.bottom());
    if (option->state & (State_Item | State_Sibling))
        p->drawLine(cx, cy - spacer, cx, r.top());

    p->restore();
}

void PlatinumStyle::drawGroupBox(QPainter *p, const QStyleOption *option,
                                 const QWidget *widget) const
{
    const QStyleOptionGroupBox *gb = qstyleoption_cast<const QStyleOptionGroupBox *>(option);
    if (!gb)
        return;

    QRect textRect = subControlRect(CC_GroupBox, gb, SC_GroupBoxLabel, widget);
    QRect checkRect = subControlRect(CC_GroupBox, gb, SC_GroupBoxCheckBox, widget);
    const bool flat = gb->features & QStyleOptionFrame::Flat;

    if ((gb->subControls & SC_GroupBoxFrame) && !flat) {
        QStyleOptionFrame frame;
        frame.QStyleOption::operator=(*gb);
        frame.rect = subControlRect(CC_GroupBox, gb, SC_GroupBoxFrame, widget);
        p->save();
        QRegion region(gb->rect);
        const bool ltr = gb->direction == Qt::LeftToRight;
        region -= checkRect.united(textRect).adjusted(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
        if (!gb->text.isEmpty() || (gb->subControls & SC_GroupBoxCheckBox))
            p->setClipRegion(region);
        drawPrimitive(PE_FrameGroupBox, &frame, p, widget);
        p->restore();
    }

    if ((gb->subControls & SC_GroupBoxLabel) && !gb->text.isEmpty()) {
        p->save();
        const QColor textColor = gb->textColor.isValid()
                ? gb->textColor : gb->palette.color(QPalette::WindowText);
        p->setPen(textColor);
        int alignment = int(gb->textAlignment);
        if (!styleHint(SH_UnderlineShortcut, option, widget))
            alignment |= Qt::TextHideMnemonic;
        p->drawText(textRect, Qt::TextShowMnemonic | Qt::AlignLeft | alignment, gb->text);
        p->restore();
    }

    if (gb->subControls & SC_GroupBoxCheckBox) {
        QStyleOptionButton box;
        box.QStyleOption::operator=(*gb);
        box.rect = checkRect;
        drawPrimitive(PE_IndicatorCheckBox, &box, p, widget);
    }
}

void PlatinumStyle::drawArrow(QPainter *p, PrimitiveElement arrow, const QRect &rect,
                              const QColor &line, const QColor &point) const
{
    if (rect.width() < 3 || rect.height() < 3)
        return;

    // Qt 3 QWindowsStyle PE_Arrow*: a 7-point array centered in the rect.
    // The first three line segments build the arrow shaft, the seventh point
    // is the arrow tip.
    static const QPoint up[] = { QPoint(-4, 1), QPoint(2, 1), QPoint(-3, 0),
                                 QPoint(1, 0), QPoint(-2, -1), QPoint(0, -1),
                                 QPoint(-1, -2) };
    static const QPoint down[] = { QPoint(-4, -2), QPoint(2, -2), QPoint(-3, -1),
                                   QPoint(1, -1), QPoint(-2, 0), QPoint(0, 0),
                                   QPoint(-1, 1) };
    static const QPoint right[] = { QPoint(-2, -3), QPoint(-2, 3), QPoint(-1, -2),
                                    QPoint(-1, 2), QPoint(0, -1), QPoint(0, 1),
                                    QPoint(1, 0) };
    static const QPoint left[] = { QPoint(0, -3), QPoint(0, 3), QPoint(-1, -2),
                                   QPoint(-1, 2), QPoint(-2, -1), QPoint(-2, 1),
                                   QPoint(-3, 0) };

    const QPoint *pts = nullptr;
    switch (arrow) {
    case PE_IndicatorArrowUp:
        pts = up;
        break;
    case PE_IndicatorArrowDown:
        pts = down;
        break;
    case PE_IndicatorArrowRight:
        pts = right;
        break;
    case PE_IndicatorArrowLeft:
        pts = left;
        break;
    default:
        return;
    }

    // Qt 3 centers the arrow on rect.x() + rect.width() / 2 (a closed
    // interval), while QRect::center() in Qt 6 uses (width() - 1) / 2,
    // which shifts even-sized arrows one pixel toward the top-left.
    const QPoint c(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    p->save();
    p->setPen(line);
    p->drawLine(c + pts[0], c + pts[1]);
    p->drawLine(c + pts[2], c + pts[3]);
    p->drawLine(c + pts[4], c + pts[5]);
    p->setPen(point);
    p->drawPoint(c + pts[6]);
    p->restore();
}

void PlatinumStyle::drawArrowGlyph(QPainter *p, PrimitiveElement arrow, const QRect &rect,
                                   const QPalette &palette, bool enabled,
                                   const QColor &enabledColor) const
{
    // Qt 3 QWindowsStyle PE_Arrow*: disabled arrows get a Light etch one
    // pixel down-right, then the Mid glyph on top — without that offset
    // they look like a flat Mid smear.
    if (enabled) {
        drawArrow(p, arrow, rect, enabledColor, enabledColor);
        return;
    }
    const QColor light = palette.color(QPalette::Light);
    const QColor mid = palette.color(QPalette::Mid);
    drawArrow(p, arrow, rect.translated(1, 1), light, light);
    drawArrow(p, arrow, rect, mid, mid);
}

void PlatinumStyle::drawCheckMark(QPainter *p, const QRect &rect, const QColor &color,
                                  const QColor &shadow) const
{
    // Qt 3 check_mark point array (relative coordinates, 13x13 box).
    static const QLine checkLines[] = {
        QLine(3, 5, 5, 5), QLine(4, 6, 5, 6), QLine(5, 7, 6, 7), QLine(5, 8, 6, 8),
        QLine(6, 9, 9, 9), QLine(6, 10, 8, 10), QLine(7, 11, 8, 11), QLine(7, 12, 7, 12),
        QLine(8, 8, 9, 8), QLine(8, 7, 10, 7), QLine(9, 6, 10, 6), QLine(9, 5, 11, 5),
        QLine(10, 4, 11, 4), QLine(10, 3, 12, 3), QLine(11, 2, 12, 2), QLine(11, 1, 13, 1),
        QLine(12, 0, 13, 0)
    };
    const int x = rect.x(), y = rect.y();
    p->save();
    p->setPen(shadow);
    for (const QLine &l : checkLines)
        p->drawLine(l.translated(x + 1, y + 1));
    p->setPen(color);
    for (const QLine &l : checkLines)
        p->drawLine(l.translated(x, y));
    p->restore();
}

void PlatinumStyle::drawCheckBox(QPainter *p, const QStyleOption *option) const
{
    const QRect r = option->rect;
    const QPalette &pal = option->palette;
    const bool on = option->state & State_On;
    const bool tri = option->state & State_NoChange;
    const bool down = option->state & State_Sunken;

    // Qt 3 PE_Indicator: the left 13 pixels are a small bevel, the right
    // 2 pixels are filled with the window color, and the bevel is framed
    // with a shadow rectangle. A pressed box is drawn sunken.
    const int bw = r.width() - 2;
    if (bw < 1)
        return;
    const QRect bevelRect(r.x(), r.y(), bw, r.height());
    drawBevel(p, bevelRect, pal, down, QBrush(pal.color(QPalette::Mid)));
    p->fillRect(bevelRect.right() + 1, r.y(), r.width() - bw, r.height(),
                pal.color(QPalette::Window));
    p->setPen(pal.color(QPalette::Shadow));
    p->setBrush(Qt::NoBrush);
    p->drawRect(bevelRect);

    // Qt 3 shifts the mark one pixel down/right while the box is pressed.
    // The mark sits 1 px left of the box origin (the check is drawn from
    // x+3 inside the mark rect, so it stays inside the bevel).
    const int mx = r.x() - 1 + (down ? 1 : 0);
    const int my = r.y() + (down ? 1 : 0);
    const QColor mark = pal.color(QPalette::Text);
    if (on) {
        drawCheckMark(p, QRect(mx, my, bw, r.height()), mark, pal.color(QPalette::Dark));
    } else if (tri) {
        // Qt 3 nochange_mark: a two-pixel high dash, shadowed like the check.
        p->setPen(pal.color(QPalette::Dark));
        p->drawLine(mx + 4, my + 6, mx + 10, my + 6);
        p->drawLine(mx + 4, my + 7, mx + 10, my + 7);
        p->setPen(mark);
        p->drawLine(mx + 3, my + 5, mx + 9, my + 5);
        p->drawLine(mx + 3, my + 6, mx + 9, my + 6);
    }
}

void PlatinumStyle::drawRadioButton(QPainter *p, const QStyleOption *option) const
{
    const QRect r = option->rect;
    const QPalette &pal = option->palette;
    const bool down = option->state & State_Sunken;
    const bool on = option->state & State_On;

    // Qt 3 PE_ExclusiveIndicator: the whole 15x15 box is erased to the
    // window color, then a 13x13 circle is filled, outlined by a shadow
    // polyline and finished with a top-left/bottom-right arc. All geometry
    // is integer and drawn without antialiasing, exactly like Qt 3.
    static const QPoint circleOutline[] = { // pts1: normal circle
        {5, 0}, {8, 0}, {9, 1}, {10, 1}, {11, 2}, {12, 3}, {12, 4}, {13, 5},
        {13, 8}, {12, 9}, {12, 10}, {11, 11}, {10, 12}, {9, 12}, {8, 13},
        {5, 13}, {4, 12}, {3, 12}, {2, 11}, {1, 10}, {1, 9}, {0, 8}, {0, 5},
        {1, 4}, {1, 3}, {2, 2}, {3, 1}, {4, 1}
    };
    static const QPoint topArc[] = { // pts2: top-left arc
        {5, 1}, {8, 1}, {3, 2}, {7, 2}, {2, 3}, {5, 3}, {2, 4}, {4, 4},
        {1, 5}, {3, 5}, {1, 6}, {1, 8}, {2, 6}, {2, 7}
    };
    static const QPoint bottomDark[] = { // pts3: bottom-right, released
        {5, 12}, {8, 12}, {7, 11}, {10, 11}, {8, 10}, {11, 10}, {9, 9},
        {11, 9}, {10, 8}, {12, 8}, {11, 7}, {11, 7}, {12, 5}, {12, 7}
    };
    static const QPoint bottomLight[] = { // pts4: bottom-right, sunken/on
        {5, 12}, {8, 12}, {7, 11}, {10, 11}, {9, 10}, {11, 10}, {10, 9},
        {11, 9}, {11, 7}, {11, 8}, {12, 5}, {12, 8}
    };
    static const QPoint dotOctagon[] = { // pts5: the check mark
        {6, 4}, {8, 4}, {10, 6}, {10, 8}, {8, 10}, {6, 10}, {4, 8}, {4, 6}
    };
    static const QPoint dotExtras[] = { // pts6: check mark extensions
        {4, 5}, {5, 4}, {9, 4}, {10, 5}, {10, 9}, {9, 10}, {5, 10}, {4, 9}
    };

    const int x = r.x(), y = r.y();
    p->save();
    p->fillRect(r, pal.color(QPalette::Window));

    p->setBrush((down || on) ? pal.color(QPalette::Dark) : pal.color(QPalette::Button));
    p->setPen(Qt::NoPen);
    p->drawEllipse(x, y, 13, 13);

    p->setPen(pal.color(QPalette::Shadow));
    for (int i = 0; i < 27; ++i)
        p->drawLine(QPoint(x, y) + circleOutline[i], QPoint(x, y) + circleOutline[i + 1]);

    const QColor topColor = (down || on) ? pal.color(QPalette::Dark).darker()
                                         : pal.color(QPalette::Light);
    const QColor bottomColor = (down || on) ? pal.color(QPalette::Light)
                                            : pal.color(QPalette::Dark);
    const QPoint *bottomArc = (down || on) ? bottomLight : bottomDark;
    const int bottomLen = (down || on) ? 12 : 14;

    p->setPen(topColor);
    for (int i = 0; i < 14; i += 2)
        p->drawLine(QPoint(x, y) + topArc[i], QPoint(x, y) + topArc[i + 1]);
    p->setPen(bottomColor);
    for (int i = 0; i < bottomLen; i += 2)
        p->drawLine(QPoint(x, y) + bottomArc[i], QPoint(x, y) + bottomArc[i + 1]);

    if (on) {
        p->setBrush(pal.color(QPalette::Text));
        p->setPen(pal.color(QPalette::Text));
        QPolygon oct;
        oct.reserve(8);
        for (const QPoint &pt : dotOctagon)
            oct << pt;
        oct.translate(x, y);
        p->drawPolygon(oct);
        p->setBrush(Qt::NoBrush);
        p->setPen(pal.color(QPalette::Dark));
        for (int i = 0; i < 8; i += 2)
            p->drawLine(QPoint(x, y) + dotExtras[i], QPoint(x, y) + dotExtras[i + 1]);
    }
    p->restore();
}

QColor PlatinumStyle::mixedColor(const QColor &c1, const QColor &c2)
{
    int h1, s1, v1, h2, s2, v2;
    c1.getHsv(&h1, &s1, &v1);
    c2.getHsv(&h2, &s2, &v2);
    return QColor::fromHsv((h1 + h2) / 2, (s1 + s2) / 2, (v1 + v2) / 2);
}
