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

#ifndef KERAMIKSTYLE_H
#define KERAMIKSTYLE_H

#include <QtWidgets/qproxystyle.h>
#include <QtCore/qhash.h>

class QPainter;
class QStyleOption;
class QStyleOptionMenuItem;
class QStyleOptionTab;

/*
    Keramik is the ceramic-themed widget style of KDE 3 / TDE. It is
    characterised by smooth bevel gradients, slightly rounded buttons with
    thin frames, square recessed input wells, and accent-coloured (palette
    highlight) slider and scroll-bar handles.

    This implementation is a clean-room rewrite written purely for this
    project. The look is reproduced with QPainter vectors and a small
    palette-derived ramp derived from the original theme's behaviour; it
    shares no code with the KDE/TDE original nor with later community
    ports, and it does not depend on any KDE libraries.

    The style is layered on top of QProxyStyle, so text layout, icons and
    all un-overridden elements are supplied by the base platform style.
*/
class KeramikStyle : public QProxyStyle
{
    Q_OBJECT

public:
    KeramikStyle();
    ~KeramikStyle();

    void polish(QWidget *widget) override;
    void polish(QPalette &palette) override;
    void unpolish(QWidget *widget) override;

    QPalette standardPalette() const override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt,
                       QPainter *p, const QWidget *widget) const override;
    void drawControl(ControlElement ce, const QStyleOption *opt,
                     QPainter *p, const QWidget *widget) const override;
    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt,
                            QPainter *p, const QWidget *widget) const override;

    int pixelMetric(PixelMetric pm, const QStyleOption *opt = nullptr,
                    const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(ContentsType ct, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget = nullptr) const override;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget = nullptr) const override;
    int styleHint(StyleHint sh, const QStyleOption *opt = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *shret = nullptr) const override;

private:
    // Palette-derived colour ramp. gradientTop/gradientBottom are the two
    // ends of the vertical button bevel, wellBase is the white/beige fill
    // of sunken wells, and the highlight* colours drive the accent ramp
    // used for sliders, scroll bars and progress bars.
    struct KeramikColors {
        QColor gradientTop;
        QColor gradientBottom;
        QColor border;
        QColor innerTop;
        QColor innerBottom;
        QColor wellBase;
        QColor highlightTop;
        QColor highlightBottom;
        QColor highlightBorder;
        QColor menuGradTop;
        QColor menuGradBottom;
        QRgb buttonRgb = 0;
        QRgb highlightRgb = 0;
    };

    const KeramikColors *colors(const QPalette &palette) const;

    static QPainterPath roundedRect(const QRect &r, int radius);

    void drawButtonPanel(QPainter *p, const QStyleOption *opt) const;
    void drawWell(QPainter *p, const QStyleOption *opt) const;
    void drawHighlightPanel(QPainter *p, const QStyleOption *opt,
                            const QRect &r, int radius) const;
    void drawGroove(QPainter *p, const QStyleOption *opt, const QRect &r) const;
    void drawCheckBoxIndicator(QPainter *p, const QStyleOption *opt,
                               bool on, bool tri) const;
    void drawRadioIndicator(QPainter *p, const QStyleOption *opt, bool on) const;
    void drawCheckMark(QPainter *p, const QRect &r, const QColor &color) const;
    void drawRipple(QPainter *p, const QRect &r, const QColor &color) const;
    void drawArrow(QPainter *p, PrimitiveElement pe, const QRect &r,
                   const QColor &color, const QColor *etch = nullptr) const;
    void drawComboArrow(QPainter *p, const QRect &r, const QColor &color) const;
    void drawSplitter(QPainter *p, const QStyleOption *opt) const;
    void drawMenuCheckPanel(QPainter *p, const QRect &r, const QPalette &pal) const;
    void drawMenuBarItem(QPainter *p, const QStyleOptionMenuItem *mi) const;
    void drawMenuItem(QPainter *p, const QStyleOptionMenuItem *mi) const;
    void drawTabShape(QPainter *p, const QStyleOptionTab *tab) const;
    void drawScrollBarButton(QPainter *p, const QStyleOption *opt,
                             const QRect &r, PrimitiveElement arrow) const;

    mutable QHash<quint64, KeramikColors> m_colorCache;
};

#endif // KERAMIKSTYLE_H
