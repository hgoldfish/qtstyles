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

#include "keramikstyle.h"
#include "qtstyles_palette.h"
#include "qstylehelper_p.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qprogressbar.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qtabbar.h>
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qtoolbutton.h>

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpalette.h>

#include <QtCore/qmath.h>
#include <QtCore/qdebug.h>

// Qt 6 renamed QStyleOptionMenuItem::tabWidth to reservedShortcutWidth.
static inline int menuItemTabWidth(const QStyleOptionMenuItem *mi)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return mi->reservedShortcutWidth;
#else
    return mi->tabWidth;
#endif
}

// A standalone toolbar (not inside a QMainWindow) reports NoToolBarArea but
// still lays out horizontally; treat it like the top/bottom areas.
static inline bool toolbarHorizontal(const QStyleOptionToolBar *tb)
{
    return !tb
            || tb->toolBarArea == Qt::NoToolBarArea
            || tb->toolBarArea == Qt::TopToolBarArea
            || tb->toolBarArea == Qt::BottomToolBarArea;
}

// The pop-up menu surface: the original's CE_PopupMenuItem fills every row
// (and the empty areas) with background.light(105), so the whole pop-up is
// this one flat tone.
static inline QColor menuSurfaceColor(const QPalette &pal)
{
    return pal.color(QPalette::Window).lighter(105);
}

// Menu item metrics from the original KDE3 Keramik style.
enum {
    itemFrame = 2,
    itemHMargin = 6,
    arrowHMargin = 6,
    rightBorder = 12,
};

// ---------------------------------------------------------------------------
// Palette ramp
// ---------------------------------------------------------------------------

// The original theme brightens colors with a value-weighted curve rather than
// a plain factor: the darker the base colour, the less it is lifted.  This is
// a verbatim re-implementation of the original ColorUtil::lighten: the lifted
// amount is share^2 * diff (share = min(1, value/230)), then a fixed extra
// luminance of (diff - lift) * 7.55 is added to every channel.
static inline QColor colorLighten(const QColor &in, int factor)
{
    if (factor <= 100)
        return in;
    const int v = in.value();
    const qreal share = qMin(1.0, v / 230.0);
    const int diff = factor - 100;
    const int lift = qRound(share * share * diff);
    QColor out = in.lighter(100 + lift);
    const int extra = qRound((diff - lift) * 7.55);
    return QColor(qMin(255, out.red() + extra),
                  qMin(255, out.green() + extra),
                  qMin(255, out.blue() + extra));
}

KeramikStyle::KeramikStyle() = default;
KeramikStyle::~KeramikStyle() = default;

/*!
    \reimp

    Returns the classic KDE 3 default palette the ceramic Keramik look was
    designed for: light grey surfaces with a blue highlight accent.  This is
    a suggestion only -- Qt does not adopt it automatically; callers may
    apply it with QApplication::setPalette().
*/
QPalette KeramikStyle::standardPalette() const
{
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(0xdf, 0xdf, 0xdf));
    pal.setColor(QPalette::WindowText, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::AlternateBase, QColor(0xee, 0xee, 0xee));
    pal.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xdc));
    pal.setColor(QPalette::ToolTipText, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::Text, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::Button, QColor(0xd8, 0xd8, 0xd8));
    pal.setColor(QPalette::ButtonText, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::Light, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::Midlight, QColor(0xec, 0xec, 0xec));
    pal.setColor(QPalette::Mid, QColor(0xbd, 0xbd, 0xbd));
    pal.setColor(QPalette::Dark, QColor(0x8a, 0x8a, 0x8a));
    pal.setColor(QPalette::Shadow, QColor(0x5a, 0x5a, 0x5a));
    pal.setColor(QPalette::Highlight, QColor(0x3d, 0x7e, 0xbb));
    pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::PlaceholderText, QColor(0x8a, 0x8a, 0x8a));
    pal.setColor(QPalette::Link, QColor(0x00, 0x00, 0xee));
    pal.setColor(QPalette::LinkVisited, QColor(0x52, 0x18, 0x8b));

    QtStyles::applyClassicDisabled(&pal);
    return pal;
}

const KeramikStyle::KeramikColors *KeramikStyle::colors(const QPalette &palette) const
{
    const QRgb buttonRgb = palette.color(QPalette::Button).rgb();
    const QRgb highlightRgb = palette.color(QPalette::Highlight).rgb();
    // Compact 64-bit key: 32 bits of button colour, 32 bits of highlight.
    const quint64 key = (quint64(buttonRgb) << 32) | quint64(highlightRgb);

    const auto it = m_colorCache.constFind(key);
    if (it != m_colorCache.constEnd())
        return &it.value();

    KeramikColors c;
    c.buttonRgb = buttonRgb;
    c.highlightRgb = highlightRgb;

    const QColor button = palette.color(QPalette::Button);
    const QColor highlight = palette.color(QPalette::Highlight);

    // Vertical ceramic bevel: brighter top, darker bottom.
    c.gradientTop = button.lighter(118);
    c.gradientBottom = button.darker(112);
    // Thin outer frame and the two bevel rims.
    c.border = button.darker(160);
    c.innerTop = button.lighter(128);
    c.innerBottom = button.darker(140);
    // White/beige fill for sunken wells.
    c.wellBase = palette.color(QPalette::Base);
    // Accent ramp (palette highlight) for handles and progress bars.
    c.highlightTop = highlight.lighter(112);
    c.highlightBottom = highlight.darker(112);
    c.highlightBorder = highlight.darker(145);
    // Menu/tool-bar background ramp.  The original's "menu" gradient starts
    // slightly dark and grows lighter towards the bottom, unlike the button
    // bevel which is bright on top and dark below.
    c.menuGradTop = button.lighter(93);
    c.menuGradBottom = colorLighten(button, 109);

    return &m_colorCache.insert(key, c).value();
}

QPainterPath KeramikStyle::roundedRect(const QRect &r, int radius)
{
    const qreal rr = qMin(qreal(radius), qMin(qreal(r.width()), qreal(r.height())) / 2.0);
    QPainterPath path;
    path.addRoundedRect(QRectF(r), rr, rr);
    return path;
}

// ---------------------------------------------------------------------------
// Shared drawing helpers
// ---------------------------------------------------------------------------

// Keramik arrow line segments, origin at the arrow centre. These reproduce
// the original theme's thin line-drawn arrows (horizontal "slices" of a
// triangle); when etched, the light pass is drawn one pixel down/right first.
struct ArrowSeg { qint8 x1, y1, x2, y2; };

static const ArrowSeg keramikUpArrow[] = {
    { -1, -3,  0, -3 }, { -2, -2,  1, -2 }, { -3, -1,  2, -1 },
    { -4,  0,  3,  0 }, { -4,  1,  3,  1 }
};
static const ArrowSeg keramikDownArrow[] = {
    { -4, -2,  3, -2 }, { -4, -1,  3, -1 }, { -3,  0,  2,  0 },
    { -2,  1,  1,  1 }, { -1,  2,  0,  2 }
};
static const ArrowSeg keramikLeftArrow[] = {
    { -3, -1, -3,  0 }, { -2, -2, -2,  1 }, { -1, -3, -1,  2 },
    {  0, -4,  0,  3 }, {  1, -4,  1,  3 }
};
static const ArrowSeg keramikRightArrow[] = {
    { -2, -4, -2,  3 }, { -1, -4, -1,  3 }, {  0, -3,  0,  2 },
    {  1, -2,  1,  1 }, {  2, -1,  2,  0 }
};
static const ArrowSeg keramikComboArrow[] = {
    { -4, -5,  4, -5 }, { -2, -2,  2, -2 }, { -2, -1,  2, -1 },
    { -2,  0,  2,  0 }, { -4,  1,  4,  1 }, { -3,  2,  3,  2 },
    { -2,  3,  2,  3 }, { -1,  4,  1,  4 }, {  0,  5,  0,  5 }
};

