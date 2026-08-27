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
#include <QtWidgets/qdockwidget.h>
#include <QtWidgets/qgroupbox.h>
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
#include <QtWidgets/qtoolbox.h>
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qtoolbutton.h>
#include <QtWidgets/qstylefactory.h>

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpalette.h>

#include <QtCore/qmath.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>
#include <QtCore/qcoreevent.h>

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

KeramikStyle::KeramikStyle(bool forceClassicPalette)
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Windows"))),
      m_forceClassicPalette(forceClassicPalette)
{
    // Drive the busy (indeterminate) progress bar animation from a QTimer.
    // A plain startTimer() on this style would deliver timer events through
    // QProxyStyle::event(), which on Qt 5 forwards every event to the base
    // style, so timerEvent() would never be called.  A QTimer's timeout
    // signal bypasses QStyle::event() entirely, so the animation behaves
    // identically on Qt 5 and Qt 6 (same trick as phase/winxp in this repo).
    m_busyTimer = new QTimer(this);
    connect(m_busyTimer, &QTimer::timeout, this, &KeramikStyle::animateProgressBars);
}
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

// ---------------------------------------------------------------------------
// Shared drawing helpers
// ---------------------------------------------------------------------------

// Keramik arrow line segments, origin at the arrow centre. These reproduce
// the original theme's thin line-drawn arrows (horizontal "slices" of a
// triangle); when etched, the light pass is drawn one pixel down/right first.
struct ArrowSeg { qint8 x1, y1, x2, y2; };

// Up/left are mirrors of down/right so opposite directions share the same
// bbox centering.
static const ArrowSeg keramikDownArrow[] = {
    { -4, -2,  3, -2 }, { -4, -1,  3, -1 }, { -3,  0,  2,  0 },
    { -2,  1,  1,  1 }, { -1,  2,  0,  2 }
};
static const ArrowSeg keramikUpArrow[] = {
    { -1, -2,  0, -2 }, { -2, -1,  1, -1 }, { -3,  0,  2,  0 },
    { -4,  1,  3,  1 }, { -4,  2,  3,  2 }
};
static const ArrowSeg keramikRightArrow[] = {
    { -2, -4, -2,  3 }, { -1, -4, -1,  3 }, {  0, -3,  0,  2 },
    {  1, -2,  1,  1 }, {  2, -1,  2,  0 }
};
static const ArrowSeg keramikLeftArrow[] = {
    { -2, -1, -2,  0 }, { -1, -2, -1,  1 }, {  0, -3,  0,  2 },
    {  1, -4,  1,  3 }, {  2, -4,  2,  3 }
};
static const ArrowSeg keramikComboArrow[] = {
    { -2, -2,  2, -2 }, { -2, -1,  2, -1 },
    { -2,  0,  2,  0 }, { -4,  1,  4,  1 }, { -3,  2,  3,  2 },
    { -2,  3,  2,  3 }, { -1,  4,  1,  4 }, {  0,  5,  0,  5 }
};

// Centre the glyph's axis-aligned bbox in r on both axes.  Leftover pixels
// follow Qt's AlignCenter convention (far / bottom-right side).
static void arrowOrigin(const QRect &r, const ArrowSeg *seg, int count,
                        int *cx, int *cy)
{
    int minX = 127, minY = 127, maxX = -128, maxY = -128;
    for (int i = 0; i < count; ++i) {
        minX = qMin(minX, qMin(int(seg[i].x1), int(seg[i].x2)));
        maxX = qMax(maxX, qMax(int(seg[i].x1), int(seg[i].x2)));
        minY = qMin(minY, qMin(int(seg[i].y1), int(seg[i].y2)));
        maxY = qMax(maxY, qMax(int(seg[i].y1), int(seg[i].y2)));
    }
    const int gw = maxX - minX + 1;
    const int gh = maxY - minY + 1;
    *cx = r.left() + (r.width() - gw) / 2 - minX;
    *cy = r.top() + (r.height() - gh) / 2 - minY;
}

void KeramikStyle::drawArrow(QPainter *p, PrimitiveElement pe, const QRect &r,
                             const QColor &color, const QColor *etch) const
{
    const ArrowSeg *seg = nullptr;
    int count = 0;
    switch (pe) {
    case PE_IndicatorArrowUp:
        seg = keramikUpArrow;
        count = int(sizeof keramikUpArrow / sizeof *keramikUpArrow);
        break;
    case PE_IndicatorArrowDown:
        seg = keramikDownArrow;
        count = int(sizeof keramikDownArrow / sizeof *keramikDownArrow);
        break;
    case PE_IndicatorArrowLeft:
        seg = keramikLeftArrow;
        count = int(sizeof keramikLeftArrow / sizeof *keramikLeftArrow);
        break;
    case PE_IndicatorArrowRight:
        seg = keramikRightArrow;
        count = int(sizeof keramikRightArrow / sizeof *keramikRightArrow);
        break;
    default:
        return;
    }

    int cx, cy;
    arrowOrigin(r, seg, count, &cx, &cy);

    // Crisp pixel slices: AA softens the thin lines and shifts the perceived
    // centre of the triangle on odd-sized scroll-bar buttons.
    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    if (etch) {
        p->setPen(QPen(*etch, 1));
        for (int i = 0; i < count; ++i)
            p->drawLine(cx + seg[i].x1 + 1, cy + seg[i].y1 + 1,
                        cx + seg[i].x2 + 1, cy + seg[i].y2 + 1);
    }
    p->setPen(QPen(color, 1));
    for (int i = 0; i < count; ++i)
        p->drawLine(cx + seg[i].x1, cy + seg[i].y1,
                    cx + seg[i].x2, cy + seg[i].y2);
    p->restore();
}

void KeramikStyle::drawComboArrow(QPainter *p, const QRect &r,
                                  const QColor &color) const
{
    const int count = int(sizeof keramikComboArrow / sizeof *keramikComboArrow);
    int cx, cy;
    arrowOrigin(r, keramikComboArrow, count, &cx, &cy);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    p->setPen(QPen(color, 1));
    for (int i = 0; i < count; ++i)
        p->drawLine(cx + keramikComboArrow[i].x1, cy + keramikComboArrow[i].y1,
                    cx + keramikComboArrow[i].x2, cy + keramikComboArrow[i].y2);
    p->restore();
}

