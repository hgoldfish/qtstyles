//////////////////////////////////////////////////////////////////////////////
// phasestyle.h
// -------------------
// Qt widget style
// -------------------
// Copyright (c) 2004-2007 David Johnson <david@usermode.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#ifndef PHASESTYLE_H
#define PHASESTYLE_H

#include <QProxyStyle>

class QStyleOptionTab;
class QProgressBar;
class QTimer;

class PhaseStyle : public QProxyStyle
{
    Q_OBJECT
public:
    PhaseStyle();
    ~PhaseStyle() override;

    void polish(QApplication* app) override;
    void polish(QWidget *widget) override;
    void polish(QPalette &pal) override;
    void unpolish(QApplication *app) override;
    void unpolish(QWidget *widget) override;

    QPalette standardPalette() const override;

    void drawPrimitive(PrimitiveElement element,
            const QStyleOption *option,
            QPainter *painter,
            const QWidget *widget = 0) const override;

    void drawControl(ControlElement element,
            const QStyleOption *option,
            QPainter *painter,
            const QWidget *widget = 0) const override;

    void drawComplexControl(ComplexControl control,
            const QStyleOptionComplex *option,
            QPainter *painter,
            const QWidget *widget = 0) const override;

    QPixmap standardPixmap(StandardPixmap pixmap,
            const QStyleOption *option,
            const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric,
            const QStyleOption *option = 0,
            const QWidget *widget = 0) const override;

    QRect subElementRect(SubElement element,
            const QStyleOption *option,
            const QWidget *widget) const override;

    QRect subControlRect(ComplexControl control,
            const QStyleOptionComplex *option,
            SubControl subcontrol,
            const QWidget *widget = 0) const override;

    SubControl hitTestComplexControl(ComplexControl control,
            const QStyleOptionComplex *option,
            const QPoint &position,
            const QWidget *widget = 0) const override;

    int styleHint(StyleHint hint,
            const QStyleOption *option = 0,
            const QWidget *widget = 0,
            QStyleHintReturn *data = 0) const override;

private:
    enum GradientType {
        Horizontal,
        Vertical,
        HorizontalReverse,
        VerticalReverse,
        GradientCount
    };

    enum BitmapType {
        UArrow,
        DArrow,
        LArrow,
        RArrow,
        PlusSign,
        MinusSign,
        CheckMark,
        TitleClose,
        TitleMin,
        TitleMax,
        TitleNormal,
        TitleHelp
    };

    PhaseStyle(const PhaseStyle &);
    PhaseStyle& operator=(const PhaseStyle &);

    void drawPhaseGradient(QPainter *painter,
            const QRect &rect,
            QColor color,
            bool horizontal,
            const QSize &gsize = QSize(),
            bool reverse=false) const;

    void drawPhaseBevel(QPainter *painter,
            QRect rect,
            const QPalette &pal,
	    const QBrush &fill,
            bool sunken=false,
            bool horizontal=true,
            bool reverse=false) const;

    void drawPhaseButton(QPainter *painter,
            QRect rect,
            const QPalette &pal,
	    const QBrush &fill,
            bool sunken=false) const;

    void drawPhasePanel(QPainter *painter,
            const QRect &rect,
            const QPalette &pal,
            const QBrush &fill,
            bool sunken = false) const;

    void drawPhaseDoodads(QPainter *painter,
            const QRect &rect,
            const QPalette &pal,
            bool horizontal) const;

    void drawPhaseTab(QPainter *painter,
            const QPalette &pal,
            const QStyleOptionTab *option) const;

    void animateProgressBars();
    void addProgressBar(QProgressBar *bar);
    void removeProgressBar(QProgressBar *bar);
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    int contrast_;
    bool gradients_;
    bool highlights_;

    QList<QBitmap> bitmaps_;
    QList<QProgressBar*> bars_; // animated progressbars
    QTimer *timer_;
};

#endif // PHASESTYLE_H