void KeramikStyle::drawArrow(QPainter *p, PrimitiveElement pe, const QRect &r,
                             const QColor &color, const QColor *etch) const
{
    const ArrowSeg *seg = nullptr;
    switch (pe) {
    case PE_IndicatorArrowUp:    seg = keramikUpArrow; break;
    case PE_IndicatorArrowDown:  seg = keramikDownArrow; break;
    case PE_IndicatorArrowLeft:  seg = keramikLeftArrow; break;
    case PE_IndicatorArrowRight: seg = keramikRightArrow; break;
    default: return;
    }

    const int cx = r.center().x();
    const int cy = r.center().y();

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    if (etch) {
        p->setPen(QPen(*etch, 1));
        for (int i = 0; i < 5; ++i)
            p->drawLine(cx + seg[i].x1 + 1, cy + seg[i].y1 + 1,
                        cx + seg[i].x2 + 1, cy + seg[i].y2 + 1);
    }
    p->setPen(QPen(color, 1));
    for (int i = 0; i < 5; ++i)
        p->drawLine(cx + seg[i].x1, cy + seg[i].y1,
                    cx + seg[i].x2, cy + seg[i].y2);
    p->restore();
}

void KeramikStyle::drawComboArrow(QPainter *p, const QRect &r,
                                  const QColor &color) const
{
    const int cx = r.center().x();
    const int cy = r.center().y();

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(color, 1));
    for (int i = 0; i < 9; ++i)
        p->drawLine(cx + keramikComboArrow[i].x1, cy + keramikComboArrow[i].y1,
                    cx + keramikComboArrow[i].x2, cy + keramikComboArrow[i].y2);
    p->restore();
}

void KeramikStyle::drawCheckMark(QPainter *p, const QRect &r, const QColor &color) const
{
    const qreal cx = r.center().x();
    const qreal cy = r.center().y();

    QPainterPath path;
    path.moveTo(cx - 3.0, cy);
    path.lineTo(cx - 1.0, cy + 2.5);
    path.lineTo(cx + 3.5, cy - 2.5);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->setBrush(Qt::NoBrush);
    p->drawPath(path);
    p->restore();
}

void KeramikStyle::drawRipple(QPainter *p, const QRect &r, const QColor &color) const
{
    if (r.width() < 6 || r.height() < 8)
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(color, 1));
    const qreal top = r.top() + 1;
    const qreal bottom = r.bottom() - 1;
    const qreal span = bottom - top;
    const qreal cx = r.center().x();

    // Three slightly wavy vertical strokes, the signature Keramik "ripple".
    for (int i = -1; i <= 1; ++i) {
        const qreal x = cx + i * 2.0;
        QPainterPath path;
        path.moveTo(x, top);
        path.cubicTo(x + 1.2, top + span * 0.30,
                     x - 1.2, top + span * 0.55,
                     x, top + span * 0.82);
        path.lineTo(x, bottom);
        p->drawPath(path);
    }
    p->restore();
}

// Button bevel: vertical gradient fill, thin rounded frame, bright top rim.
void KeramikStyle::drawButtonPanel(QPainter *p, const QStyleOption *opt) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;
    const bool sunken = opt->state & (State_Sunken | State_On);
    const bool hover = opt->state & State_MouseOver;
    const bool disabled = !(opt->state & State_Enabled);
    const QStyleOptionButton *btnOpt = qstyleoption_cast<const QStyleOptionButton *>(opt);
    const bool isDefault = btnOpt && (btnOpt->features & QStyleOptionButton::DefaultButton);

    QColor top = c->gradientTop;
    QColor bottom = c->gradientBottom;
    if (sunken) {
        top = c->gradientBottom;
        bottom = c->gradientBottom.darker(105);
    } else if (hover) {
        top = c->gradientTop.lighter(105);
        bottom = c->gradientBottom.lighter(105);
    }
    if (disabled) {
        top = top.lighter(112);
        bottom = bottom.lighter(106);
    }

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath shape = roundedRect(r.adjusted(0, 0, -1, -1), 3);

    QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
    grad.setColorAt(0.0, top);
    grad.setColorAt(1.0, bottom);
    p->setPen(Qt::NoPen);
    p->setBrush(grad);
    p->drawPath(shape);

    p->setBrush(Qt::NoBrush);
    p->setPen(disabled ? c->border.lighter(125)
                       : isDefault ? c->border.darker(112) : c->border);
    p->drawPath(shape);

    if (!disabled) {
        // Ceramic sheen along the top edge.
        p->setPen(c->innerTop);
        p->drawLine(r.left() + 3, r.top() + 1, r.right() - 3, r.top() + 1);
        p->setPen(sunken ? c->innerBottom : c->innerTop);
        p->drawLine(r.left() + 1, r.top() + 3, r.left() + 1, r.bottom() - 3);
        if (!sunken) {
            p->setPen(c->innerBottom);
            p->drawLine(r.left() + 3, r.bottom() - 1, r.right() - 3, r.bottom() - 1);
        }
    }
    p->restore();
}

// Sunken input well: white/beige fill, recessed inner bevel.
void KeramikStyle::drawWell(QPainter *p, const QStyleOption *opt) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;
    const bool disabled = !(opt->state & State_Enabled);

    QColor base = c->wellBase;
    if (disabled)
        base = base.darker(105);

    // The original's PE_PanelLineEdit is a square 1px frame: dark on the
    // top/left, a dimmed light on the bottom/right, over the base fill.
    p->save();
    p->setPen(Qt::NoPen);
    p->setBrush(base);
    p->drawRect(r);

    p->setPen(opt->palette.color(QPalette::Dark));
    p->drawLine(r.left(), r.top(), r.right(), r.top());
    p->drawLine(r.left(), r.top(), r.left(), r.bottom());
    p->setPen(opt->palette.color(QPalette::Light).darker(110));
    p->drawLine(r.right(), r.top(), r.right(), r.bottom());
    p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
    p->restore();
}

// Accent panel: palette-highlight gradient with a thin frame. Used for the
// slider thumb, scroll-bar handle, progress chunks, selections and menus.
void KeramikStyle::drawHighlightPanel(QPainter *p, const QStyleOption *opt,
                                      const QRect &r, int radius) const
{
    const KeramikColors *c = colors(opt->palette);
    const bool active = opt->state & (State_Sunken | State_MouseOver | State_On | State_Selected);
    const bool disabled = !(opt->state & State_Enabled);

    QColor top = c->highlightTop;
    QColor bottom = c->highlightBottom;
    if (active) {
        top = top.lighter(105);
        bottom = bottom.lighter(105);
    }
    if (disabled) {
        top = top.darker(112);
        bottom = bottom.darker(108);
    }

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath shape = roundedRect(r.adjusted(0, 0, -1, -1), radius);

    QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
    grad.setColorAt(0.0, top);
    grad.setColorAt(1.0, bottom);
    p->setPen(Qt::NoPen);
    p->setBrush(grad);
    p->drawPath(shape);

    p->setBrush(Qt::NoBrush);
    p->setPen(disabled ? c->highlightBorder.lighter(130) : c->highlightBorder);
    p->drawPath(shape);

    if (!disabled) {
        p->setPen(top.lighter(108));
        p->drawLine(r.left() + 2, r.top() + 1, r.right() - 2, r.top() + 1);
    }
    p->restore();
}

// Recessed track/groove strip (slider groove, progress groove, scroll track).
void KeramikStyle::drawGroove(QPainter *p, const QStyleOption *opt, const QRect &r) const
{
    const KeramikColors *c = colors(opt->palette);
    if (r.width() <= 0 || r.height() <= 0)
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath shape = roundedRect(r.adjusted(0, 0, -1, -1), 1);

    p->setPen(Qt::NoPen);
    p->setBrush(c->wellBase.darker(106));
    p->drawPath(shape);

    p->setBrush(Qt::NoBrush);
    p->setPen(c->border.lighter(118));
    p->drawPath(shape);

    // Inner top/left shadow.
    p->setPen(c->innerBottom);
    if (opt->state & State_Horizontal)
        p->drawLine(r.left() + 2, r.top() + 1, r.right() - 2, r.top() + 1);
    else
        p->drawLine(r.left() + 1, r.top() + 2, r.left() + 1, r.bottom() - 2);
    p->restore();
}

