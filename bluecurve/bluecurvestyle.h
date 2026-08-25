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

#ifndef BLUECURVESTYLE_H
#define BLUECURVESTYLE_H

#include <QtWidgets/qcommonstyle.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>

class QPainter;
class QProgressBar;
class QStyleOption;

/*
    Bluecurve is the default widget theme of Red Hat 8/9 (2002-2003).
    It is characterised by light beveled buttons with a subtle 1-pixel
    highlight rim, sunken line-edit wells, blue gradient highlights for
    selected items and progress bars, and 13x13 checkbox/radio indicators.

    This implementation is a clean-room rewrite written purely for this
    project. It reproduces the Bluecurve look using QPainter vectors and a
    small palette-derived shade ramp; it shares no code with the original
    Qt 3 style by Red Hat, Inc. nor with later community ports.
*/
class BluecurveStyle : public QCommonStyle
{
    Q_OBJECT

public:
    BluecurveStyle();
    ~BluecurveStyle();

    void polish(QWidget *widget) override;
    void unpolish(QWidget *widget) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt,
                       QPainter *p, const QWidget *widget) const override;
    void drawControl(ControlElement element, const QStyleOption *opt,
                     QPainter *p, const QWidget *widget) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *opt,
                            QPainter *p, const QWidget *widget) const override;

    QRect subElementRect(SubElement element, const QStyleOption *opt,
                         const QWidget *widget) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *opt,
                    const QWidget *widget) const override;
    QSize sizeFromContents(ContentsType contents, const QStyleOption *opt,
                           const QSize &contentsSize, const QWidget *widget) const override;
    int styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget,
                  QStyleHintReturn *returnData) const override;

    QPalette standardPalette() const override;

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    // busy（indeterminate）进度条集合。QProgressBar 通过 polish 安装的事件
    // 过滤器进出这个集合，集合非空时由 timerEvent 驱动重绘。
    void startProgressAnimation(QProgressBar *bar);
    void stopProgressAnimation(QProgressBar *bar);

    QList<QProgressBar *> animatedBars;
    int animateTimer = 0;
    int animationFps = 30;

    // Palette-derived color ramp. shades[] are the button-color bevel
    // ramp (light -> dark), spots[] are derived from the highlight color
    // and used for gradient borders.
    struct ColorData {
        QColor shades[8];
        QColor spots[3];
        QRgb buttonRgb = 0;
        QRgb highlightRgb = 0;
    };

    const ColorData *colorData(const QPalette &palette) const;
    static void shadeColor(const QColor &src, QColor &dst, double factor);

    void drawLightBevel(QPainter *p, const QStyleOption *opt,
                        const QBrush *fill, bool darkBorder) const;
    void drawTextRect(QPainter *p, const QStyleOption *opt, const QBrush *fill) const;
    void drawGradient(QPainter *p, const QRect &rect, const QPalette &pal,
                      double shade1, double shade2) const;
    void drawGradientBox(QPainter *p, const QRect &rect, const QPalette &pal,
                         double shade1, double shade2) const;

    void drawArrow(QPainter *p, PrimitiveElement pe, const QRect &r,
                   const QColor &color) const;
    void drawCheckMark(QPainter *p, const QRect &r, const QColor &color) const;
    void drawCheckBox(QPainter *p, const QStyleOption *opt, bool on, bool tri) const;
    void drawRadioButton(QPainter *p, const QStyleOption *opt, bool on) const;
    void drawSliderGrip(QPainter *p, const ColorData *cdata, const QRect &r, bool horizontal) const;

    mutable QHash<quint64, ColorData> m_colorCache;
};

#endif // BLUECURVESTYLE_H