void KeramikStyle::drawCheckMark(QPainter *p, const QRect &r, const QColor &color) const
{
    // Single source for the tick geometry; bbox centering keeps the longer
    // upper arm from parking the glyph toward the top-left of r.
    static const QPointF pts[] = {
        QPointF(-3.0,  0.0),
        QPointF(-1.0,  2.5),
        QPointF( 3.5, -2.5),
    };
    qreal minX = pts[0].x(), maxX = pts[0].x();
    qreal minY = pts[0].y(), maxY = pts[0].y();
    for (const QPointF &pt : pts) {
        minX = qMin(minX, pt.x());
        maxX = qMax(maxX, pt.x());
        minY = qMin(minY, pt.y());
        maxY = qMax(maxY, pt.y());
    }
    const qreal ox = r.left() + (r.width() - (maxX - minX)) / 2.0 - minX;
    const qreal oy = r.top() + (r.height() - (maxY - minY)) / 2.0 - minY;

    QPainterPath path;
    path.moveTo(ox + pts[0].x(), oy + pts[0].y());
    for (int i = 1; i < int(sizeof pts / sizeof *pts); ++i)
        path.lineTo(ox + pts[i].x(), oy + pts[i].y());

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->setBrush(Qt::NoBrush);
    p->drawPath(path);
    p->restore();
}

// The signature Keramik "ripple": three slightly wavy vertical strokes next
// to the combo-box arrow.  Drawn etched -- a light pass one pixel down/right,
// then a dark main pass on top -- so it reads as pressed-in grooves instead
// of the high-contrast black stripes of the original version.
void KeramikStyle::drawRipple(QPainter *p, const QStyleOption *opt, const QRect &r) const
{
    if (r.width() < 6 || r.height() < 8)
        return;

    const KeramikColors *c = colors(opt->palette);
    const QColor dark = (opt->state & State_Enabled)
            ? c->innerBottom : c->border;
    const QColor light = opt->palette.color(QPalette::Light);

    const auto strokes = [p, &r](qreal dx, qreal dy) {
        const qreal top = r.top() + 1 + dy;
        const qreal bottom = r.bottom() - 1 + dy;
        const qreal span = bottom - top;
        const qreal cx = r.center().x() + dx;
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
    };

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(light, 1));
    strokes(1, 1);
    p->setPen(QPen(dark, 1));
    strokes(0, 0);
    p->restore();
}

// Button bevel: vertical gradient fill, thin frame, bright top rim.
// Free-standing buttons keep rounded corners; flush inside-controls pass
// rounded=false for a square panel that butts against its neighbour.
void KeramikStyle::drawButtonPanel(QPainter *p, const QStyleOption *opt,
                                   bool rounded) const
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
    QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
    grad.setColorAt(0.0, top);
    grad.setColorAt(1.0, bottom);
    const QPen borderPen(disabled ? c->border.lighter(125)
                                  : isDefault ? c->border.darker(112) : c->border, 0);

    if (rounded) {
        // Half-pixel face + cosmetic pen (Plastique recipe) keeps AA arcs
        // off the extreme corner pixels.
        const QRectF face = QRectF(r).adjusted(0.5, 0.5, -0.5, -0.5);
        p->setRenderHint(QPainter::Antialiasing, true);
        p->setPen(Qt::NoPen);
        p->setBrush(grad);
        p->drawRoundedRect(face, 3.0, 3.0);
        p->setBrush(Qt::NoBrush);
        p->setPen(borderPen);
        p->drawRoundedRect(face, 3.0, 3.0);
    } else {
        // Fill the full rect, stroke the inner QRect edges.  Stroking
        // QRectF(r) would put the right/bottom on the next pixel and the
        // widget clips them — the face then looks shifted one pixel left.
        p->setRenderHint(QPainter::Antialiasing, false);
        p->setPen(Qt::NoPen);
        p->setBrush(grad);
        p->drawRect(r);
        p->setBrush(Qt::NoBrush);
        p->setPen(borderPen);
        p->drawRect(r.adjusted(0, 0, -1, -1));
    }

    if (!disabled) {
        // Sheen is 1px orthogonal lines — AA would blur them and can leave
        // dark crumbs next to the rounded rim.
        p->setRenderHint(QPainter::Antialiasing, false);
        p->setPen(c->innerTop);
        p->drawLine(r.left() + 3, r.top() + 1, r.right() - 3, r.top() + 1);
        // Free-standing (rounded) buttons keep the classic left bevel
        // highlight.  Flush square panels (scroll-bar / spin-box) skip it:
        // a bright column on the left only makes the face look shifted left
        // against a flat track.
        if (rounded) {
            p->setPen(sunken ? c->innerBottom : c->innerTop);
            p->drawLine(r.left() + 1, r.top() + 3, r.left() + 1, r.bottom() - 3);
        }
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
// These are all content/inside controls, so the panel is square like the
// original Keramik (only free-standing buttons keep rounded corners).
// inset=true (default) pulls the face 1px in so it sits inside a recessed
// groove; scroll-bar thumbs pass false to sit flush on the flat track.
void KeramikStyle::drawHighlightPanel(QPainter *p, const QStyleOption *opt,
                                      const QRect &r, bool inset) const
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
    p->setRenderHint(QPainter::Antialiasing, false);
    const QRect box = inset ? r.adjusted(1, 1, -1, -1) : r;

    QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
    grad.setColorAt(0.0, top);
    grad.setColorAt(1.0, bottom);
    p->setPen(Qt::NoPen);
    p->setBrush(grad);
    p->drawRect(box);

    p->setBrush(Qt::NoBrush);
    p->setPen(disabled ? c->highlightBorder.lighter(130) : c->highlightBorder);
    p->drawRect(box.adjusted(0, 0, -1, -1));

    if (!disabled) {
        p->setPen(top.lighter(108));
        p->drawLine(box.left() + 3, box.top() + 1, box.right() - 3, box.top() + 1);
    }
    p->restore();
}