void KeramikStyle::drawCheckBoxIndicator(QPainter *p, const QStyleOption *opt,
                                         bool on, bool tri) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;
    const bool hover = opt->state & State_MouseOver;
    const bool disabled = !(opt->state & State_Enabled);

    QColor base = c->wellBase;
    if (hover && !disabled)
        base = base.lighter(104);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath shape = roundedRect(r.adjusted(0, 0, -1, -1), 3);

    p->setPen(Qt::NoPen);
    p->setBrush(base);
    p->drawPath(shape);

    p->setBrush(Qt::NoBrush);
    p->setPen(disabled ? c->border.lighter(120) : c->border);
    p->drawPath(shape);

    // Sunken bevel inside the box.
    p->setPen(c->innerBottom);
    p->drawLine(r.left() + 2, r.top() + 1, r.right() - 3, r.top() + 1);
    p->drawLine(r.left() + 1, r.top() + 2, r.left() + 1, r.bottom() - 3);
    p->setPen(opt->palette.color(QPalette::Light));
    p->drawLine(r.left() + 2, r.bottom() - 2, r.right() - 3, r.bottom() - 2);
    p->drawLine(r.right() - 2, r.top() + 2, r.right() - 2, r.bottom() - 3);

    if (on || tri) {
        QColor mark = disabled ? c->border
                               : opt->palette.color(QPalette::ButtonText);
        if (tri) {
            p->setPen(QPen(mark, 1.6));
            p->drawLine(r.left() + 3, r.center().y(), r.right() - 3, r.center().y());
        } else {
            drawCheckMark(p, r.adjusted(0, 0, -1, -1), mark);
        }
    }
    p->restore();
}

void KeramikStyle::drawRadioIndicator(QPainter *p, const QStyleOption *opt, bool on) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;
    const bool hover = opt->state & State_MouseOver;
    const bool disabled = !(opt->state & State_Enabled);

    QColor base = c->wellBase;
    if (hover && !disabled)
        base = base.lighter(104);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    QPainterPath circle;
    circle.addEllipse(QRectF(r.adjusted(0, 0, -1, -1)));

    p->setPen(Qt::NoPen);
    p->setBrush(base);
    p->drawPath(circle);

    p->setBrush(Qt::NoBrush);
    p->setPen(disabled ? c->border.lighter(120) : c->border);
    p->drawPath(circle);

    // Inner bevel arc: dark upper-left, light lower-right.
    p->setPen(c->innerBottom);
    p->drawArc(r.adjusted(1, 1, -2, -2), 200 * 16, 150 * 16);
    p->setPen(opt->palette.color(QPalette::Light));
    p->drawArc(r.adjusted(1, 1, -2, -2), 20 * 16, 150 * 16);

    if (on) {
        const qreal dotR = 2.6;
        QRectF dot(r.center().x() - dotR, r.center().y() - dotR, dotR * 2, dotR * 2);
        QPainterPath dotPath;
        dotPath.addEllipse(dot);
        p->setPen(Qt::NoPen);
        p->setBrush(disabled ? c->border
                             : opt->palette.color(QPalette::ButtonText));
        p->drawPath(dotPath);
    }
    p->restore();
}

void KeramikStyle::drawScrollBarButton(QPainter *p, const QStyleOption *opt,
                                       const QRect &r, PrimitiveElement arrow) const
{
    const KeramikColors *c = colors(opt->palette);
    const bool down = opt->state & (State_Sunken | State_On);
    const bool disabled = !(opt->state & State_Enabled);

    QStyleOption btn(*opt);
    btn.rect = r;
    drawButtonPanel(p, &btn);

    QColor arrowColor = disabled ? c->border
                                 : down ? c->border : opt->palette.color(QPalette::ButtonText);
    drawArrow(p, arrow, r.adjusted(1, 1, -1, -1), arrowColor);
}

// Classic 3-D splitter bar: raised bevel on a dark frame with a filled core.
void KeramikStyle::drawSplitter(QPainter *p, const QStyleOption *opt) const
{
    const QRect r = opt->rect;
    const int x2 = r.right();
    const int y2 = r.bottom();

    const QPalette &pal = opt->palette;
    const QColor win = pal.color(QPalette::Window);

    p->save();
    p->setPen(pal.color(QPalette::Dark));
    p->drawRect(r);

    // Corner gaps in the window colour, then the bevel edges.
    p->setPen(win);
    p->drawPoint(r.left(), r.top());
    p->drawPoint(x2, r.top());
    p->drawPoint(r.left(), y2);
    p->drawPoint(x2, y2);

    p->setPen(pal.color(QPalette::Light));
    p->drawLine(r.left() + 1, r.top() + 1, r.left() + 1, y2 - 1);
    p->drawLine(r.left() + 1, r.top() + 1, x2 - 1, r.top() + 1);
    p->setPen(pal.color(QPalette::Midlight));
    p->drawLine(r.left() + 2, r.top() + 2, r.left() + 2, y2 - 2);
    p->drawLine(r.left() + 2, r.top() + 2, x2 - 2, r.top() + 2);
    p->setPen(pal.color(QPalette::Mid));
    p->drawLine(x2 - 1, r.top() + 1, x2 - 1, y2 - 1);
    p->drawLine(r.left() + 1, y2 - 1, x2 - 1, y2 - 1);

    p->fillRect(r.left() + 3, r.top() + 3, r.width() - 5, r.height() - 5, win);
    p->restore();
}

// Sunken shade panel used behind the check mark of checked menu items
// (the original's qDrawShadePanel with a Midlight fill).
void KeramikStyle::drawMenuCheckPanel(QPainter *p, const QRect &r,
                                      const QPalette &pal) const
{
    const int x2 = r.right();
    const int y2 = r.bottom();

    p->save();
    p->fillRect(r, pal.color(QPalette::Midlight));
    p->setPen(pal.color(QPalette::Dark));
    p->drawLine(r.left(), r.top(), x2, r.top());
    p->drawLine(r.left(), r.top(), r.left(), y2);
    p->setPen(pal.color(QPalette::Light));
    p->drawLine(r.left(), y2, x2, y2);
    p->drawLine(x2, r.top(), x2, y2);
    p->restore();
}

// ---------------------------------------------------------------------------
// Polish / unpolish
// ---------------------------------------------------------------------------

void KeramikStyle::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);

    if (qobject_cast<QPushButton *>(widget) || qobject_cast<QToolButton *>(widget)
        || qobject_cast<QComboBox *>(widget))
        widget->setAttribute(Qt::WA_Hover);

    if (qobject_cast<QScrollBar *>(widget) || qobject_cast<QSlider *>(widget)
        || qobject_cast<QHeaderView *>(widget)) {
        widget->setAttribute(Qt::WA_Hover);
        widget->setMouseTracking(true);
    }
}

void KeramikStyle::polish(QPalette &palette)
{
    QProxyStyle::polish(palette);
    // The original Keramik kept the palette as-is; the accent colour for
    // handles is the standard highlight role.
    m_colorCache.clear();
}

void KeramikStyle::unpolish(QWidget *widget)
{
    QProxyStyle::unpolish(widget);
    widget->setAttribute(Qt::WA_Hover, false);
    widget->setMouseTracking(false);
}

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------

void KeramikStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt,
                                 QPainter *p, const QWidget *widget) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;

    switch (pe) {
    // Buttons -----------------------------------------------------------------
    case PE_PanelButtonCommand:
    case PE_PanelButtonBevel:
    case PE_PanelButtonTool:
        drawButtonPanel(p, opt);
        break;

    case PE_FrameDefaultButton:
        // The default-button ring is drawn as a darker border by
        // drawButtonPanel(); nothing further is needed here.
        break;

    case PE_FrameFocusRect: {
        // Square dotted frame in the palette text colour, matching the
        // original's drawWinFocusRect (a dotted black rectangle).
        p->save();
        p->setPen(QPen(opt->palette.color(QPalette::Text), 1, Qt::DotLine));
        p->setBrush(Qt::NoBrush);
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->restore();
        break;
    }

    // Input fields -------------------------------------------------------------
    case PE_PanelLineEdit:
        drawWell(p, opt);
        break;

    // Check boxes / radio buttons ---------------------------------------------
    case PE_IndicatorCheckBox: {
        const bool on = opt->state & State_On;
        const bool tri = opt->state & State_NoChange;
        drawCheckBoxIndicator(p, opt, on, tri);
        break;
    }

    case PE_IndicatorRadioButton: {
        drawRadioIndicator(p, opt, opt->state & State_On);
        break;
    }

    case PE_IndicatorMenuCheckMark: {
        const QColor mark = (opt->state & State_Enabled)
                ? opt->palette.color(QPalette::ButtonText) : c->border;
        drawCheckMark(p, r, mark);
        break;
    }

    case PE_IndicatorItemViewItemCheck: {
        QStyleOption box(*opt);
        box.rect = r.adjusted(0, 0, -1, -1);
        drawCheckBoxIndicator(p, &box, opt->state & State_On, false);
        break;
    }

    // Arrows -------------------------------------------------------------------
    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowLeft:
    case PE_IndicatorArrowRight: {
        const bool enabled = opt->state & State_Enabled;
        QColor color = enabled
                ? (opt->state & State_Sunken
                   ? c->border : opt->palette.color(QPalette::ButtonText))
                : c->border;
        p->save();
        if (opt->state & State_Sunken)
            p->translate(0, pixelMetric(PM_ButtonShiftVertical, opt, widget));
        const QColor etch = opt->palette.color(QPalette::Light);
        drawArrow(p, pe, r, color, enabled ? nullptr : &etch);
        p->restore();
        break;
    }

    case PE_IndicatorHeaderArrow:
        drawArrow(p, opt->state & State_UpArrow ? PE_IndicatorArrowUp
                                                : PE_IndicatorArrowDown,
                  r, c->border);
        break;

    // Scroll-bar --------------------------------------------------------------
    case PE_IndicatorProgressChunk:
        drawHighlightPanel(p, opt, r, 1);
        break;

    // Menus / menu bar ---------------------------------------------------------
    case PE_PanelMenu: {
        // The original paints the whole pop-up with the same slightly
        // lightened surface as the non-selected rows (background.light(105));
        // the rows then repaint themselves with the identical colour, so the
        // gaps between them never show a darker shade.
        p->save();
        p->setPen(Qt::NoPen);
        p->setBrush(menuSurfaceColor(opt->palette));
        p->drawRect(r);
        p->restore();
        break;
    }

    case PE_FrameMenu:
        p->save();
        p->setPen(c->border);
        p->setBrush(Qt::NoBrush);
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->restore();
        break;

    case PE_PanelMenuBar: {
        // The original paints the menu bar with the "menu" gradient:
        // slightly dark at the top, lighter at the bottom, tiled vertically.
        QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
        grad.setColorAt(0.0, c->menuGradTop);
        grad.setColorAt(1.0, c->menuGradBottom);
        p->save();
        p->setPen(Qt::NoPen);
        p->setBrush(grad);
        p->drawRect(r);
        p->setPen(opt->palette.color(QPalette::Mid));
        p->drawLine(r.right(), r.top(), r.right(), r.bottom());
        p->restore();
        break;
    }

    // Tool bar -----------------------------------------------------------------
    case PE_PanelToolBar: {
        p->save();
        const QStyleOptionToolBar *tb =
                qstyleoption_cast<const QStyleOptionToolBar *>(opt);
        const bool horizontal = toolbarHorizontal(tb);
        const QColor gradTop = colorLighten(
                opt->palette.color(QPalette::Button), 110);
        const QColor gradBot = colorLighten(
                opt->palette.color(QPalette::Button), 109);
        if (horizontal) {
            p->setPen(gradTop);
            p->drawLine(r.left(), r.top(), r.right(), r.top());
            p->setPen(gradBot);
            p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
            p->setPen(opt->palette.color(QPalette::Mid));
            p->drawLine(r.right(), r.top(), r.right(), r.bottom());
        } else {
            p->setPen(gradTop);
            p->drawLine(r.left(), r.top(), r.left(), r.bottom());
            p->setPen(gradBot);
            p->drawLine(r.right(), r.top(), r.right(), r.bottom());
            p->setPen(opt->palette.color(QPalette::Mid));
            p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        }
        p->restore();
        break;
    }

    case PE_IndicatorToolBarHandle: {
        const bool horiz = opt->state & State_Horizontal;
        const QColor light = opt->palette.color(QPalette::Light);
        const QColor dark = opt->palette.color(QPalette::Mid);
        p->save();
        if (horiz) {
            for (int i = 1; i <= 6; ++i) {
                p->setPen(i % 2 ? light : dark);
                p->drawLine(r.left() + i, r.top() + 4,
                            r.left() + i, r.bottom() - 4);
            }
        } else {
            for (int i = 1; i <= 6; ++i) {
                p->setPen(i % 2 ? light : dark);
                p->drawLine(r.left() + 4, r.top() + i,
                            r.right() - 4, r.top() + i);
            }
        }
        p->restore();
        break;
    }

    case PE_IndicatorToolBarSeparator: {
        const bool vertical = opt->state & State_Horizontal;
        p->save();
        p->setPen(opt->palette.color(QPalette::Mid));
        if (vertical) {
            p->drawLine(r.center().x(), r.top() + 3,
                        r.center().x(), r.bottom() - 3);
            p->setPen(opt->palette.color(QPalette::Light));
            p->drawLine(r.center().x() + 1, r.top() + 3,
                        r.center().x() + 1, r.bottom() - 3);
        } else {
            p->drawLine(r.left() + 3, r.center().y(),
                        r.right() - 3, r.center().y());
            p->setPen(opt->palette.color(QPalette::Light));
            p->drawLine(r.left() + 3, r.center().y() + 1,
                        r.right() - 3, r.center().y() + 1);
        }
        p->restore();
        break;
    }

    // Item views ---------------------------------------------------------------
    case PE_PanelItemViewItem:
        if ((opt->state & State_Selected) || (opt->state & State_MouseOver)) {
            QStyleOption sel(*opt);
            if (!(sel.state & State_Selected)) {
                sel.state |= State_Selected;
                sel.state &= ~State_MouseOver;
            }
            drawHighlightPanel(p, &sel, r.adjusted(0, 0, -1, -1), 3);
        }
        break;

    // Tabs ---------------------------------------------------------------------
    case PE_FrameTabBarBase:
        // The light "shelf" the inactive tabs sit on; the active tab's fill
        // covers it so its open bottom fuses with the page.
        p->save();
        p->setPen(c->innerTop);
        p->drawLine(r.left(), r.top(), r.right(), r.top());
        p->restore();
        break;

    case PE_FrameTabWidget: {
        // Thin Keramik page frame. The top edge doubles as the tab-bar base
        // line that separates the tabs from the page pane.
        p->save();
        p->setPen(c->border.lighter(145));
        p->drawLine(r.left(), r.top(), r.right() - 1, r.top());
        p->drawLine(r.left(), r.top(), r.left(), r.bottom() - 1);
        p->drawLine(r.right() - 1, r.top(), r.right() - 1, r.bottom() - 1);
        p->drawLine(r.left(), r.bottom() - 1, r.right() - 1, r.bottom() - 1);
        p->restore();
        break;
    }

    case PE_Widget:
    default:
        QProxyStyle::drawPrimitive(pe, opt, p, widget);
        break;
    }
}

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------

