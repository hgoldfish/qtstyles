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
#include <QtCore/qlist.h>

class QPainter;
class QProgressBar;
class QStyleOption;
class QStyleOptionMenuItem;
class QStyleOptionProgressBar;
class QStyleOptionTab;
class QTimer;

/*
    Keramik is the ceramic-themed widget style of KDE 3 / TDE. It is
    characterised by smooth bevel gradients, lightly rounded free-standing
    buttons, square flush panels (scroll bars, spins, thumbs), thin frames,
    square recessed input wells, and accent-coloured (palette highlight)
    slider and scroll-bar handles.

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
    // 当 forceClassicPalette 为 true 时（对应 "keramik-classic" 键），样式
    // 安装时通过 polish(QPalette&) 强制套用 standardPalette() 的经典配色。
    explicit KeramikStyle(bool forceClassicPalette = false);
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
    QRect subElementRect(SubElement sr, const QStyleOption *opt,
                         const QWidget *widget = nullptr) const override;

    bool eventFilter(QObject *obj, QEvent *event) override;

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

    // Free-standing buttons keep rounded corners; flush inside-controls
    // (scroll-bar / spin-box faces) pass rounded=false for a square panel.
    void drawButtonPanel(QPainter *p, const QStyleOption *opt,
                         bool rounded = true) const;
    void drawWell(QPainter *p, const QStyleOption *opt) const;
    void drawHighlightPanel(QPainter *p, const QStyleOption *opt,
                            const QRect &r, bool inset = true) const;
    void drawGroove(QPainter *p, const QStyleOption *opt, const QRect &r,
                    bool recessed = true) const;
    void drawCheckBoxIndicator(QPainter *p, const QStyleOption *opt,
                               bool on, bool tri) const;
    void drawRadioIndicator(QPainter *p, const QStyleOption *opt, bool on) const;
    void drawCheckMark(QPainter *p, const QRect &r, const QColor &color) const;
    void drawRipple(QPainter *p, const QStyleOption *opt, const QRect &r) const;
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

    // Modern-control drawing helpers.
    void drawToolButton(QPainter *p, const QStyleOption *opt,
                        const QWidget *widget) const;
    void drawGroupBox(QPainter *p, const QStyleOption *opt,
                      const QWidget *widget) const;
    void drawToolBoxTab(QPainter *p, const QStyleOption *opt) const;
    void drawDockWidgetTitle(QPainter *p, const QStyleOption *opt) const;
    void drawToolTip(QPainter *p, const QStyleOption *opt) const;
    void drawSizeGrip(QPainter *p, const QStyleOption *opt) const;
    void drawBranch(QPainter *p, const QStyleOption *opt) const;
    void drawMenuScroller(QPainter *p, const QStyleOption *opt) const;
    void drawProgressContents(QPainter *p, const QStyleOption *opt) const;
    void drawProgressLabel(QPainter *p, const QStyleOptionProgressBar *pb) const;

    // Busy (indeterminate) QProgressBar animation: one QTimer advances the
    // value of every visible busy bar, exactly like phase/winxp in this repo.
    void addProgressBar(QProgressBar *bar);
    void removeProgressBar(QProgressBar *bar);
    void animateProgressBars();

    mutable QHash<quint64, KeramikColors> m_colorCache;
    bool m_forceClassicPalette;
    QTimer *m_busyTimer = nullptr;
    QList<QProgressBar *> m_busyBars;
};

#endif // KERAMIKSTYLE_H