// Track strip: flat fill + thin border.  Sliders / progress bars pass
// recessed=true for the L-shaped inner shadow; scroll-bar tracks pass false
// so the strip stays flat (still painted — not Window-coloured invisible).
// Square: tracks sit flush against neighbours, so no rounded edge.
void KeramikStyle::drawGroove(QPainter *p, const QStyleOption *opt, const QRect &r,
                              bool recessed) const
{
    const KeramikColors *c = colors(opt->palette);
    if (r.width() <= 0 || r.height() <= 0)
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);

    p->setPen(Qt::NoPen);
    p->setBrush(c->wellBase.darker(106));
    p->drawRect(r);

    p->setBrush(Qt::NoBrush);
    p->setPen(c->border.lighter(118));
    p->drawRect(r.adjusted(0, 0, -1, -1));

    if (recessed) {
        // Recessed L-shaped inner shadow: dark along top and left.
        p->setPen(c->innerBottom);
        p->drawLine(r.left() + 1, r.top() + 1, r.right() - 2, r.top() + 1);
        p->drawLine(r.left() + 1, r.top() + 1, r.left() + 1, r.bottom() - 2);
    }
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

    // Symmetric 1px inset so a 13×13 indicator does not hug the top-left.
    // Square box — Keramik checkboxes are not rounded.
    const QRect box = r.adjusted(1, 1, -1, -1);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    p->setPen(Qt::NoPen);
    p->setBrush(base);
    p->drawRect(box);

    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(disabled ? c->border.lighter(120) : c->border, 0));
    p->drawRect(box.adjusted(0, 0, -1, -1));

    // Sunken bevel inside the box.
    p->setPen(c->innerBottom);
    p->drawLine(box.left() + 1, box.top() + 1, box.right() - 1, box.top() + 1);
    p->drawLine(box.left() + 1, box.top() + 1, box.left() + 1, box.bottom() - 1);
    p->setPen(opt->palette.color(QPalette::Light));
    p->drawLine(box.left() + 1, box.bottom() - 1, box.right() - 1, box.bottom() - 1);
    p->drawLine(box.right() - 1, box.top() + 1, box.right() - 1, box.bottom() - 1);

    if (on || tri) {
        QColor mark = disabled ? c->border
                               : opt->palette.color(QPalette::ButtonText);
        if (tri) {
            p->setPen(QPen(mark, 1.6));
            p->drawLine(box.left() + 2, box.center().y(), box.right() - 2, box.center().y());
        } else {
            drawCheckMark(p, box, mark);
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
    p->setPen(QPen(disabled ? c->border.lighter(120) : c->border, 0));
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
    drawButtonPanel(p, &btn, false);

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
// Modern-control drawing helpers
// ---------------------------------------------------------------------------

// QToolButton: pressed/checked/hover bevel, focus frame, and the drop-down
// menu indicator (MenuButtonPopup gets a separated arrow half, InstantPopup a
// small corner arrow).  The layout mirrors QCommonStyle's CC_ToolButton so the
// auto-raise logic keeps plain toolbar icons flat until hovered.
void KeramikStyle::drawToolButton(QPainter *p, const QStyleOption *opt,
                                  const QWidget *widget) const
{
    const QStyleOptionToolButton *toolBtn = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
    if (!toolBtn)
        return;

    const KeramikColors *c = colors(opt->palette);

    const QRect button = subControlRect(CC_ToolButton, toolBtn, SC_ToolButton, widget);
    const QRect menuarea = subControlRect(CC_ToolButton, toolBtn, SC_ToolButtonMenu, widget);

    State bflags = toolBtn->state & ~State_Sunken;
    if (bflags & State_AutoRaise) {
        if (!(bflags & State_MouseOver) || !(bflags & State_Enabled))
            bflags &= ~State_Raised;
    }
    State mflags = bflags;
    if (toolBtn->state & State_Sunken) {
        if (toolBtn->activeSubControls & SC_ToolButton)
            bflags |= State_Sunken;
        mflags |= State_Sunken;
    }

    if (toolBtn->subControls & SC_ToolButton) {
        if (bflags & (State_Sunken | State_On | State_Raised)) {
            QStyleOption tool(*opt);
            tool.rect = button;
            tool.state = bflags;
            drawButtonPanel(p, &tool);
        }
    }

    if (toolBtn->state & State_HasFocus) {
        QStyleOptionFocusRect fr;
        fr.QStyleOption::operator=(*toolBtn);
        fr.rect.adjust(3, 3, -3, -3);
        if (toolBtn->features & QStyleOptionToolButton::MenuButtonPopup)
            fr.rect.adjust(0, 0, -pixelMetric(PM_MenuButtonIndicator, opt, widget), 0);
        drawPrimitive(PE_FrameFocusRect, &fr, p, widget);
    }

    QStyleOptionToolButton label(*toolBtn);
    label.state = bflags;
    const int fw = pixelMetric(PM_DefaultFrameWidth, opt, widget);
    label.rect = button.adjusted(fw, fw, -fw, -fw);
    QProxyStyle::drawControl(CE_ToolButtonLabel, &label, p, widget);

    if (toolBtn->subControls & SC_ToolButtonMenu) {
        // The separated arrow half of a MenuButtonPopup button: a pressed
        // ceramic panel while the arrow is active, hover feedback otherwise,
        // plus a thin divider from the button face when idle.
        QStyleOption menu(*opt);
        menu.rect = menuarea;
        menu.state = mflags;
        if (mflags & (State_Sunken | State_On | State_Raised))
            drawButtonPanel(p, &menu);
        else {
            p->save();
            p->setPen(c->border.lighter(150));
            const int x = toolBtn->direction == Qt::RightToLeft
                    ? menuarea.right() : menuarea.left();
            p->drawLine(x, menuarea.top() + 2, x, menuarea.bottom() - 2);
            p->restore();
        }
        drawArrow(p, PE_IndicatorArrowDown, menuarea.adjusted(0, 2, 0, -2),
                  (mflags & State_Enabled)
                          ? opt->palette.color(QPalette::ButtonText) : c->border);
    } else if (toolBtn->features & QStyleOptionToolButton::HasMenu) {
        // InstantPopup: a small arrow tucked into the lower corner.
        const int mbi = pixelMetric(PM_MenuButtonIndicator, opt, widget);
        const int aw = mbi - 4;
        QRect ar(button.right() + 4 - mbi, button.bottom() - aw - 1,
                 aw, aw);
        ar = visualRect(toolBtn->direction, button, ar);
        drawArrow(p, PE_IndicatorArrowDown, ar,
                  (bflags & State_Enabled)
                          ? opt->palette.color(QPalette::ButtonText) : c->border);
    }
}

// QGroupBox: a thin two-pixel ceramic frame interrupted by the title, with a
// proper label and a checkable-group indicator.
void KeramikStyle::drawGroupBox(QPainter *p, const QStyleOption *opt,
                                const QWidget *widget) const
{
    const QStyleOptionGroupBox *gb = qstyleoption_cast<const QStyleOptionGroupBox *>(opt);
    if (!gb)
        return;

    QRect textRect = subControlRect(CC_GroupBox, gb, SC_GroupBoxLabel, widget);
    QRect checkRect = subControlRect(CC_GroupBox, gb, SC_GroupBoxCheckBox, widget);
    const bool flat = gb->features & QStyleOptionFrame::Flat;

    if (gb->subControls & SC_GroupBoxFrame && !flat) {
        QStyleOptionFrame frame;
        frame.QStyleOption::operator=(*gb);
        frame.rect = subControlRect(CC_GroupBox, gb, SC_GroupBoxFrame, widget);
        // Clip the top edge where the title sits so the frame line is
        // interrupted (the same trick as QCommonStyle).
        p->save();
        QRegion region(gb->rect);
        const bool ltr = gb->direction == Qt::LeftToRight;
        region -= checkRect.united(textRect).adjusted(ltr ? -4 : 0, 0, ltr ? 0 : 4, 0);
        if (!gb->text.isEmpty() || gb->subControls & SC_GroupBoxCheckBox)
            p->setClipRegion(region);
        drawPrimitive(PE_FrameGroupBox, &frame, p, widget);
        p->restore();
    }

    if (gb->subControls & SC_GroupBoxLabel && !gb->text.isEmpty()) {
        p->save();
        const QColor textColor = gb->textColor.isValid()
                ? gb->textColor : gb->palette.color(QPalette::WindowText);
        p->setPen(textColor);
        int alignment = int(gb->textAlignment);
        if (!styleHint(SH_UnderlineShortcut, opt, widget))
            alignment |= Qt::TextHideMnemonic;
        if (flat) {
            QFont font = p->font();
            font.setBold(true);
            p->setFont(font);
            if (gb->subControls & SC_GroupBoxCheckBox)
                textRect.adjust(checkRect.right() + 4, 0, checkRect.right() + 4, 0);
        }
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

// QToolBox tab: a full-width ceramic button; the current tab is sunken.
// Square-edged so it fuses with the page below and the neighbouring tabs.
void KeramikStyle::drawToolBoxTab(QPainter *p, const QStyleOption *opt) const
{
    const QRect r = opt->rect;
    QStyleOption btn(*opt);
    if (opt->state & (State_Sunken | State_On | State_Selected))
        btn.state |= State_Sunken;
    drawButtonPanel(p, &btn, false);
    p->save();
    p->setPen(colors(opt->palette)->border.lighter(150));
    p->drawLine(r.left(), r.bottom() - 1, r.right(), r.bottom() - 1);
    p->restore();
}

// QDockWidget title: a ceramic bevel strip with the (possibly rotated) title.
void KeramikStyle::drawDockWidgetTitle(QPainter *p, const QStyleOption *opt) const
{
    const QStyleOptionDockWidget *dw = qstyleoption_cast<const QStyleOptionDockWidget *>(opt);
    if (!dw)
        return;

    const KeramikColors *c = colors(opt->palette);
    const QRect r = opt->rect;

    QLinearGradient grad(r.left(), r.top(), r.left(), r.bottom());
    grad.setColorAt(0.0, c->gradientTop);
    grad.setColorAt(1.0, c->gradientBottom);
    p->save();
    p->setPen(Qt::NoPen);
    p->setBrush(grad);
    p->drawRect(r);
    p->setPen(c->border);
    p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
    p->restore();

    QRect rect = r.adjusted(6, 0, -4, 0);
    if (dw->closable)
        rect.adjust(0, 0, -16, 0);
    if (dw->floatable)
        rect.adjust(0, 0, -16, 0);

    const bool vertical =
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            dw->verticalTitleBar;
#else
            false;   // Qt 5 dock titles are always horizontal
#endif
    const int textLength = vertical ? r.height() : rect.width();
    const QString text = p->fontMetrics().elidedText(dw->title, Qt::ElideRight,
                                                     qMax(textLength, 1));
    if (vertical) {
        p->save();
        p->translate(r.center());
        p->rotate(-90);
        QRect vr(-r.height() / 2, -r.width() / 2, r.height(), r.width());
        drawItemText(p, vr, Qt::AlignCenter, opt->palette,
                     opt->state & State_Enabled, text, QPalette::WindowText);
        p->restore();
    } else {
        drawItemText(p, rect, Qt::AlignLeft | Qt::AlignVCenter, opt->palette,
                     opt->state & State_Enabled, text, QPalette::WindowText);
    }
}

// QToolTip: the classic light-yellow tip with a thin 3-D bevel.
void KeramikStyle::drawToolTip(QPainter *p, const QStyleOption *opt) const
{
    const QRect r = opt->rect;
    p->save();
    p->setPen(Qt::NoPen);
    p->setBrush(opt->palette.color(QPalette::ToolTipBase));
    p->drawRect(r);
    p->setPen(opt->palette.color(QPalette::Light));
    p->drawLine(r.left(), r.top(), r.right() - 1, r.top());
    p->drawLine(r.left(), r.top(), r.left(), r.bottom() - 1);
    p->setPen(opt->palette.color(QPalette::Mid));
    p->drawLine(r.left(), r.bottom() - 1, r.right() - 1, r.bottom() - 1);
    p->drawLine(r.right() - 1, r.top(), r.right() - 1, r.bottom() - 1);
    p->restore();
}

// CE_SizeGrip: diagonal light/dark dots in the status-bar corner.
void KeramikStyle::drawSizeGrip(QPainter *p, const QStyleOption *opt) const
{
    const QRect r = opt->rect;
    const QPalette &pal = opt->palette;
    const QColor dark = pal.color(QPalette::Mid);
    const QColor light = pal.color(QPalette::Light);

    p->save();
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j <= i; ++j) {
            const int x = r.right() - 3 - j * 4;
            const int y = r.bottom() - 3 - i * 4;
            p->setPen(dark);
            p->drawPoint(x, y);
            p->setPen(light);
            p->drawPoint(x + 1, y + 1);
        }
    }
    p->restore();
}

// PE_IndicatorBranch: ceramic expander box + guide lines for tree views.
void KeramikStyle::drawBranch(QPainter *p, const QStyleOption *opt) const
{
    const QRect r = opt->rect;
    const QPalette &pal = opt->palette;
    const int cx = r.center().x();
    const int cy = r.center().y();

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);

    int spacer = 0;
    if (opt->state & State_Children) {
        // A small raised box holding a filled triangle arrow.
        const QRect box(cx - 4, cy - 4, 9, 9);
        p->setPen(pal.color(QPalette::Mid));
        p->setBrush(pal.color(QPalette::Button));
        p->drawRect(box);
        p->setPen(pal.color(QPalette::Light));
        p->drawLine(box.left() + 1, box.top() + 1, box.right() - 1, box.top() + 1);
        p->drawLine(box.left() + 1, box.top() + 1, box.left() + 1, box.bottom() - 1);
        p->setPen(Qt::NoPen);
        p->setBrush(pal.color(QPalette::ButtonText));
        if (opt->state & State_Open) {
            p->drawPolygon(QPolygon() << QPoint(cx - 2, cy - 1)
                                      << QPoint(cx + 2, cy - 1)
                                      << QPoint(cx, cy + 2));
        } else if (opt->direction == Qt::RightToLeft) {
            // Collapsed branch arrow points left in RTL, right in LTR,
            // matching QCommonStyle (the original had the two swapped).
            p->drawPolygon(QPolygon() << QPoint(cx + 1, cy - 2)
                                      << QPoint(cx + 1, cy + 2)
                                      << QPoint(cx - 2, cy));
        } else {
            p->drawPolygon(QPolygon() << QPoint(cx - 1, cy - 2)
                                      << QPoint(cx - 1, cy + 2)
                                      << QPoint(cx + 2, cy));
        }
        spacer = 6;
    }

    // Guide lines to the row, siblings and ancestors.
    p->setPen(pal.color(QPalette::Mid));
    if (opt->state & State_Item) {
        if (opt->direction == Qt::RightToLeft)
            p->drawLine(r.left(), cy, cx - spacer, cy);
        else
            p->drawLine(cx + spacer, cy, r.right(), cy);
    }
    if (opt->state & State_Sibling)
        p->drawLine(cx, cy + spacer, cx, r.bottom());
    if (opt->state & (State_Item | State_Sibling))
        p->drawLine(cx, cy - spacer, cx, r.top());

    p->restore();
}

// CE_MenuScroller: the up/down arrows shown on over-long menus.
void KeramikStyle::drawMenuScroller(QPainter *p, const QStyleOption *opt) const
{
    const QRect r = opt->rect;
    p->save();
    p->fillRect(r, menuSurfaceColor(opt->palette));
    const bool up = opt->state & State_UpArrow;
    const QColor arrow = opt->palette.color(QPalette::ButtonText);
    drawArrow(p, up ? PE_IndicatorArrowUp : PE_IndicatorArrowDown, r, arrow);
    p->restore();
}

// CE_ProgressBarContents: the ceramic highlight block (solid for determinate
// bars, a travelling block for busy ones).  The busy offset comes from
// QProgressBar's own animated value, advanced by the style's busy timer.
void KeramikStyle::drawProgressContents(QPainter *p, const QStyleOption *opt) const
{
    const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
    if (!pb)
        return;

    // QCommonStyle's CE_ProgressBar already routed us through
    // subElementRect(SE_ProgressBarContents), so opt->rect is the contents
    // rect.  Calling subElementRect again here would inset it a second time.
    const QRect cr = opt->rect;
    if (!cr.isValid())
        return;

    // Qt6's QStyleOptionProgressBar has no orientation member (Qt5 keeps the
    // aspect ratio implied by the widget), so infer it from the rect.
    const bool vertical = pb->rect.height() > pb->rect.width();
    const bool reverse = vertical ? pb->invertedAppearance
                                  : ((pb->direction == Qt::RightToLeft)
                                     != pb->invertedAppearance);
    const int total = qMax(pb->maximum - pb->minimum, 1);
    const int progress = qMax(pb->progress, pb->minimum);
    // A 0..0 range is the busy indicator; other min==max ranges are static
    // (QCommonStyle and the animation timer both use this definition).
    const bool busy = (pb->minimum == 0 && pb->maximum == 0);

    p->save();
    p->setClipRect(cr);

    if (busy) {
        // A single ceramic block travelling back and forth along the groove.
        const int length = vertical ? cr.height() : cr.width();
        const int chunk = qBound(10, length / 6, 30);
        const int span = qMax(length - chunk, 1);
        int pos = progress % (span * 2);
        if (pos > span)
            pos = span * 2 - pos;
        QRect chunkRect;
        if (vertical)
            chunkRect = QRect(cr.left(), cr.top() + pos, cr.width(), chunk);
        else
            chunkRect = QRect(cr.left() + pos, cr.top(), chunk, cr.height());
        drawHighlightPanel(p, opt, chunkRect);
    } else if (progress > 0) {
        QRect block;
        if (vertical) {
            const int h = cr.height() * progress / total;
            block = reverse ? QRect(cr.left(), cr.top(), cr.width(), h)
                            : QRect(cr.left(), cr.bottom() - h, cr.width(), h);
        } else {
            const int w = cr.width() * progress / total;
            block = reverse ? QRect(cr.right() - w, cr.top(), w, cr.height())
                            : QRect(cr.left(), cr.top(), w, cr.height());
        }
        // inset=true (default) pulls the block inside the groove's L-shadow.
        drawHighlightPanel(p, opt, block);
    }

    p->restore();
}

// CE_ProgressBarLabel: single-pass black text centred in the bar.  Unlike the
// two-pass highlighted/normal split, the label keeps one colour so it reads as
// plain text over the groove and the ceramic block alike (the original Keramik
// drew its progress text in the normal text colour).  Vertical bars rotate the
// painter so the text reads along the bar.
void KeramikStyle::drawProgressLabel(QPainter *p, const QStyleOptionProgressBar *pb) const
{
    if (pb->text.isEmpty())
        return;

    const QRect r = pb->rect;
    const bool vertical = r.height() > r.width();
    const int flags = Qt::AlignCenter | Qt::TextSingleLine;
    const bool enabled = pb->state & State_Enabled;

    p->save();
    if (vertical) {
        // Rotate the coordinate system so the bar's long axis is horizontal.
        const QPoint c = r.center();
        p->translate(c);
        p->rotate(pb->bottomToTop ? -90 : 90);
        p->translate(-c);
    }
    drawItemText(p, r, flags, pb->palette, enabled, pb->text, QPalette::Text);
    p->restore();
}

// ---------------------------------------------------------------------------
// Busy progress bar animation
// ---------------------------------------------------------------------------

void KeramikStyle::addProgressBar(QProgressBar *bar)
{
    if (!m_busyBars.contains(bar)) {
        m_busyBars.append(bar);
        if (!m_busyTimer->isActive())
            m_busyTimer->start(30);
    }
}

void KeramikStyle::removeProgressBar(QProgressBar *bar)
{
    m_busyBars.removeAll(bar);
    if (m_busyBars.isEmpty())
        m_busyTimer->stop();
}

// Advance every visible busy bar; setValue() repaints the widget itself, so no
// explicit update() is needed.  CE_ProgressBarContents maps the value onto the
// travelling chunk position.
void KeramikStyle::animateProgressBars()
{
    for (QProgressBar *bar : m_busyBars) {
        if (bar->isVisible() && bar->minimum() == 0 && bar->maximum() == 0)
            bar->setValue(bar->value() + 2);
    }
}

bool KeramikStyle::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::StyleChange:
    case QEvent::Paint:
        if (QProgressBar *bar = qobject_cast<QProgressBar *>(obj)) {
            if (bar->isVisible() && bar->minimum() == 0 && bar->maximum() == 0)
                addProgressBar(bar);
            else
                removeProgressBar(bar);
        }
        break;
    case QEvent::Hide:
    case QEvent::Destroy:
        // Only progress bars get our filter installed, so the cast is safe
        // even while the object is being destroyed.
        removeProgressBar(static_cast<QProgressBar *>(obj));
        break;
    default:
        break;
    }
    return QProxyStyle::eventFilter(obj, event);
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

    if (qobject_cast<QProgressBar *>(widget))
        widget->installEventFilter(this);

    // The toolbox tabs are internal QAbstractButton subclasses without
    // auto-raise, so enable hover on them explicitly.
    if (qobject_cast<QToolBox *>(widget)) {
        widget->setAttribute(Qt::WA_Hover);
        const auto children = widget->findChildren<QWidget *>();
        for (QWidget *child : children)
            child->setAttribute(Qt::WA_Hover);
    }
}