void KeramikStyle::drawTabShape(QPainter *p, const QStyleOptionTab *tab) const
{
    const KeramikColors *c = colors(tab->palette);
    const QRect r = tab->rect;
    const bool selected = tab->state & State_Selected;
    const QTabBar::Shape shape = tab->shape;
    const bool north = (shape == QTabBar::RoundedNorth || shape == QTabBar::TriangularNorth);
    const bool south = (shape == QTabBar::RoundedSouth || shape == QTabBar::TriangularSouth);
    const bool east  = (shape == QTabBar::RoundedEast  || shape == QTabBar::TriangularEast);
    const int corner = 3;

    // The active tab spans the whole tab-bar strip; inactive tabs are drawn
    // 3px back from the strip edge and 2px short of the open (page) side,
    // leaving the classic Keramik notch below/behind them.
    QRect tr = r.adjusted(0, 0, -1, 0);
    if (!selected) {
        if (north)
            tr.adjust(0, 3, 0, -2);
        else if (south)
            tr.adjust(0, 2, 0, -3);
        else if (east)
            tr.adjust(2, 0, -3, 0);
        else // west
            tr.adjust(3, 0, -2, 0);
    }

    // Fill shape and rim. The rounded edge faces away from the page; the page
    // side is left open on the active tab so its fill fuses with the pane.
    QPainterPath fill;
    QPainterPath rim;
    if (north || south) {
        const int outerY = north ? tr.top() : tr.bottom();  // rounded edge
        const int openY  = north ? tr.bottom() : tr.top();  // page side
        if (north) {
            fill.moveTo(tr.left(), openY);
            fill.lineTo(tr.left(), outerY + corner);
            fill.quadTo(tr.left(), outerY, tr.left() + corner, outerY);
            fill.lineTo(tr.right() - corner, outerY);
            fill.quadTo(tr.right(), outerY, tr.right(), outerY + corner);
            fill.lineTo(tr.right(), openY);
            rim.moveTo(tr.left(), outerY + corner);
            rim.quadTo(tr.left(), outerY, tr.left() + corner, outerY);
            rim.lineTo(tr.right() - corner, outerY);
            rim.quadTo(tr.right(), outerY, tr.right(), outerY + corner);
        } else {
            fill.moveTo(tr.left(), openY);
            fill.lineTo(tr.left(), outerY - corner);
            fill.quadTo(tr.left(), outerY, tr.left() + corner, outerY);
            fill.lineTo(tr.right() - corner, outerY);
            fill.quadTo(tr.right(), outerY, tr.right(), outerY - corner);
            fill.lineTo(tr.right(), openY);
            rim.moveTo(tr.left(), outerY - corner);
            rim.quadTo(tr.left(), outerY, tr.left() + corner, outerY);
            rim.lineTo(tr.right() - corner, outerY);
            rim.quadTo(tr.right(), outerY, tr.right(), outerY - corner);
        }
        if (!selected) {
            fill.closeSubpath();
            rim.lineTo(tr.right(), openY);
            rim.lineTo(tr.left(), openY);
            rim.closeSubpath();
        }
    } else {
        const int outerX = east ? tr.right() : tr.left();   // rounded edge
        const int openX  = east ? tr.left() : tr.right();   // page side
        if (east) {
            fill.moveTo(openX, tr.top());
            fill.lineTo(outerX - corner, tr.top());
            fill.quadTo(outerX, tr.top(), outerX, tr.top() + corner);
            fill.lineTo(outerX, tr.bottom() - corner);
            fill.quadTo(outerX, tr.bottom(), outerX - corner, tr.bottom());
            fill.lineTo(openX, tr.bottom());
            rim.moveTo(outerX - corner, tr.top());
            rim.quadTo(outerX, tr.top(), outerX, tr.top() + corner);
            rim.lineTo(outerX, tr.bottom() - corner);
            rim.quadTo(outerX, tr.bottom(), outerX - corner, tr.bottom());
        } else { // west
            fill.moveTo(openX, tr.top());
            fill.lineTo(outerX + corner, tr.top());
            fill.quadTo(outerX, tr.top(), outerX, tr.top() + corner);
            fill.lineTo(outerX, tr.bottom() - corner);
            fill.quadTo(outerX, tr.bottom(), outerX + corner, tr.bottom());
            fill.lineTo(openX, tr.bottom());
            rim.moveTo(outerX + corner, tr.top());
            rim.quadTo(outerX, tr.top(), outerX, tr.top() + corner);
            rim.lineTo(outerX, tr.bottom() - corner);
            rim.quadTo(outerX, tr.bottom(), outerX + corner, tr.bottom());
        }
        if (!selected) {
            fill.closeSubpath();
            rim.lineTo(openX, tr.bottom());
            rim.lineTo(openX, tr.top());
            rim.closeSubpath();
        }
    }

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient grad;
    if (north || south)
        grad = QLinearGradient(tr.left(), tr.top(), tr.left(), tr.bottom());
    else
        grad = QLinearGradient(tr.left(), tr.top(), tr.right(), tr.top());
    if (selected) {
        // Bright "lifted" tab: near-white fill that stays light down to the
        // page edge, mirroring the KDE 3 reference.
        grad.setColorAt(0.0, c->gradientTop);
        grad.setColorAt(1.0, c->gradientBottom.lighter(115));
    } else {
        // Sitting on the shelf: a clearly darker grey that deepens toward
        // the base line, so the selected tab stands out.
        grad.setColorAt(0.0, c->gradientTop.darker(102));
        grad.setColorAt(1.0, c->gradientBottom.lighter(106));
    }
    p->setPen(Qt::NoPen);
    p->setBrush(grad);
    p->drawPath(fill);

    p->setBrush(Qt::NoBrush);
    p->setPen(c->border.lighter(140));
    p->drawPath(rim);

    // Ceramic sheen along the outer edge.
    p->setPen(c->innerTop);
    if (north)
        p->drawLine(tr.left() + 4, tr.top() + 1, tr.right() - 4, tr.top() + 1);
    else if (south)
        p->drawLine(tr.left() + 4, tr.bottom() - 1, tr.right() - 4, tr.bottom() - 1);
    else if (east)
        p->drawLine(tr.right() - 1, tr.top() + 4, tr.right() - 1, tr.bottom() - 4);
    else // west
        p->drawLine(tr.left() + 1, tr.top() + 4, tr.left() + 1, tr.bottom() - 4);

    p->restore();
}

void KeramikStyle::drawMenuBarItem(QPainter *p, const QStyleOptionMenuItem *mi) const
{
    const KeramikColors *c = colors(mi->palette);
    const QRect r = mi->rect;
    const bool active = mi->state & State_Selected;

    // Sunken shade panel (the original's qDrawShadePanel with a Midlight
    // fill): dark top/left, light bottom/right.
    if (active) {
        p->save();
        p->fillRect(r.adjusted(1, 1, -1, -1),
                    mi->palette.color(QPalette::Midlight));
        p->setPen(mi->palette.color(QPalette::Dark));
        p->drawLine(r.left(), r.top(), r.right(), r.top());
        p->drawLine(r.left(), r.top(), r.left(), r.bottom());
        p->setPen(mi->palette.color(QPalette::Light));
        p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        p->drawLine(r.right(), r.top(), r.right(), r.bottom());
        p->restore();
    } else {
        // The original repaints the bar gradient behind every non-selected
        // item so the item background always matches the surrounding surface.
        QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
        grad.setColorAt(0.0, c->menuGradTop);
        grad.setColorAt(1.0, c->menuGradBottom);
        p->save();
        p->setPen(Qt::NoPen);
        p->setBrush(grad);
        p->drawRect(r);
        p->restore();
    }

    drawItemText(p, r, Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextSingleLine,
                 mi->palette, mi->state & State_Enabled, mi->text,
                 QPalette::ButtonText);
}

