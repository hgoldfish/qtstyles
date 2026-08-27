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

#ifndef PLATINUMSTYLE_H
#define PLATINUMSTYLE_H

#include <QProxyStyle>
#include <QtCore/qlist.h>

class QColor;
class QPainter;
class QPalette;
class QProgressBar;
class QRect;
class QSize;
class QStyleOption;
class QStyleOptionProgressBar;
class QTimer;
class QWidget;

/*
    Platinum is a clean-room re-implementation of the "Platinum" look of
    classic Macintosh system software (Mac OS 8/9): a warm beige/gray color
    scheme, square command buttons with cut corners, multi-line beveled
    panels, radio buttons drawn as point-sequence circles, square check
    boxes, accent-colored scroll/slider thumbs with riffles, Mac OS 9
    signature textures (title-bar racing stripes, grow-box size grip),
    disclosure triangles, primary group boxes and accent focus rings.

    The style is built on QProxyStyle with the "windows" style as its base,
    so every element that is not re-drawn here keeps a solid classic look.
    The visual specification (element drawing order, color roles, metrics and
    sub-control layout) is derived from the Qt 3 QPlatinumStyle; no code from
    it is included in this implementation.
*/
class PlatinumStyle : public QProxyStyle
{
    Q_OBJECT

public:
    // 当 forceClassicPalette 为 true 时（对应 "platinum-classic" 键），样式
    // 安装时通过 polish(QPalette&) 强制套用 standardPalette() 的经典配色。
    explicit PlatinumStyle(bool forceClassicPalette = false);
    ~PlatinumStyle() override;

    void polish(QWidget *widget) override;
    void polish(QPalette &palette) override;
    void unpolish(QWidget *widget) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;
    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override;
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl subControl, const QWidget *widget) const override;
    QRect subElementRect(SubElement element, const QStyleOption *option,
                         const QWidget *widget) const override;

    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                    const QWidget *widget) const override;

    QPalette standardPalette() const override;

private:
    // busy（indeterminate）进度条集合。QProgressBar 通过 polish 安装的事件
    // 过滤器进出这个集合，集合非空时由 QTimer 驱动重绘。
    void startProgressAnimation(QProgressBar *bar);
    void stopProgressAnimation(QProgressBar *bar);
    void animateProgressBars();

    QList<QProgressBar *> animatedBars;
    QTimer *animationTimer = nullptr;
    int animationFps = 30;

    // Qt 3 PE_ButtonBevel: a multi-line bevel drawn with plain lines, square
    // corners. Small bevels (tiny or strongly non-square faces) use the
    // 2-pixel ramp, large bevels the 3-pixel ramp with mixed corner dots.
    // @p sunken draws the pressed face (filled with @p fill, default Mid).
    void drawBevel(QPainter *painter, const QRect &rect, const QPalette &palette,
                   bool sunken, const QBrush &fill) const;

    // Qt 3 PE_ButtonCommand: the big "command" push button. A square face
    // with 4-pixel rounded corners (cut by background dots), a shadow ring
    // and a light/mid bevel ramp.
    void drawCommandButton(QPainter *painter, const QRect &rect, const QPalette &palette,
                           bool sunken) const;

    // Qt 3 PE_ScrollBarAddPage / SubPage: recessed trough with per-orientation
    // dark/light line shading.
    void drawScrollBarPage(QPainter *painter, const QRect &rect, const QPalette &palette,
                           bool horizontal) const;

    // Qt 3 qDrawShadePanel(): a sunken panel with a 2-pixel Dark top/left and
    // Light bottom/right border. Used for the editable combo box field.
    void drawSunkenPanel(QPainter *painter, const QRect &rect, const QPalette &palette) const;

    void drawPlatinumFocusRect(QPainter *painter, const QRect &rect,
                               const QPalette &palette) const;

    // A set of short parallel lines ("riffles") centered in @p rect.
    // @p horizontal is true when the widget moves horizontally, so the
    // riffles are drawn as vertical lines.
    void drawRiffles(QPainter *painter, const QRect &rect, const QPalette &palette,
                     bool horizontal) const;
    void drawRiffles(QPainter *painter, const QRect &rect, const QColor &light,
                     const QColor &dark, bool horizontal) const;

    // Appearance Manager "variation" / accent color: scroll thumbs, slider
    // tabs and focus rings. Derived from Highlight; very dark highlights are
    // lifted into the classic periwinkle range so riffles stay readable.
    static QColor accentColor(const QPalette &palette);

    // Mac OS 8/9 title-bar "racing stripes": alternating Light/Dark 1 px
    // lines. @p vertical draws columns instead of rows (for vertical dock
    // title bars).
    void drawRacingStripes(QPainter *painter, const QRect &rect,
                           const QPalette &palette, bool vertical = false) const;

    // Mac OS grow box: diagonal Light/Dark ridges in the status-bar corner.
    void drawSizeGrip(QPainter *painter, const QStyleOption *option) const;

    // Dock title with racing stripes (active) and a solid title gap, like
    // a Platinum window title bar.
    void drawDockWidgetTitle(QPainter *painter, const QStyleOption *option) const;

    // Finder-style disclosure triangle + guide lines (PE_IndicatorBranch).
    void drawBranch(QPainter *painter, const QStyleOption *option) const;

    // Primary group box: raised 2 px frame with the title punched out.
    void drawGroupBox(QPainter *painter, const QStyleOption *option,
                      const QWidget *widget) const;

    // Qt 3 Windows-style three-line arrow (also used for scroll bar and
    // combo box buttons): the outline is drawn in @p line and the arrow tip
    // in @p point.
    void drawArrow(QPainter *painter, PrimitiveElement arrow, const QRect &rect,
                   const QColor &line, const QColor &point) const;

    // Enabled: solid @p enabledColor glyph. Disabled: Qt 3 Light etch at
    // +1,+1 under a Mid glyph (the missing “disabled arrow shadow”).
    void drawArrowGlyph(QPainter *painter, PrimitiveElement arrow, const QRect &rect,
                        const QPalette &palette, bool enabled,
                        const QColor &enabledColor) const;

    // Double-drawn check mark (a dark offset shadow pass plus the main pass),
    // like the Qt 3 point-array check marks.
    void drawCheckMark(QPainter *painter, const QRect &rect, const QColor &color,
                       const QColor &shadow) const;
    void drawCheckBox(QPainter *painter, const QStyleOption *option) const;
    void drawRadioButton(QPainter *painter, const QStyleOption *option) const;

    // Qt 3 CE_ProgressBarContents: deterministic bars delegate to the base
    // style (the chunk grid is drawn through PE_IndicatorProgressChunk);
    // a busy bar (range 0..0) shows Qt 3's sweeping 4-pixel highlight line
    // instead of the Qt 6 chunk animation.
    void drawProgressBarContents(QPainter *painter, const QStyleOptionProgressBar *option,
                                 const QWidget *widget) const;

    static QColor mixedColor(const QColor &c1, const QColor &c2);

    bool m_forceClassicPalette;
};

#endif // PLATINUMSTYLE_H
