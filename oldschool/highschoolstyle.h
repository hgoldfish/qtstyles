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

#ifndef HIGHSCHOOLSTYLE_H
#define HIGHSCHOOLSTYLE_H

#include "oldschoolstyle.h"

class QPainter;
class QStyleOption;

/*
    HighschoolStyle is OldschoolStyle with the SGI check marks and radio
    dots layered on top.  drawPrimitive() replaces the checkbox, radio
    button and menu check-mark rendering with the SGI look -- red check
    marks, a red radio dot in a diamond indicator -- while every other
    primitive, control and complex control is drawn unchanged by
    OldschoolStyle.  The warm beige-grey SGI palette is offered as the
    style's standardPalette() (a suggestion, never applied automatically);
    metrics and sizeFromContents are inherited unchanged from
    OldschoolStyle.

    This implementation is a clean-room rewrite written purely for this
    project.  It shares no code with the original Qt 3 style by Trolltech.
*/
class HighschoolStyle : public OldschoolStyle
{
    Q_OBJECT

public:
    explicit HighschoolStyle(bool useHighlightCols = false, bool forceClassicPalette = false);
    ~HighschoolStyle() override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p,
                       const QWidget *widget = nullptr) const override;
    QPalette standardPalette() const override;

private:
    void drawHSCheckMark(QPainter *p, const QRect &r, const QPalette &pal, bool enabled) const;
    void drawHSRadio(QPainter *p, const QStyleOption *opt) const;
};

#endif // HIGHSCHOOLSTYLE_H