void KeramikStyle::drawMenuItem(QPainter *p, const QStyleOptionMenuItem *mi) const
{
    const KeramikColors *c = colors(mi->palette);
    const QRect r = mi->rect;
    const bool selected = mi->state & State_Selected;
    const bool enabled = mi->state & State_Enabled;

    // Row background ------------------------------------------------------
    // Filled first for every row, separators included, exactly as the
    // original's CE_PopupMenuItem (fillRect background.light(105), then the
    // separator lines) -- so the separator area never relies on Qt having
    // painted PE_PanelMenu underneath it.
    if (selected) {
        QStyleOption sel(*mi);
        sel.state |= State_Selected;
        drawHighlightPanel(p, &sel, r.adjusted(0, 0, -1, -1), 3);
    } else {
        p->save();
        p->setPen(Qt::NoPen);
        p->setBrush(menuSurfaceColor(mi->palette));
        p->drawRect(r);
        p->restore();
    }

    // Separator ---------------------------------------------------------------
    if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
        // Two 1px lines just under the top of the item: dark mid tone, then
        // a light highlight beneath it (as in the original).
        p->save();
        p->setPen(mi->palette.color(QPalette::Mid));
        p->drawLine(r.left() + 5, r.top() + 1, r.right() - 5, r.top() + 1);
        p->setPen(mi->palette.color(QPalette::Light));
        p->drawLine(r.left() + 5, r.top() + 2, r.right() - 5, r.top() + 2);
        p->restore();
        return;
    }

    // Check / icon column ------------------------------------------------------
    // Reserve a column only when the menu actually shows icons or check
    // boxes; a plain text menu keeps a flush left edge (as in the original).
    const int maxpmw = mi->maxIconWidth;
    const int checkcol = mi->menuHasCheckableItems ? qMax(maxpmw, 20) : maxpmw;
    const QRect checkRect(r.left() + itemFrame, r.top() + itemFrame,
                          qMax(checkcol, 1) - 1, r.height() - 2 * itemFrame);
    if (mi->checked) {
        // Sunken shade panel behind the check column (the original's
        // qDrawShadePanel with a Midlight fill), used both with an icon
        // (panel under the icon) and without one (panel under the check).
        drawMenuCheckPanel(p, checkRect.adjusted(1, 1, -1, -1), mi->palette);
        if (mi->icon.isNull())
            drawCheckMark(p, checkRect.adjusted(0, 0, -1, -1),
                          enabled ? (selected ? mi->palette.color(QPalette::HighlightedText)
                                              : mi->palette.color(QPalette::ButtonText))
                                  : c->border);
    }

    if (!mi->icon.isNull()) {
        const QIcon::Mode mode = !enabled ? QIcon::Disabled
                                : selected ? QIcon::Active : QIcon::Normal;
        const QPixmap pm = mi->icon.pixmap(QSize(16, 16),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                           QStyleHelper::getDpr(p),
#endif
                                           mode);
        const QSize iconSize(pm.width() / pm.devicePixelRatio(),
                             pm.height() / pm.devicePixelRatio());
        const QRect iconRect(QPoint(0, 0), iconSize);
        QRect target(checkRect.center().x() - iconRect.width() / 2,
                     checkRect.center().y() - iconRect.height() / 2,
                     iconRect.width(), iconRect.height());
        p->drawPixmap(target, pm);
    }

    // Label and shortcut -------------------------------------------------------
    if (!mi->text.isEmpty()) {
        QFont savedFont = p->font();
        p->setFont(mi->font);

        QString text = mi->text;
        QString shortcut;
        const int tab = text.indexOf(QLatin1Char('\t'));
        if (tab >= 0) {
            shortcut = text.mid(tab + 1);
            text = text.left(tab);
        }

        const int shortcutW = menuItemTabWidth(mi);
        const int xm = itemFrame + checkcol + itemHMargin;
        const QRect textRect(r.left() + xm, r.top(),
                             r.width() - xm - shortcutW - arrowHMargin
                                     - itemHMargin * 3 - itemFrame + 1,
                             r.height());
        QColor textColor = !enabled
                ? mi->palette.color(QPalette::Disabled, QPalette::Text)
                : selected ? mi->palette.color(QPalette::HighlightedText)
                           : mi->palette.color(QPalette::Text);
        p->setPen(textColor);

        if (!shortcut.isEmpty()) {
            // The accelerator column is right-aligned at the menu's shortcut
            // edge (rightBorder + itemHMargin + itemFrame from the right).
            const QRect shortcutRect(r.right() - shortcutW - rightBorder
                                     - itemHMargin - itemFrame + 1, r.top(),
                                     shortcutW, r.height());
            drawItemText(p, shortcutRect, Qt::AlignVCenter | Qt::AlignRight
                                          | Qt::TextSingleLine,
                         mi->palette, enabled, shortcut,
                         selected ? QPalette::HighlightedText : QPalette::Text);
        }

        drawItemText(p, textRect, Qt::AlignVCenter | Qt::AlignLeft
                                          | Qt::TextShowMnemonic | Qt::TextSingleLine,
                     mi->palette, enabled, text,
                     selected ? QPalette::HighlightedText : QPalette::Text);

        p->setFont(savedFont);
    }

    // Sub-menu arrow -----------------------------------------------------------
    if (mi->menuItemType == QStyleOptionMenuItem::SubMenu) {
        const int dim = pixelMetric(PM_MenuButtonIndicator, mi, nullptr);
        const QRect ar(r.right() - arrowHMargin - itemFrame - dim + 1,
                       r.top() + (r.height() - dim) / 2, dim, dim);
        drawArrow(p, PE_IndicatorArrowRight, ar,
                  !enabled ? c->border
                           : selected ? mi->palette.color(QPalette::HighlightedText)
                                      : mi->palette.color(QPalette::Text));
    }
}

void KeramikStyle::drawControl(ControlElement ce, const QStyleOption *opt,
                               QPainter *p, const QWidget *widget) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;

    switch (ce) {
    // Progress bar --------------------------------------------------------------
    case CE_ProgressBarGroove:
        drawGroove(p, opt, r.adjusted(1, 1, -2, -2));
        break;

    case CE_ProgressBarContents:
        // Handled by the base style through PE_IndicatorProgressChunk.
        QProxyStyle::drawControl(ce, opt, p, widget);
        break;

    // Tabs ----------------------------------------------------------------------
    case CE_TabBarTab: {
        const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tab)
            break;
        drawTabShape(p, tab);
        QStyleOptionTab labelOpt(*tab);
        QProxyStyle::drawControl(CE_TabBarTabLabel, &labelOpt, p, widget);
        break;
    }

    // Menus ---------------------------------------------------------------------
    case CE_MenuBarEmptyArea: {
        // "Menu" gradient, dark top to light bottom (see PE_PanelMenuBar).
        QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
        grad.setColorAt(0.0, c->menuGradTop);
        grad.setColorAt(1.0, c->menuGradBottom);
        p->save();
        p->setPen(Qt::NoPen);
        p->setBrush(grad);
        p->drawRect(r);
        p->restore();
        break;
    }

    case CE_MenuBarItem: {
        const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (mi)
            drawMenuBarItem(p, mi);
        break;
    }

    case CE_MenuItem: {
        const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (mi)
            drawMenuItem(p, mi);
        break;
    }

    case CE_MenuEmptyArea:
        // The menu background is drawn by PE_PanelMenu.
        break;

    // Tool bar ------------------------------------------------------------------
    case CE_ToolBar: {
        const QStyleOptionToolBar *tb = qstyleoption_cast<const QStyleOptionToolBar *>(opt);
        const bool horizontal = toolbarHorizontal(tb);
        // The original's toolbar surface ramps bright -> grey -> bright
        // (3/4 down it passes through the dimmest tone), unlike the menu
        // gradient which is dark on top.
        const QColor b = opt->palette.color(QPalette::Button);
        const QColor gradTop = colorLighten(b, 110);
        const QColor gradMid = b.lighter(94);
        const QColor gradBot = colorLighten(b, 109);
        QLinearGradient grad = horizontal
                ? QLinearGradient(r.left(), r.top(), r.left(), r.bottom())
                : QLinearGradient(r.left(), r.top(), r.right(), r.top());
        grad.setColorAt(0.0, gradTop);
        grad.setColorAt(0.75, gradMid);
        grad.setColorAt(1.0, gradBot);
        p->save();
        p->setPen(Qt::NoPen);
        p->setBrush(grad);
        p->drawRect(r);
        // Frame: gradient edges on the "lit" sides, a flat mid line on the
        // end, as in PE_PanelDockWindow of the original.
        if (horizontal) {
            p->setPen(gradTop);
            p->drawLine(r.left(), r.top(), r.right(), r.top());
            p->setPen(gradBot);
            p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
            QLinearGradient lgrad(r.left(), r.top(), r.left(), r.bottom());
            lgrad.setColorAt(0.0, gradTop);
            lgrad.setColorAt(0.75, gradMid);
            lgrad.setColorAt(1.0, gradBot);
            p->setBrush(lgrad);
            p->drawRect(r.left(), r.top(), 1, r.height());
            p->setPen(opt->palette.color(QPalette::Mid));
            p->drawLine(r.right(), r.top(), r.right(), r.bottom());
        } else {
            p->setPen(gradTop);
            p->drawLine(r.left(), r.top(), r.left(), r.bottom());
            p->setPen(gradBot);
            p->drawLine(r.right(), r.top(), r.right(), r.bottom());
            QLinearGradient tgrad(r.left(), r.top(), r.right(), r.top());
            tgrad.setColorAt(0.0, gradTop);
            tgrad.setColorAt(0.75, gradMid);
            tgrad.setColorAt(1.0, gradBot);
            p->setBrush(tgrad);
            p->drawRect(r.left(), r.top(), r.width(), 1);
            p->setPen(opt->palette.color(QPalette::Mid));
            p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        }
        p->restore();
        break;
    }

    // Header --------------------------------------------------------------------
    case CE_HeaderSection:
        // The pressed state is preserved and read by drawButtonPanel.
        drawButtonPanel(p, opt);
        p->save();
        p->setPen(c->border.lighter(140));
        p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        p->drawLine(r.right(), r.top(), r.right(), r.bottom());
        p->restore();
        break;

    case CE_HeaderEmptyArea: {
        p->save();
        p->fillRect(r, c->gradientBottom.lighter(112));
        p->setPen(c->border.lighter(140));
        p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        p->restore();
        break;
    }

    // Splitter ------------------------------------------------------------------
    case CE_Splitter:
        drawSplitter(p, opt);
        break;

    default:
        QProxyStyle::drawControl(ce, opt, p, widget);
        break;
    }
}