void KeramikStyle::polish(QPalette &palette)
{
    if (m_forceClassicPalette)
        palette = standardPalette();
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
    if (qobject_cast<QProgressBar *>(widget)) {
        widget->removeEventFilter(this);
        removeProgressBar(static_cast<QProgressBar *>(widget));
    }
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
        // Rounded dotted frame in the palette text colour, matching the
        // original's drawWinFocusRect (a dotted black rectangle) but rounded
        // so the dots stay on the control face instead of spilling past the
        // corner arcs onto the background.
        p->save();
        p->setPen(QPen(opt->palette.color(QPalette::Text), 0, Qt::DotLine));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(QRectF(r.adjusted(1, 1, -1, -1)), 3.0, 3.0);
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
        box.rect = r;
        drawCheckBoxIndicator(p, &box, opt->state & State_On,
                              opt->state & State_NoChange);
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

    case PE_IndicatorHeaderArrow: {
        // Qt 6 CE_Header no longer sets State_UpArrow; read sortIndicator.
        // Match QCommonStyle: SortUp → tip down, SortDown → tip up.
        const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt);
        if (!header || header->sortIndicator == QStyleOptionHeader::None)
            break;
        const bool tipDown = header->sortIndicator == QStyleOptionHeader::SortUp;
        drawArrow(p, tipDown ? PE_IndicatorArrowDown : PE_IndicatorArrowUp,
                  r, c->border);
        break;
    }

    // Scroll-bar --------------------------------------------------------------
    case PE_IndicatorProgressChunk:
        drawHighlightPanel(p, opt, r);
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
            drawHighlightPanel(p, &sel, r);
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

    // Group boxes ------------------------------------------------------------
    case PE_FrameGroupBox: {
        // A raised ceramic frame: outer light rim, inner highlight on the
        // top/left and a mid tone on the bottom/right.
        p->save();
        p->setBrush(Qt::NoBrush);
        p->setPen(c->border.lighter(150));
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->setPen(opt->palette.color(QPalette::Light));
        p->drawLine(r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1);
        p->drawLine(r.left() + 1, r.top() + 1, r.left() + 1, r.bottom() - 1);
        p->setPen(opt->palette.color(QPalette::Mid));
        p->drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
        p->drawLine(r.right() - 1, r.top() + 1, r.right() - 1, r.bottom() - 1);
        p->restore();
        break;
    }

    // Tool tips ---------------------------------------------------------------
    case PE_PanelTipLabel:
        drawToolTip(p, opt);
        break;

    // Tree branches -----------------------------------------------------------
    case PE_IndicatorBranch:
        drawBranch(p, opt);
        break;

    // Dock widgets -------------------------------------------------------------
    case PE_FrameDockWidget: {
        // Thin ceramic frame around each dock widget in the main window.
        p->save();
        p->setPen(c->border.lighter(140));
        p->setBrush(Qt::NoBrush);
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->restore();
        break;
    }

    case PE_IndicatorDockWidgetResizeHandle: {
        // Three subtle dots marking the resize grip of a floating dock.
        p->save();
        p->setPen(opt->palette.color(QPalette::Mid));
        const int n = 3;
        for (int i = 0; i < n; ++i) {
            const int off = (i - (n - 1) / 2) * 4;
            if (r.width() >= r.height())
                p->drawPoint(r.center().x() + off, r.center().y());
            else
                p->drawPoint(r.center().x(), r.center().y() + off);
        }
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

    // Rounded tab: half-pixel face so the AA stroke stays off the extreme
    // corner pixels (same sparkle fix as drawButtonPanel).  The active tab's
    // page side is covered by the pane, so stroking the full rounded border
    // there is invisible.
    const QRectF rf = QRectF(tr).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = 3.0;

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
    p->drawRoundedRect(rf, radius, radius);

    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(c->border.lighter(140), 0));
    p->drawRoundedRect(rf, radius, radius);

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
    const bool reverse = mi->direction == Qt::RightToLeft;

    // Row background ------------------------------------------------------
    // Filled first for every row, separators included, exactly as the
    // original's CE_PopupMenuItem (fillRect background.light(105), then the
    // separator lines) -- so the separator area never relies on Qt having
    // painted PE_PanelMenu underneath it.
    if (selected) {
        QStyleOption sel(*mi);
        sel.state |= State_Selected;
        drawHighlightPanel(p, &sel, r);
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
    // Mirrored for RTL menus so the check column sits on the far edge, clear
    // of the mirrored shortcut column and label.
    const QRect checkRect = visualRect(mi->direction, r,
            QRect(r.left() + itemFrame, r.top() + itemFrame,
                  qMax(checkcol, 1) - 1, r.height() - 2 * itemFrame));
    if (mi->checked) {
        // Sunken shade panel behind the check column (the original's
        // qDrawShadePanel with a Midlight fill), used both with an icon
        // (panel under the icon) and without one (panel under the check).
        const QRect checkFace = checkRect.adjusted(1, 1, -1, -1);
        drawMenuCheckPanel(p, checkFace, mi->palette);
        if (mi->icon.isNull()) {
            // Shade panel is Midlight; PE_IndicatorMenuCheckMark always paints
            // ButtonText (not HighlightedText) so the tick stays visible when
            // the row behind is selected.
            QStyleOption mark(*mi);
            mark.rect = checkFace;
            drawPrimitive(PE_IndicatorMenuCheckMark, &mark, p, nullptr);
        }
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
        // Right gutter must match what CT_MenuItem adds (rightBorder, plus an
        // arrow gutter for sub-menus).  The previous formula also subtracted
        // arrowHMargin + 3*itemHMargin + itemFrame on every row (~25px that
        // sizeFromContents never provided), so long labels without a
        // shortcut were clipped on the right.
        int rightPad = rightBorder;
        if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
            rightPad += arrowHMargin;
        const int tw = qMax(1, r.width() - xm - shortcutW - rightPad + 1);
        // Label starts after the check/icon column on the left, or after the
        // accelerator column on the right when the menu is reversed.
        const QRect textRect(reverse
                ? r.left() + shortcutW + rightBorder + itemHMargin + itemFrame - 1
                : r.left() + xm,
                r.top(), tw, r.height());

        if (!shortcut.isEmpty()) {
            // The accelerator column sits at the far edge of the menu (right
            // for LTR, left for RTL).
            const QRect shortcutRect(reverse
                    ? r.left() + rightBorder + itemHMargin + itemFrame - 1
                    : r.right() - shortcutW - rightBorder
                            - itemHMargin - itemFrame + 1,
                    r.top(), shortcutW, r.height());
            drawItemText(p, shortcutRect, Qt::AlignVCenter
                                          | (reverse ? Qt::AlignLeft : Qt::AlignRight)
                                          | Qt::TextSingleLine,
                         mi->palette, enabled, shortcut,
                         selected ? QPalette::HighlightedText : QPalette::Text);
        }

        drawItemText(p, textRect, Qt::AlignVCenter
                                          | (reverse ? Qt::AlignRight : Qt::AlignLeft)
                                          | Qt::TextShowMnemonic | Qt::TextSingleLine,
                     mi->palette, enabled, text,
                     selected ? QPalette::HighlightedText : QPalette::Text);

        p->setFont(savedFont);
    }

    // Sub-menu arrow -----------------------------------------------------------
    if (mi->menuItemType == QStyleOptionMenuItem::SubMenu) {
        const int dim = pixelMetric(PM_MenuButtonIndicator, mi, nullptr);
        const QRect ar(reverse
                ? r.left() + arrowHMargin + itemFrame - 1
                : r.right() - arrowHMargin - itemFrame - dim + 1,
                r.top() + (r.height() - dim) / 2, dim, dim);
        drawArrow(p, reverse ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight, ar,
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
    // Push buttons --------------------------------------------------------------
    case CE_PushButton: {
        // Keramik draws its own menu indicator: the base Windows style paints
        // a solid grey triangle, which does not match the thin ceramic arrows
        // used everywhere else in the theme.
        const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
        if (!btn)
            break;
        // Flat buttons keep the base-style semantics: no panel except when
        // pressed/checked.  Drawing a hover panel would be a behaviour
        // change beyond this fix.
        const bool flat = btn->features & QStyleOptionButton::Flat;
        if (!flat || (btn->state & (State_Sunken | State_On)))
            drawButtonPanel(p, opt);
        if (btn->state & State_HasFocus) {
            QStyleOptionFocusRect fr;
            fr.QStyleOption::operator=(*btn);
            fr.rect = subElementRect(SE_PushButtonFocusRect, btn, widget);
            drawPrimitive(PE_FrameFocusRect, &fr, p, widget);
        }
        QStyleOptionButton labelOpt(*btn);
        if (btn->features & QStyleOptionButton::HasMenu) {
            const int mbi = pixelMetric(PM_MenuButtonIndicator, btn, widget);
            // Ceramic chevron bbox is 8px; keep the indicator box ≥ that.
            const int aw = mbi - 4;
            QRect ar(btn->rect.right() - mbi - 1,
                     btn->rect.y() + (btn->rect.height() - aw) / 2,
                     aw, aw);
            ar = visualRect(btn->direction, btn->rect, ar);
            const QColor color = (btn->state & State_Enabled)
                    ? btn->palette.color(QPalette::ButtonText) : c->border;
            drawArrow(p, PE_IndicatorArrowDown, ar, color);

            // The base Windows CE_PushButtonLabel paints its own solid menu
            // triangle when HasMenu is set, so clear the flag and manually
            // shrink the text rect to keep the ceramic arrow as the only
            // indicator without letting narrow labels run into it.
            labelOpt.features &= ~QStyleOptionButton::HasMenu;
            if (btn->direction == Qt::RightToLeft)
                labelOpt.rect.setLeft(btn->rect.left() + mbi);
            else
                labelOpt.rect.setRight(btn->rect.right() - mbi);
        }
        QProxyStyle::drawControl(CE_PushButtonLabel, &labelOpt, p, widget);
        return;
    }

    // Progress bar --------------------------------------------------------------
    case CE_ProgressBar: {
        // Route the three sub-elements through our own subElementRect so the
        // groove/contents/label coordinates stay consistent (the base Windows
        // style insets contents by 3px and misaligns the block with the
        // groove's shadow lines).
        const QStyleOptionProgressBar *pb =
                qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
        if (pb) {
            QStyleOptionProgressBar subopt = *pb;
            subopt.rect = subElementRect(SE_ProgressBarGroove, pb, widget);
            drawControl(CE_ProgressBarGroove, &subopt, p, widget);
            subopt.rect = subElementRect(SE_ProgressBarContents, pb, widget);
            drawControl(CE_ProgressBarContents, &subopt, p, widget);
            if (pb->textVisible) {
                subopt.rect = subElementRect(SE_ProgressBarLabel, pb, widget);
                drawControl(CE_ProgressBarLabel, &subopt, p, widget);
            }
        }
        break;
    }

    case CE_ProgressBarGroove:
        drawGroove(p, opt, r.adjusted(1, 1, -2, -2));
        break;

    case CE_ProgressBarContents:
        drawProgressContents(p, opt);
        break;

    case CE_ProgressBarLabel:
        if (const QStyleOptionProgressBar *pb =
                qstyleoption_cast<const QStyleOptionProgressBar *>(opt))
            drawProgressLabel(p, pb);
        break;

    // Combo box ---------------------------------------------------------------
    case CE_ComboBoxLabel: {
        // QComboBox::initStyleOption marks a focused, non-editable combo
        // State_Selected, so the base QWindowsStyle paints the current text
        // in QPalette::HighlightedText (white) although Keramik draws no
        // highlight background here -- the text became invisible on the grey
        // button face.  The focus is already shown by the focus rect, so
        // drop the flags and keep the normal text colour.
        const QStyleOptionComboBox *cmb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
        if (!cmb)
            break;
        QStyleOptionComboBox clean(*cmb);
        clean.state &= ~(State_HasFocus | State_Selected);
        QProxyStyle::drawControl(CE_ComboBoxLabel, &clean, p, widget);
        break;
    }

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

    // Long menus --------------------------------------------------------------
    case CE_MenuScroller:
        drawMenuScroller(p, opt);
        break;

    // Tool boxes ---------------------------------------------------------------
    case CE_ToolBoxTabShape:
        drawToolBoxTab(p, opt);
        break;

    // Dock widgets -------------------------------------------------------------
    case CE_DockWidgetTitle:
        drawDockWidgetTitle(p, opt);
        break;

    // Status bar ---------------------------------------------------------------
    case CE_SizeGrip:
        drawSizeGrip(p, opt);
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
        // Header sections live inside the view, so they stay square like the
        // other inside controls (scrollbar buttons, spinbox arrows).
        drawButtonPanel(p, opt, false);
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
                drawRipple(p, opt, rippleRect);
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
                drawHighlightPanel(p, &thumb, handle);
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

        // Track: flat strip (no L-shadow).  Still painted — same fill/border
        // as drawGroove, without the recessed bevel.
        if (groove.isValid())
            drawGroove(p, opt, groove, false);

        // Thumb: palette-highlight panel, brighter while hovered/dragged.
        if (slider.isValid() && opt->subControls & SC_ScrollBarSlider) {
            QStyleOption thumb(*opt);
            if (opt->activeSubControls & SC_ScrollBarSlider)
                thumb.state |= State_MouseOver;
            drawHighlightPanel(p, &thumb, slider, false);
        }

        // End buttons. The horizontal add line keeps the original Keramik
        // "double arrow" design: a wide button with a divider whose two
        // halves scroll by page and by line respectively.  Buttons stay
        // square so they butt flush against the track.
        if (subline.isValid() && opt->subControls & SC_ScrollBarSubLine)
            drawScrollBarButton(p, opt, subline,
                                horiz ? PE_IndicatorArrowLeft : PE_IndicatorArrowUp);
        if (addline.isValid() && opt->subControls & SC_ScrollBarAddLine) {
            QStyleOption btn(*opt);
            btn.rect = addline;
            drawButtonPanel(p, &btn, false);
            const QColor arrowColor = (opt->state & (State_Sunken | State_On))
                    ? c->border : opt->palette.color(QPalette::ButtonText);
            const QColor divider = opt->palette.color(QPalette::ButtonText);
            const int mx = addline.center().x();
            const int my = addline.center().y();
            p->save();
            p->setPen(divider);
            if (horiz)
                p->drawLine(mx, my - 3, mx, my + 3);
            else
                p->drawLine(mx - 3, my, mx + 3, my);
            p->restore();
            if (horiz) {
                const QRect left(addline.left(), addline.top(),
                                 addline.width() / 2, addline.height());
                const QRect right(left.right() + 1, addline.top(),
                                  addline.width() - left.width(), addline.height());
                // Inset past the bevel so the glyph centres in the face, not
                // in a rect that still includes the 1px border.
                drawArrow(p, PE_IndicatorArrowLeft, left.adjusted(1, 1, -1, -1), arrowColor);
                drawArrow(p, PE_IndicatorArrowRight, right.adjusted(1, 1, -1, -1), arrowColor);
            } else {
                const QRect top(addline.left(), addline.top(),
                                addline.width(), addline.height() / 2);
                const QRect bottom(top.left(), top.bottom() + 1, addline.width(),
                                   addline.height() - top.height());
                drawArrow(p, PE_IndicatorArrowUp, top.adjusted(1, 1, -1, -1), arrowColor);
                drawArrow(p, PE_IndicatorArrowDown, bottom.adjusted(1, 1, -1, -1), arrowColor);
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
                drawButtonPanel(p, &btn, false);
                drawArrow(p, PE_IndicatorArrowUp, up.adjusted(1, 1, -1, -1),
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
                drawButtonPanel(p, &btn, false);
                drawArrow(p, PE_IndicatorArrowDown, down.adjusted(1, 1, -1, -1),
                          (opt->state & State_Enabled)
                                  ? opt->palette.color(QPalette::ButtonText) : c->border);
            }
        }
        break;
    }

    // Tool buttons ---------------------------------------------------------------
    case CC_ToolButton:
        drawToolButton(p, opt, widget);
        break;

    // Group boxes -------------------------------------------------------------
    case CC_GroupBox:
        drawGroupBox(p, opt, widget);
        break;

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
    // Tool-bar layout -------------------------------------------------------
    case PM_ToolBarFrameWidth:
        return 1;
    case PM_ToolBarItemMargin:
        return qRound(QStyleHelper::dpiScaled(2, opt));
    case PM_ToolBarItemSpacing:
        return qRound(QStyleHelper::dpiScaled(2, opt));
    case PM_ToolBarHandleExtent:
        return qRound(QStyleHelper::dpiScaled(10, opt));
    case PM_ToolBarSeparatorExtent:
        return qRound(QStyleHelper::dpiScaled(8, opt));
    case PM_DockWidgetFrameWidth:
        return 1;
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

QRect KeramikStyle::subElementRect(SubElement sr, const QStyleOption *opt,
                                   const QWidget *widget) const
{
    switch (sr) {
    case SE_ProgressBarGroove:
    case SE_ProgressBarLabel:
        // The base Windows style keys off QStyleOptionProgressBar::bottomToTop
        // for direction, which QProgressBar no longer sets on Qt 6, so it
        // hands back broken rects and reserves space for the label above the
        // bar.  This style draws the whole bar itself, so both span the full
        // option rect and the label stays centred inside the groove.
        return opt->rect;
    case SE_ProgressBarContents:
        return opt->rect.adjusted(1, 1, -1, -1);
    default:
        break;
    }
    return QProxyStyle::subElementRect(sr, opt, widget);
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
        // Keep the drawMenuItem layout and this formula in lock-step: the
        // label sits after xm = itemFrame+checkcol+itemHMargin, and the
        // right side keeps rightBorder (+ arrowHMargin for sub-menus).
        if (const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt)) {
            if (mi->menuItemType == QStyleOptionMenuItem::Separator)
                return QSize(10, 3);
            int w = contentsSize.width();
            // Left gutter matching drawMenuItem's xm without the check column.
            w += itemFrame + itemHMargin;
            if (mi->text.contains(QLatin1Char('\t')))
                w += itemHMargin + itemFrame * 2 + 7;   // gap before the accelerator
            if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
                w += arrowHMargin;                      // matches draw rightPad
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
    case SH_Menu_Scrollable:
        // Over-long menus scroll with the ceramic up/down scroller buttons.
        return 1;
    default:
        break;
    }
    return QProxyStyle::styleHint(sh, opt, widget, shret);
}
