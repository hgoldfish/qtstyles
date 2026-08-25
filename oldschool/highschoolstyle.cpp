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

#include "highschoolstyle.h"
#include "qtstyles_palette.h"

#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qdrawutil.h>

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qpalette.h>

// The SGI accent red, used for check marks and radio dots.
static const QColor hsCheckRed(255, 0, 0);
static const QColor hsCheckRedDisabled(230, 120, 120);

/*!
    Constructs a HighschoolStyle.  When \a useHighlightCols is false (the
    default) the application palette is polished so that selected text is
    shown inverted, the Motif way.
*/
HighschoolStyle::HighschoolStyle(bool useHighlightCols)
    : OldschoolStyle(useHighlightCols)
{
}

HighschoolStyle::~HighschoolStyle() = default;

/*!
    \reimp

    Returns the warm beige-grey SGI palette.  The button colour is derived
    by darkening the window colour (the historical theme used that relation),
    and the base colour is the darkened "paper white", so the returned
    palette is self-contained: it no longer depends on a polish() pass to
    look right.

    The Motif inverted selection (Highlight=Text / HighlightedText=Base) is
    intentionally not baked in here -- OldschoolStyle::polish(QPalette&)
    applies it automatically when the style is installed, so standardPalette()
    describes the classic palette itself only.
*/
QPalette HighschoolStyle::standardPalette() const
{
    // 全部角色显式给出，返回的 palette 完全自足：QPalette 的默认构造会
    // 继承当前 application palette，缺省角色会在样式切换时串入上一个
    // 样式的配色。
    QColor window(0xc9, 0xbe, 0xb1);      // warm beige grey
    QColor base(0xf2, 0xec, 0xe4);        // warm paper white
    base = base.darker(130);              // darker base colour in list widgets

    QPalette pal;
    pal.setColor(QPalette::Window, window);
    pal.setColor(QPalette::WindowText, Qt::black);
    pal.setColor(QPalette::Base, base);
    pal.setColor(QPalette::AlternateBase, base.lighter(105));
    pal.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xdc));
    pal.setColor(QPalette::ToolTipText, Qt::black);
    pal.setColor(QPalette::Text, Qt::black);
    pal.setColor(QPalette::Button, window.darker(120));   // button is the darkened window colour
    pal.setColor(QPalette::ButtonText, Qt::black);
    pal.setColor(QPalette::BrightText, Qt::white);
    pal.setColor(QPalette::Light, window.lighter(135));
    pal.setColor(QPalette::Midlight, window.lighter(115));
    pal.setColor(QPalette::Mid, window.darker(125));
    pal.setColor(QPalette::Dark, window.darker(175));
    pal.setColor(QPalette::Shadow, window.darker(225));
    pal.setColor(QPalette::Highlight, QColor(0x00, 0x00, 0x80));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::PlaceholderText, window.darker(175));
    pal.setColor(QPalette::Link, QColor(0x00, 0x00, 0xee));
    pal.setColor(QPalette::LinkVisited, QColor(0x52, 0x18, 0x8b));

    QtStyles::applyClassicDisabled(&pal);
    return pal;
}

// ---------------------------------------------------------------------------
// Helper drawing
// ---------------------------------------------------------------------------

void HighschoolStyle::drawHSCheckMark(QPainter *p, const QRect &r, const QPalette &pal,
                                      bool enabled) const
{
    // The reference check is a small zig-zag stroke; a thick two-segment
    // tick with a Motif drop shadow reads as the same solid red check.
    const QPoint c = r.center();
    const int d = qMax(4, r.width() / 4);
    const QColor red = enabled ? hsCheckRed : hsCheckRedDisabled;

    QPainterPath path;
    path.moveTo(c.x() - d, c.y() - 1);
    path.lineTo(c.x() - 1, c.y() + d);
    path.lineTo(c.x() + d + 1, c.y() - d);

    p->save();
    p->setPen(QPen(pal.shadow().color(), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path.translated(1, 1));
    p->setPen(QPen(red, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->drawPath(path);
    p->restore();
}

void HighschoolStyle::drawHSRadio(QPainter *p, const QStyleOption *opt) const
{
    // Diamond shaped radio indicator with a red dot when checked.
    const QRect r = opt->rect;
    const bool on = opt->state & State_On;
    const bool down = opt->state & State_Sunken;
    const bool showUp = !(down ^ on);
    const bool hot = (opt->state & State_MouseOver) && (opt->state & State_Enabled);

    const QPoint lt(r.left() + 1, r.center().y());
    const QPoint tp(r.center().x(), r.top());
    const QPoint rt(r.right() - 1, r.center().y());
    const QPoint bt(r.center().x(), r.bottom());

    p->save();
    QPolygon diamond;
    diamond << lt << tp << rt << bt;
    p->setPen(Qt::NoPen);
    p->setBrush(opt->palette.brush(hot ? QPalette::Midlight
                                       : (showUp ? QPalette::Button : QPalette::Mid)));
    p->drawPolygon(diamond);
    p->setBrush(Qt::NoBrush);

    if (showUp) {
        p->setPen(opt->palette.dark().color());
        p->drawLine(lt, tp);
        p->drawLine(tp, rt);
        p->setPen(opt->palette.light().color());
        p->drawLine(rt, bt);
        p->drawLine(bt, lt);
        p->drawLine(QPoint(r.left() + 2, r.center().y()), QPoint(r.center().x() - 1, r.top() + 2));
    } else {
        p->setPen(opt->palette.light().color());
        p->drawLine(lt, tp);
        p->drawLine(tp, rt);
        p->setPen(opt->palette.dark().color());
        p->drawLine(rt, bt);
        p->drawLine(bt, lt);
    }

    if (on) {
        const QColor dot = (opt->state & State_Enabled) ? hsCheckRed : hsCheckRedDisabled;
        const int d = qMax(2, r.width() / 5);
        p->setPen(Qt::NoPen);
        p->setBrush(dot);
        p->drawEllipse(r.center(), d, d);
    }
    p->restore();
}

// ---------------------------------------------------------------------------
// drawPrimitive
// ---------------------------------------------------------------------------

void HighschoolStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                                    const QWidget *widget) const
{
    switch (pe) {
    case PE_IndicatorCheckBox:
        // Motif base class sunks the panel on check but draws no tick;
        // layer the SGI red check on top.
        OldschoolStyle::drawPrimitive(pe, opt, p, widget);
        if (opt->state & State_On)
            drawHSCheckMark(p, opt->rect.adjusted(1, 2, 1, 1), opt->palette,
                            opt->state & State_Enabled);
        break;

    case PE_IndicatorRadioButton:
        drawHSRadio(p, opt);
        break;

    default:
        OldschoolStyle::drawPrimitive(pe, opt, p, widget);
        break;
    }
}