// ---------------------------------------------------------------------------
// Complex controls
// ---------------------------------------------------------------------------

void KeramikStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                                      QPainter *p, const QWidget *widget) const
{
    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;

    switch (cc) {
    // Combo box -----------------------------------------------------------------
    case CC_ComboBox: {
        const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
        if (!cmb)
            break;
        const bool disabled = !(opt->state & State_Enabled);
        const bool activeArrow = opt->activeSubControls & SC_ComboBoxArrow;

        if (opt->subControls & SC_ComboBoxFrame) {
            // Raised button panel over the whole combo, as in the original.
            QStyleOption btn(*opt);
            if (activeArrow)
                btn.state |= State_Sunken;
            drawButtonPanel(p, &btn);

            // Editable combos get a sunken well for the edit field.
            if (cmb->editable && opt->subControls & SC_ComboBoxEditField) {
                QRect er = subControlRect(cc, opt, SC_ComboBoxEditField, widget);
                if (er.isValid()) {
                    QStyleOption well(*opt);
                    well.rect = er.adjusted(-2, -2, 2, 2);
                    drawWell(p, &well);
                }
            }
        }

        if (opt->subControls & SC_ComboBoxArrow) {
            const QRect ar = subControlRect(cc, opt, SC_ComboBoxArrow, widget);
            // Keramik "ripple" next to the arrow, then the combo arrow itself.
            const QRect rippleRect = QStyle::visualRect(opt->direction, r,
                    QRect(ar.left() - 15, ar.top() + 2, 11, ar.height() - 4));
            if (rippleRect.width() > 2 && rippleRect.height() > 4)
                drawRipple(p, rippleRect,
                           disabled ? c->border : opt->palette.color(QPalette::ButtonText));
            const QRect arrowRect = QStyle::visualRect(opt->direction, r,
                    QRect(ar.right() - 11, ar.top(), 9, ar.height()));
            drawComboArrow(p, arrowRect,
                           disabled ? c->border : opt->palette.color(QPalette::ButtonText));
        }
        break;
    }

    // Slider --------------------------------------------------------------------
    case CC_Slider: {
        const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!slider)
            break;

        if (opt->subControls & SC_SliderGroove) {
            const QRect groove = subControlRect(cc, opt, SC_SliderGroove, widget);
            if (groove.isValid()) {
                if (opt->state & State_HasFocus) {
                    QStyleOption focus(*opt);
                    focus.rect = groove;
                    focus.state = QStyle::State();
                    drawPrimitive(PE_FrameFocusRect, &focus, p, widget);
                }
                drawGroove(p, opt, groove.adjusted(0, 0, 0, 0));
            }
        }

        if (opt->subControls & SC_SliderHandle) {
            const QRect handle = subControlRect(cc, opt, SC_SliderHandle, widget);
            if (handle.isValid()) {
                QStyleOption thumb(*opt);
                if (opt->activeSubControls & SC_SliderHandle)
                    thumb.state |= State_MouseOver;
                drawHighlightPanel(p, &thumb, handle, 3);
            }
        }

        if (opt->subControls & SC_SliderTickmarks) {
            QStyleOptionComplex copy(*opt);
            copy.subControls = SC_SliderTickmarks;
            QProxyStyle::drawComplexControl(cc, &copy, p, widget);
        }
        break;
    }

    // Scroll bar -----------------------------------------------------------------
    case CC_ScrollBar: {
        const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!sb)
            break;

        const bool horiz = opt->state & State_Horizontal;

        const QRect subline = subControlRect(cc, opt, SC_ScrollBarSubLine, widget);
        const QRect addline = subControlRect(cc, opt, SC_ScrollBarAddLine, widget);
        const QRect groove = subControlRect(cc, opt, SC_ScrollBarGroove, widget);
        const QRect slider = subControlRect(cc, opt, SC_ScrollBarSlider, widget);

        // Track: recessed groove spanning the area between the buttons.
        if (groove.isValid())
            drawGroove(p, opt, groove);

        // Thumb: palette-highlight panel, brighter while hovered/dragged.
        if (slider.isValid() && opt->subControls & SC_ScrollBarSlider) {
            QStyleOption thumb(*opt);
            if (opt->activeSubControls & SC_ScrollBarSlider)
                thumb.state |= State_MouseOver;
            drawHighlightPanel(p, &thumb, slider, 2);
        }

        // End buttons. The horizontal add line keeps the original Keramik
        // "double arrow" design: a wide button with a divider whose two
        // halves scroll by page and by line respectively.
        if (subline.isValid() && opt->subControls & SC_ScrollBarSubLine)
            drawScrollBarButton(p, opt, subline,
                                horiz ? PE_IndicatorArrowLeft : PE_IndicatorArrowUp);
        if (addline.isValid() && opt->subControls & SC_ScrollBarAddLine) {
            QStyleOption btn(*opt);
            btn.rect = addline;
            drawButtonPanel(p, &btn);
            const QColor arrowColor = (opt->state & (State_Sunken | State_On))
                    ? c->border : opt->palette.color(QPalette::ButtonText);
            const QColor divider = opt->palette.color(QPalette::ButtonText);
            if (horiz) {
                // Short vertical divider at the middle of the double arrow.
                p->save();
                p->setPen(divider);
                p->drawLine(addline.center().x() - 1,
                            addline.center().y() - 3,
                            addline.center().x() - 1,
                            addline.center().y() + 3);
                p->restore();
                const QRect left(addline.left(), addline.top(),
                                 addline.width() / 2, addline.height());
                const QRect right(left.right() + 1, addline.top(),
                                  addline.width() - left.width(), addline.height());
                drawArrow(p, PE_IndicatorArrowLeft, left, arrowColor);
                drawArrow(p, PE_IndicatorArrowRight, right, arrowColor);
            } else {
                // Short horizontal divider at the middle of the double arrow.
                p->save();
                p->setPen(divider);
                p->drawLine(addline.center().x() - 4, addline.center().y(),
                            addline.center().x() + 2, addline.center().y());
                p->restore();
                const QRect top(addline.left(), addline.top(),
                                addline.width(), addline.height() / 2);
                const QRect bottom(top.left(), top.bottom() + 1, addline.width(),
                                   addline.height() - top.height());
                drawArrow(p, PE_IndicatorArrowUp, top, arrowColor);
                drawArrow(p, PE_IndicatorArrowDown, bottom, arrowColor);
            }
        }
        break;
    }

    // Spin box -----------------------------------------------------------------
    case CC_SpinBox: {
        const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
        if (!sb)
            break;

        if (opt->subControls & SC_SpinBoxFrame) {
            QStyleOption base(*opt);
            base.state &= ~State_HasFocus;
            drawWell(p, &base);
        }

        if (opt->subControls & SC_SpinBoxUp) {
            const QRect up = subControlRect(cc, opt, SC_SpinBoxUp, widget);
            if (up.isValid()) {
                QStyleOption btn(*opt);
                btn.rect = up;
                if (opt->activeSubControls & SC_SpinBoxUp)
                    btn.state |= State_Sunken;
                drawButtonPanel(p, &btn);
                drawArrow(p, PE_IndicatorArrowUp, up,
                          (opt->state & State_Enabled)
                                  ? opt->palette.color(QPalette::ButtonText) : c->border);
            }
        }
        if (opt->subControls & SC_SpinBoxDown) {
            const QRect down = subControlRect(cc, opt, SC_SpinBoxDown, widget);
            if (down.isValid()) {
                QStyleOption btn(*opt);
                btn.rect = down;
                if (opt->activeSubControls & SC_SpinBoxDown)
                    btn.state |= State_Sunken;
                drawButtonPanel(p, &btn);
                drawArrow(p, PE_IndicatorArrowDown, down,
                          (opt->state & State_Enabled)
                                  ? opt->palette.color(QPalette::ButtonText) : c->border);
            }
        }
        break;
    }

    default:
        QProxyStyle::drawComplexControl(cc, opt, p, widget);
        break;
    }
}

// ---------------------------------------------------------------------------
// Metrics and hints
// ---------------------------------------------------------------------------

int KeramikStyle::pixelMetric(PixelMetric pm, const QStyleOption *opt,
                              const QWidget *widget) const
{
    switch (pm) {
    case PM_ButtonMargin:
        return qRound(QStyleHelper::dpiScaled(4, opt));
    case PM_ButtonShiftHorizontal:
        return 0;
    case PM_ButtonShiftVertical:
        return 1;
    case PM_ButtonDefaultIndicator:
        return 0;
    case PM_DefaultFrameWidth:
        return 1;
    case PM_ScrollBarExtent:
        return qRound(QStyleHelper::dpiScaled(15, opt));
    case PM_ScrollBarSliderMin:
        return qRound(QStyleHelper::dpiScaled(24, opt));
    case PM_SliderThickness:
        return qRound(QStyleHelper::dpiScaled(21, opt));
    case PM_SliderControlThickness:
        return qRound(QStyleHelper::dpiScaled(9, opt));
    case PM_SliderLength:
        // The original theme uses a compact thumb, about 12px long.
        return qRound(QStyleHelper::dpiScaled(12, opt));
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
        return qRound(QStyleHelper::dpiScaled(13, opt));
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        return qRound(QStyleHelper::dpiScaled(13, opt));
    case PM_MenuButtonIndicator:
        return qRound(QStyleHelper::dpiScaled(13, opt));
    case PM_MenuPanelWidth:
        return 1;
    case PM_MenuBarItemSpacing:
        return 0;
    case PM_TabBarTabOverlap:
        return 0;
    case PM_TabBarBaseOverlap:
        // The tab bar sits flush on the page pane; the base line is drawn as
        // the top edge of PE_FrameTabWidget, so the pane must not overlap it.
        return 0;
    case PM_TabBarTabHSpace:
        return qRound(QStyleHelper::dpiScaled(16, opt));
    case PM_TabBarTabVSpace:
        return qRound(QStyleHelper::dpiScaled(12, opt));
    case PM_TabBarBaseHeight:
        return 1;
    case PM_DockWidgetSeparatorExtent:
        return qRound(QStyleHelper::dpiScaled(4, opt));
    case PM_ProgressBarChunkWidth:
        return qRound(QStyleHelper::dpiScaled(4, opt));
    case PM_TitleBarHeight:
        return qRound(QStyleHelper::dpiScaled(22, opt));
    case PM_SplitterWidth:
        return qRound(QStyleHelper::dpiScaled(6, opt));
    default:
        break;
    }
    return QProxyStyle::pixelMetric(pm, opt, widget);
}

QRect KeramikStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                                   SubControl sc, const QWidget *widget) const
{
    // The original Keramik gives the scroll bar a narrow single-arrow
    // "sub" button and a wide "add" button that holds a double arrow.
    // QCommonStyle sizes both buttons with PM_ScrollBarExtent, which would
    // crush the double arrow into a few pixels, so lay the bar out by hand.
    if (cc == CC_ScrollBar) {
        const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (sb) {
            const bool horizontal = sb->orientation == Qt::Horizontal;
            const int subline = 16;   // single-arrow button
            const int addline = 31;   // double-arrow button
            const QRect &rc = opt->rect;

            const int maxlen = horizontal
                    ? rc.width() - subline - addline + 2
                    : rc.height() - subline - addline + 2;

            int sliderlen = maxlen;
            if (sb->minimum != sb->maximum) {
                const int range = sb->maximum - sb->minimum;
                sliderlen = (sb->pageStep * maxlen) / (range + sb->pageStep);
                const int minLen = qMax(24, maxlen / 8);
                if (sliderlen < minLen)
                    sliderlen = minLen;
                if (sliderlen > maxlen)
                    sliderlen = maxlen;
            }

            int sliderpos = 0;
            if (sb->minimum != sb->maximum)
                sliderpos = (sb->sliderPosition - sb->minimum) * (maxlen - sliderlen)
                        / (sb->maximum - sb->minimum);

            switch (sc) {
            case SC_ScrollBarGroove:
                return horizontal ? QRect(subline, 0, maxlen, rc.height())
                                  : QRect(0, subline, rc.width(), maxlen);
            case SC_ScrollBarSlider:
                return horizontal ? QRect(sliderpos + subline, 0, sliderlen, rc.height())
                                  : QRect(0, sliderpos + subline, rc.width(), sliderlen);
            case SC_ScrollBarSubLine:
                return horizontal ? QRect(0, 0, subline, rc.height())
                                  : QRect(0, 0, rc.width(), subline);
            case SC_ScrollBarAddLine:
                return horizontal ? QRect(rc.width() - addline, 0, addline, rc.height())
                                  : QRect(0, rc.height() - addline, rc.width(), addline);
            case SC_ScrollBarSubPage:
                return horizontal ? QRect(subline, 0, sliderpos, rc.height())
                                  : QRect(0, subline, rc.width(), sliderpos);
            case SC_ScrollBarAddPage:
                return horizontal
                        ? QRect(sliderpos + sliderlen + subline, 0,
                                maxlen - sliderpos - sliderlen, rc.height())
                        : QRect(0, sliderpos + sliderlen + subline,
                                rc.width(), maxlen - sliderpos - sliderlen);
            default:
                break;
            }
        }
    }
    return QProxyStyle::subControlRect(cc, opt, sc, widget);
}

QSize KeramikStyle::sizeFromContents(ContentsType ct, const QStyleOption *opt,
                                     const QSize &contentsSize, const QWidget *widget) const
{
    switch (ct) {
    case CT_PushButton: {
        // Original Keramik: content + 2*PM_ButtonMargin, then +30 wide and
        // +5 tall for labelled buttons (compact icon-only buttons stay tight).
        const int margin = pixelMetric(PM_ButtonMargin, opt, widget);
        int w = contentsSize.width() + 2 * margin;
        int h = contentsSize.height() + 2 * margin;
        const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
        if ((!btn || !btn->text.isEmpty()) || contentsSize.width() >= 32) {
            w += 30;
            h += 5;
        }
        return QSize(w, h);
    }
    case CT_ComboBox: {
        // Original Keramik: width is the content plus the arrow block
        // (11px shaft + ripple) plus 26/22px of surrounding margins.
        const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
        const int arrow = 11 + 16;   // arrow shaft + ripple
        const int extra = (cmb && cmb->editable) ? 26 : 22;
        return QSize(contentsSize.width() + arrow + extra,
                     contentsSize.height() + 10);
    }
    case CT_ToolButton: {
        int w = contentsSize.width() + 12;
        int h = contentsSize.height() + 8;
        return QSize(w, h);
    }
    case CT_LineEdit: {
        int h = contentsSize.height() + 6;
        return QSize(contentsSize.width() + 4, h);
    }
    case CT_MenuItem: {
        // Original Keramik CT_PopupMenuItem formula. The accelerator width
        // itself is part of contentsSize (added by QMenu as tabWidth), so
        // only the gaps and columns are added here.
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            if (mi->menuItemType == QStyleOptionMenuItem::Separator)
                return QSize(10, 3);
            int w = contentsSize.width();
            if (mi->text.contains(QLatin1Char('\t')))
                w += itemHMargin + itemFrame * 2 + 7;   // gap before the accelerator
            if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += 2 * arrowHMargin;                  // sub-menu arrow
            const int maxpmw = mi->maxIconWidth;
            if (maxpmw > 0)
                w += maxpmw + 6;                        // icon column + spacing
            if (mi->menuHasCheckableItems && maxpmw < 20)
                w += 20 - maxpmw;                       // check column (min 20)
            if (mi->menuHasCheckableItems || maxpmw > 0)
                w += 12;
            w += rightBorder;                           // right edge padding
            const int h = qMax(contentsSize.height(), mi->fontMetrics.height() + 8);
            return QSize(w, h);
        }
        break;
    }
    default:
        break;
    }
    return QProxyStyle::sizeFromContents(ct, opt, contentsSize, widget);
}

int KeramikStyle::styleHint(StyleHint sh, const QStyleOption *opt,
                            const QWidget *widget, QStyleHintReturn *shret) const
{
    switch (sh) {
    case SH_EtchDisabledText:
        return 1;
    case SH_Menu_MouseTracking:
        return 1;
    default:
        break;
    }
    return QProxyStyle::styleHint(sh, opt, widget, shret);
}
