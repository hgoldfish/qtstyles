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

#include "bluecurvestyle.h"
#include "qtstyles_palette.h"

#include <QtWidgets/qapplication.h>
#include <QtWidgets/qabstractbutton.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qprogressbar.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qscrollbar.h>
#include <QtWidgets/qslider.h>
#include <QtWidgets/qsplitter.h>
#include <QtWidgets/qspinbox.h>
#include <QtWidgets/qtabbar.h>
#include <QtWidgets/qtoolbutton.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qstyleoption.h>

#include <QtGui/qpainter.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/qtransform.h>

#include <QtCore/qmath.h>
#include <QtCore/qhash.h>

#include <chrono>

// Qt 6 renamed QStyleOptionMenuItem::tabWidth to reservedShortcutWidth.
static inline int menuItemTabWidth(const QStyleOptionMenuItem *mi)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return mi->reservedShortcutWidth;
#else
    return mi->tabWidth;
#endif
}

// 单调毫秒时钟。busy 动画的块位置由它算出：wall-clock 的
// msecsSinceStartOfDay 在午夜回绕会让相位在午夜跳变；单调钟没有这个问题。
static qint64 steadyMs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

// 计算 progressbar 的填充块矩形（widget 坐标系）。CE_ProgressBarContents 和
// CE_ProgressBarLabel 都要用它，保证两处对"块的位置"理解一致：块内文字用
// 白色、块外文字用黑色。垂直条沿用 QCommonStyle 的"当成水平条画再旋转"
// 思路：先交换宽高当水平条算块，再把块映射回 widget 坐标。
static QRect progressChunkRect(const QStyleOptionProgressBar *pb, const QRect &optRect)
{
    QRect rect = optRect;
    const bool vertical = !(pb->state & QStyle::State_Horizontal);
    QTransform m;
    if (vertical) {
        rect = QRect(rect.y(), rect.x(), rect.height(), rect.width());
        m.rotate(90);
        m.translate(0, -(rect.height() + rect.y() * 2));
    }

    bool reverse = vertical || pb->direction == Qt::RightToLeft;
    if (pb->invertedAppearance)
        reverse = !reverse;

    QRect pr;
    if (pb->minimum == 0 && pb->maximum == 0) {
        // 忙碌指示器：来回滑动的一块。Qt 6 的 indeterminate QProgressBar 不更新
        // style option 的 progress 字段（实测恒为初始值），沿用 QCommonStyle
        // "progress % 周期" 的方案块不会动，所以改用时钟驱动，与 Fusion 的
        // QProgressStyleAnimation 一致。块宽取条宽的 1/3（至少 25px），过窄的
        // 块在长条上滑动肉眼几乎不可见；周期取 2400ms 一个来回，兼顾显眼与平滑。
        // qMin 兜底：条宽不足 25px 时块不得超过条本身，否则块溢出且滑动范围
        // 退化为 1px（remains 被钳到 1）。
        const int w = qMin(rect.width(), qMax(25, rect.width() / 3));
        const int remains = qMax(rect.width() - w, 1);
        const int period = 2400;               // 一个完整来回的毫秒数
        const int t = int(steadyMs() % period);
        int x = (t < period / 2) ? t * remains / (period / 2)
                                 : (period - t) * remains / (period / 2);
        x = reverse ? rect.right() - x - w : x + rect.x();
        pr.setRect(x, rect.y(), w, rect.height());
    } else {
        const int total = qMax(pb->maximum - pb->minimum, 1);
        const int w = int(qreal(pb->progress) * rect.width() / total);
        pr = reverse ? QRect(rect.right() - w, rect.y(), w, rect.height())
                     : QRect(rect.x(), rect.y(), w, rect.height());
    }

    if (vertical)
        pr = m.mapRect(QRectF(pr)).toRect();
    return pr;
}

// Bevel ramp applied to the button color (light -> dark). These factors
// mirror the brightness steps of the original Bluecurve palette ramp.
static const double shadeFactors[8] = {
    1.065, 0.963, 0.896, 0.85, 0.768, 0.665, 0.4, 0.205
};

BluecurveStyle::BluecurveStyle() = default;
BluecurveStyle::~BluecurveStyle() = default;

/*!
    \reimp

    Returns the classic Red Hat 8/9 palette that the Bluecurve look was
    designed for: light beige-grey surfaces with the era's GNOME blue
    highlight (#7590AE).  This is a suggestion only -- Qt does not adopt it
    automatically; callers may apply it with QApplication::setPalette().
*/
QPalette BluecurveStyle::standardPalette() const
{
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(0xe6, 0xe6, 0xe6));
    pal.setColor(QPalette::WindowText, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::AlternateBase, QColor(0xee, 0xee, 0xee));
    pal.setColor(QPalette::ToolTipBase, QColor(0xff, 0xff, 0xdc));
    pal.setColor(QPalette::ToolTipText, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::Text, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::Button, QColor(0xd9, 0xd9, 0xd9));
    pal.setColor(QPalette::ButtonText, QColor(0x00, 0x00, 0x00));
    pal.setColor(QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::Light, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::Midlight, QColor(0xe2, 0xe2, 0xe2));
    pal.setColor(QPalette::Mid, QColor(0xc0, 0xc0, 0xc0));
    pal.setColor(QPalette::Dark, QColor(0xa0, 0xa0, 0xa0));
    pal.setColor(QPalette::Shadow, QColor(0x80, 0x80, 0x80));
    pal.setColor(QPalette::Highlight, QColor(0x75, 0x90, 0xae));
    pal.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    pal.setColor(QPalette::PlaceholderText, QColor(0x80, 0x80, 0x80));
    pal.setColor(QPalette::Link, QColor(0x00, 0x00, 0xee));
    pal.setColor(QPalette::LinkVisited, QColor(0x52, 0x18, 0x8b));

    QtStyles::applyClassicDisabled(&pal);
    return pal;
}

void BluecurveStyle::shadeColor(const QColor &src, QColor &dst, double factor)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    float h = 0.0f, s = 0.0f, l = 0.0f;
#else
    double h = 0.0, s = 0.0, l = 0.0;
#endif
    src.getHslF(&h, &s, &l);
    s = double(qBound(0.0, double(s) * factor, 1.0));
    l = double(qBound(0.0, double(l) * factor, 1.0));
    dst.setHslF(h, s, l);
}

const BluecurveStyle::ColorData *BluecurveStyle::colorData(const QPalette &palette) const
{
    const QRgb buttonRgb = palette.button().color().rgb();
    const QRgb highlightRgb = palette.highlight().color().rgb();
    // Compact 64-bit key: 32 bits of button color, 32 bits of highlight color.
    const quint64 key = (quint64(buttonRgb) << 32) | quint64(highlightRgb);

    const auto it = m_colorCache.constFind(key);
    if (it != m_colorCache.constEnd())
        return &it.value();

    ColorData data;
    data.buttonRgb = buttonRgb;
    data.highlightRgb = highlightRgb;
    for (int i = 0; i < 8; ++i)
        shadeColor(palette.button().color(), data.shades[i], shadeFactors[i]);
    shadeColor(palette.highlight().color(), data.spots[0], 1.62);
    shadeColor(palette.highlight().color(), data.spots[1], 1.05);
    shadeColor(palette.highlight().color(), data.spots[2], 0.72);

    m_colorCache.insert(key, data);
    const auto cit = m_colorCache.constFind(key);
    return &cit.value();
}

// ---------------------------------------------------------------------------
// 基础绘制原语
// ---------------------------------------------------------------------------

void BluecurveStyle::drawLightBevel(QPainter *p, const QStyleOption *opt,
                                    const QBrush *fill, bool darkBorder) const
{
    const ColorData *cdata = colorData(opt->palette);
    const QRect r = opt->rect;
    QRect br = r;
    const bool sunken = opt->state & (QStyle::State_On | QStyle::State_Sunken);

    p->save();
    p->setPen(darkBorder ? cdata->shades[6] : cdata->shades[5]);
    p->drawRect(r.adjusted(0, 0, -1, -1));

    if (opt->state & (QStyle::State_On | QStyle::State_Sunken | QStyle::State_Raised)) {
        // Button bevel: bright rim top/left, dark rim bottom/right.
        p->setPen(sunken ? Qt::white : cdata->shades[2]);
        p->drawLine(r.left() + r.width() - 2, r.top() + 2,
                    r.left() + r.width() - 2, r.top() + r.height() - 3);
        p->drawLine(r.left() + 1, r.top() + r.height() - 2,
                    r.left() + r.width() - 2, r.top() + r.height() - 2);
        p->setPen(sunken ? cdata->shades[2] : Qt::white);
        p->drawLine(r.left() + 1, r.top() + 2,
                    r.left() + 1, r.top() + r.height() - 2);
        p->drawLine(r.left() + 1, r.top() + 1,
                    r.left() + r.width() - 2, r.top() + 1);
        br.adjust(2, 2, -2, -2);
    } else {
        br.adjust(1, 1, -1, -1);
    }

    if (fill)
        p->fillRect(br, *fill);
    p->restore();
}

void BluecurveStyle::drawTextRect(QPainter *p, const QStyleOption *opt,
                                  const QBrush *fill) const
{
    const ColorData *cdata = colorData(opt->palette);
    const QRect r = opt->rect;
    QRect br = r;

    p->save();
    // Dark outer frame.
    p->setPen(cdata->shades[5]);
    p->drawRect(r.adjusted(0, 0, -1, -1));
    // Bevel: bright bottom/right, dark top/left (sunken well).
    p->setPen(opt->palette.light().color());
    p->drawLine(r.left() + r.width() - 2, r.top() + 2,
                r.left() + r.width() - 2, r.top() + r.height() - 3);
    p->drawLine(r.left() + 2, r.top() + r.height() - 2,
                r.left() + r.width() - 2, r.top() + r.height() - 2);
    p->setPen(cdata->shades[1]);
    p->drawLine(r.left() + 1, r.top() + 2,
                r.left() + 1, r.top() + r.height() - 2);
    p->drawLine(r.left() + 1, r.top() + 1,
                r.left() + r.width() - 2, r.top() + 1);

    br.adjust(2, 2, -2, -2);
    if (fill)
        p->fillRect(br, *fill);
    p->restore();
}

void BluecurveStyle::drawGradient(QPainter *p, const QRect &rect, const QPalette &pal,
                                  double shade1, double shade2) const
{
    if (rect.width() <= 0 || rect.height() <= 0)
        return;
    QColor c1, c2;
    shadeColor(pal.highlight().color(), c1, shade1);
    shadeColor(pal.highlight().color(), c2, shade2);

    p->save();
    QLinearGradient grad(rect.topLeft(), rect.topRight());
    grad.setColorAt(0.0, c1);
    grad.setColorAt(1.0, c2);
    p->fillRect(rect, grad);
    p->restore();
}

void BluecurveStyle::drawGradientBox(QPainter *p, const QRect &rect, const QPalette &pal,
                                     double shade1, double shade2) const
{
    const ColorData *cdata = colorData(pal);
    p->save();
    const QRect grad = rect.adjusted(2, 2, -3, -3);
    drawGradient(p, grad, pal, shade1, shade2);

    // 3D border effect using the three highlight-derived spots.
    p->setPen(cdata->spots[2]);
    p->setBrush(Qt::NoBrush);
    p->drawRect(rect.adjusted(0, 0, -1, -1));
    p->setPen(cdata->spots[1]);
    p->drawLine(rect.left() + 1, rect.bottom() - 1, rect.right() - 1, rect.bottom() - 1);
    p->drawLine(rect.right() - 1, rect.top() + 1, rect.right() - 1, rect.bottom() - 1);
    p->setPen(cdata->spots[0]);
    p->drawLine(rect.left() + 1, rect.top() + 1, rect.right() - 1, rect.top() + 1);
    p->drawLine(rect.left() + 1, rect.top() + 1, rect.left() + 1, rect.bottom() - 1);
    p->restore();
}

// ---------------------------------------------------------------------------
// 箭头、对勾、指示器
// ---------------------------------------------------------------------------

void BluecurveStyle::drawArrow(QPainter *p, PrimitiveElement pe, const QRect &r,
                               const QColor &color) const
{
    const bool up   = pe == PE_IndicatorArrowUp;
    const bool down = pe == PE_IndicatorArrowDown;
    const bool left = pe == PE_IndicatorArrowLeft;

    // QRect::center() in Qt 6 is x + (w - 1) / 2, which biases even-size
    // rects towards the top-left; compute the true middle pixel instead.
    const int cx = (r.left() + r.right()) / 2;
    const int cy = (r.top() + r.bottom()) / 2;

    // Odd triangle size filling most of the smaller rect dimension.  Small
    // indicators (combo/spin/scrollbar) never drop below 5px; the row-based
    // fill below keeps the apex on a real pixel instead of letting the scanline
    // rasterizer clip it off.
    int maxDim = qMin(r.width(), r.height());
    int size = maxDim * 3 / 4;
    if (size < 5)
        size = 5;
    if (size > maxDim)
        size = maxDim;
    size |= 1;                  // odd => the apex lands on a single pixel
    if (size > maxDim)
        size -= 2;
    if (size < 3)
        return;

    const int half = size / 2;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    p->setPen(Qt::NoPen);
    p->setBrush(color);
    if (up || down) {
        const bool tipFirst = up;
        const int y0 = cy - half;
        for (int i = 0; i <= half; ++i) {
            const int w = tipFirst ? i * 2 + 1 : (half - i) * 2 + 1;
            p->drawRect(cx - w / 2, y0 + i, w, 1);
        }
    } else {
        const bool tipFirst = left;
        const int x0 = cx - half;
        for (int i = 0; i <= half; ++i) {
            const int w = tipFirst ? i * 2 + 1 : (half - i) * 2 + 1;
            p->drawRect(x0 + i, cy - w / 2, 1, w);
        }
    }
    p->restore();
}

void BluecurveStyle::drawCheckMark(QPainter *p, const QRect &r, const QColor &color) const
{
    // 标准对勾：左下起笔 -> 右下折点 -> 右上收笔，落在 r 内部并居中。
    // 相对坐标避免取整偏移；pen 2 保证在 13px 的 indicator 里够粗但不糊。
    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p->setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(r.left() + r.width() * 0.30, r.top() + r.height() * 0.55);
    path.lineTo(r.left() + r.width() * 0.50, r.top() + r.height() * 0.80);
    path.lineTo(r.left() + r.width() * 0.82, r.top() + r.height() * 0.30);
    p->drawPath(path);
    p->restore();
}

void BluecurveStyle::drawCheckBox(QPainter *p, const QStyleOption *opt, bool on, bool tri) const
{
    const ColorData *cdata = colorData(opt->palette);
    const QRect r = opt->rect.adjusted(1, 1, -1, -1);

    p->save();
    p->setPen(cdata->shades[5]);
    p->setBrush(Qt::white);
    p->drawRect(r);
    // Inner bevel on the well.
    p->setPen(cdata->shades[1]);
    p->drawLine(r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1);
    p->drawLine(r.left() + 1, r.top() + 1, r.left() + 1, r.bottom() - 1);
    p->setPen(cdata->shades[2]);
    p->drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
    p->drawLine(r.right() - 1, r.top() + 1, r.right() - 1, r.bottom() - 1);

    if (on) {
        // 勾使用前景文字色（黑），而不是 highlightedText（白）——
        // 勾画在白色方块上，白色会直接消失。
        drawCheckMark(p, r.adjusted(2, 2, -2, -2),
                      opt->palette.color(opt->state & QStyle::State_Enabled
                                             ? QPalette::Active
                                             : QPalette::Disabled,
                                         QPalette::Text));
    } else if (tri) {
        p->setPen(QPen(opt->palette.highlight().color(), 2));
        p->drawLine(r.left() + 2, r.center().y(), r.right() - 2, r.center().y());
    }
    p->restore();
}

void BluecurveStyle::drawRadioButton(QPainter *p, const QStyleOption *opt, bool on) const
{
    const ColorData *cdata = colorData(opt->palette);
    const QRect r = opt->rect.adjusted(1, 1, -1, -1);

    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setPen(cdata->shades[5]);
    p->setBrush(Qt::white);
    p->drawEllipse(r);

    p->setPen(cdata->shades[1]);
    p->drawArc(r, 45 * 16, 180 * 16);
    p->setPen(cdata->shades[2]);
    p->drawArc(r, 225 * 16, 180 * 16);

    if (on) {
        p->setPen(Qt::NoPen);
        p->setBrush(opt->state & QStyle::State_Enabled
                        ? opt->palette.highlight().color()
                        : opt->palette.text().color());
        p->drawEllipse(r.adjusted(r.width() / 3, r.height() / 3, -r.width() / 3, -r.height() / 3));
    }
    p->restore();
}

void BluecurveStyle::drawSliderGrip(QPainter *p, const ColorData *cdata, const QRect &r, bool horizontal) const
{
    p->save();
    p->setPen(cdata->shades[5]);
    if (horizontal) {
        const int x = r.center().x() - 7;
        const int y = (r.top() + r.bottom() - 6) / 2;
        for (int i = 0; i < 3; ++i) {
            p->drawLine(x + i * 5, y + 5, x + i * 5 + 5, y);
            p->setPen(Qt::white);
            p->drawLine(x + i * 5 + 1, y + 5, x + i * 5 + 6, y + 1);
            p->setPen(cdata->shades[5]);
        }
    } else {
        const int x = (r.left() + r.right() - 6) / 2;
        const int y = r.center().y() - 7;
        for (int i = 0; i < 3; ++i) {
            p->drawLine(x + 5, y + i * 5, x, y + i * 5 + 5);
            p->setPen(Qt::white);
            p->drawLine(x + 5, y + i * 5 + 1, x + 1, y + i * 5 + 6);
            p->setPen(cdata->shades[5]);
        }
    }
    p->restore();
}

// ---------------------------------------------------------------------------
// polish
// ---------------------------------------------------------------------------

void BluecurveStyle::polish(QWidget *widget)
{
    if (qobject_cast<QAbstractButton *>(widget) ||
        qobject_cast<QComboBox *>(widget) ||
        qobject_cast<QSplitterHandle *>(widget))
        widget->setAttribute(Qt::WA_Hover, true);

    if (qobject_cast<QScrollBar *>(widget) || qobject_cast<QAbstractSlider *>(widget)) {
        widget->setMouseTracking(true);
        widget->setAttribute(Qt::WA_Hover, true);
    }

    // 事件过滤器跟踪 QProgressBar 的可见性与范围，busy（min==max）的条由
    // timerEvent 按 animationFps 驱动重绘。与 oldschool/dirtylooks 同构。
    if (qobject_cast<QProgressBar *>(widget))
        widget->installEventFilter(this);

    QCommonStyle::polish(widget);
}

void BluecurveStyle::unpolish(QWidget *widget)
{
    if (QProgressBar *bar = qobject_cast<QProgressBar *>(widget)) {
        widget->removeEventFilter(this);
        stopProgressAnimation(bar);
    }
    QCommonStyle::unpolish(widget);
}

bool BluecurveStyle::eventFilter(QObject *watched, QEvent *event)
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
    return QCommonStyle::eventFilter(watched, event);
}

void BluecurveStyle::startProgressAnimation(QProgressBar *bar)
{
    if (!animatedBars.contains(bar)) {
        animatedBars << bar;
        if (!animateTimer) {
            Q_ASSERT(animationFps > 0);
            animateTimer = startTimer(1000 / animationFps);
        }
    }
}

void BluecurveStyle::stopProgressAnimation(QProgressBar *bar)
{
    if (!animatedBars.isEmpty()) {
        animatedBars.removeOne(bar);
        if (animatedBars.isEmpty() && animateTimer) {
            killTimer(animateTimer);
            animateTimer = 0;
        }
    }
}

void BluecurveStyle::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == animateTimer) {
        for (QProgressBar *bar : animatedBars) {
            if (bar->isVisible())
                bar->update();
        }
    }
    QCommonStyle::timerEvent(event);
}

// ---------------------------------------------------------------------------
// drawPrimitive
// ---------------------------------------------------------------------------

void BluecurveStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt,
                                   QPainter *p, const QWidget *widget) const
{
    const ColorData *cdata = colorData(opt->palette);
    const QRect r = opt->rect;

    switch (pe) {
    case PE_IndicatorHeaderArrow: {
        QStyleOption arrowOpt(*opt);
        arrowOpt.state |= QStyle::State_Enabled;
        drawPrimitive((opt->state & State_UpArrow) ? PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                      &arrowOpt, p, widget);
        break;
    }

    case PE_PanelButtonCommand:
    case PE_PanelButtonBevel:
    case PE_PanelButtonTool: {
        const QBrush *fill;
        if (opt->state & QStyle::State_Sunken)
            fill = &opt->palette.brush(QPalette::Mid);
        else if (opt->state & QStyle::State_MouseOver)
            fill = &opt->palette.brush(QPalette::Midlight);
        else if (opt->state & QStyle::State_On)
            fill = &opt->palette.brush(QPalette::Mid);
        else {
            const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(opt);
            fill = (button && (button->features & QStyleOptionButton::Flat))
                       ? nullptr
                       : &opt->palette.brush(QPalette::Button);
        }
        if (fill)
            drawLightBevel(p, opt, fill, true);
        break;
    }

    case PE_IndicatorButtonDropDown: {
        QBrush fill;
        const bool sunken = opt->state & (QStyle::State_On | QStyle::State_Sunken);
        QRect br = r;

        if (opt->state & QStyle::State_Sunken)
            fill = opt->palette.brush(QPalette::Mid);
        else if (opt->state & QStyle::State_MouseOver)
            fill = opt->palette.brush(QPalette::Midlight);
        else
            fill = (opt->state & (QStyle::State_On | QStyle::State_Open))
                       ? opt->palette.brush(QPalette::Mid)
                       : opt->palette.brush(QPalette::Button);

        p->save();
        p->setPen(sunken ? cdata->shades[6] : cdata->shades[4]);
        p->drawLine(r.topLeft(), r.bottomLeft());
        p->setPen(cdata->shades[6]);
        p->drawLine(r.topLeft(), r.topRight());
        p->drawLine(r.topRight(), r.bottomRight());
        p->drawLine(r.bottomRight(), r.bottomLeft());

        if (opt->state & (QStyle::State_On | QStyle::State_Sunken | QStyle::State_Raised)) {
            p->setPen(sunken ? Qt::white : cdata->shades[2]);
            p->drawLine(r.right() - 1, r.top() + 2, r.right() - 1, r.bottom() - 2);
            p->drawLine(r.left() + 1, r.bottom() - 1, r.right() - 1, r.bottom() - 1);
            p->setPen(sunken ? cdata->shades[2] : Qt::white);
            p->drawLine(r.left() + 1, r.top() + 2, r.left() + 1, r.bottom() - 2);
            p->drawLine(r.left() + 1, r.top() + 1, r.right() - 1, r.top() + 1);
            br.adjust(2, 2, -1, -2);
        } else {
            br.adjust(1, 1, 0, -1);
        }
        p->fillRect(br, fill);
        p->restore();
        break;
    }

    case PE_FrameButtonBevel:
    case PE_FrameButtonTool:
        drawLightBevel(p, opt, nullptr, true);
        break;

    case PE_FrameDefaultButton:
        p->save();
        p->setPen(opt->palette.shadow().color());
        p->setBrush(Qt::NoBrush);
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->restore();
        break;

    case PE_FrameFocusRect: {
        p->save();
        p->setPen(Qt::black);
        const int rw = r.width(), rh = r.height(), rx = r.x(), ry = r.y();
        for (int x = 0; x < rw; x += 2)
            p->drawPoint(rx + x, ry);
        for (int y = 0; y < rh; y += 2)
            p->drawPoint(rx, ry + y);
        for (int y = 1; y < rh; y += 2)
            p->drawPoint(rx + rw - 1, ry + y);
        for (int x = 1; x < rw; x += 2)
            p->drawPoint(rx + x, ry + rh - 1);
        p->restore();
        break;
    }

    case PE_IndicatorCheckBox: {
        const bool on = opt->state & State_On;
        const bool tri = opt->state & State_NoChange;
        drawCheckBox(p, opt, on, tri);
        break;
    }

    case PE_IndicatorMenuCheckMark: {
        const QPoint qp = QPoint(r.center().x() - 5, r.center().y() - 5);
        const QColor color = (opt->state & State_Selected)
                                 ? opt->palette.highlightedText().color()
                                 : opt->palette.buttonText().color();
        drawCheckMark(p, QRect(qp, QSize(10, 10)), color);
        break;
    }

    case PE_IndicatorRadioButton: {
        if (opt->state & State_MouseOver)
            p->fillRect(r, opt->palette.brush(QPalette::Midlight));
        else
            p->fillRect(r, opt->palette.brush(QPalette::Window));
        drawRadioButton(p, opt, opt->state & State_On);
        break;
    }

    case PE_IndicatorToolBarHandle: {
        p->fillRect(r, opt->palette.button().color());
        p->save();
        p->setPen(cdata->shades[5]);
        if (opt->state & State_Horizontal) {
            const int x = r.left() + (r.width() - 4) / 2;
            int y = r.top() + 3;
            for (; y + 3 < r.bottom(); y += 5) {
                p->drawLine(x, y + 3, x + 3, y);
                p->setPen(Qt::white);
                p->drawLine(x, y + 4, x + 3, y + 1);
                p->setPen(cdata->shades[5]);
            }
        } else {
            const int y = r.top() + (r.height() - 4) / 2;
            int x = r.left() + 3;
            for (; x + 3 < r.right(); x += 4) {
                p->drawLine(x + 3, y, x, y + 3);
                p->setPen(Qt::white);
                p->drawLine(x + 3, y + 1, x + 1, y + 4);
                p->setPen(cdata->shades[5]);
            }
        }
        p->restore();
        break;
    }

    case PE_IndicatorToolBarSeparator: {
        if (r.width() > 20 || r.height() > 20) {
            p->save();
            p->setPen(cdata->shades[5]);
            if (opt->state & State_Horizontal)
                p->drawLine(r.left() + 1, r.top() + 6, r.left() + 1, r.bottom() - 6);
            else
                p->drawLine(r.left() + 6, r.top() + 1, r.right() - 6, r.top() + 1);
            p->setPen(cdata->shades[3]);
            if (opt->state & State_Horizontal)
                p->drawLine(r.left() + 2, r.top() + 6, r.left() + 2, r.bottom() - 6);
            else
                p->drawLine(r.left() + 6, r.top() + 2, r.right() - 6, r.top() + 2);
            p->restore();
        } else {
            QCommonStyle::drawPrimitive(pe, opt, p, widget);
        }
        break;
    }

    case PE_PanelLineEdit:
        drawTextRect(p, opt, &opt->palette.base());
        break;

    case PE_FrameTabWidget: {
        p->save();
        p->setPen(cdata->shades[6]);
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->setPen(opt->palette.light().color());
        p->drawLine(r.left() + 1, r.top() + 1, r.left() + 1, r.bottom() - 2);
        p->drawLine(r.left() + 1, r.bottom() - 2, r.right() - 2, r.bottom() - 2);
        p->drawLine(r.right() - 2, r.bottom() - 2, r.right() - 2, r.top() + 1);
        p->drawLine(r.left() + 1, r.top() + 1, r.right() - 2, r.top() + 1);
        p->restore();
        break;
    }

    case PE_FrameGroupBox: {
        const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt);
        p->save();
        if (frame && (frame->features & QStyleOptionFrame::Flat)) {
            p->setPen(cdata->shades[3]);
            p->drawLine(r.left(), r.top(), r.right(), r.top());
            p->setPen(cdata->shades[0]);
            p->drawLine(r.left(), r.top() + 1, r.right(), r.top() + 1);
        } else {
            p->setPen(cdata->shades[6]);
            p->drawRect(r.adjusted(0, 0, -1, -1));
            p->setPen(cdata->shades[3]);
            p->drawRect(r.adjusted(1, 1, -2, -2));
            p->setPen(cdata->shades[0]);
            p->drawLine(r.left() + 2, r.top() + 2, r.left() + 2, r.bottom() - 2);
            p->drawLine(r.left() + 2, r.top() + 2, r.right() - 2, r.top() + 2);
        }
        p->restore();
        break;
    }

    case PE_Frame:
    case PE_FrameWindow:
    case PE_FrameMenu: {
        QStyleOption optCopy(*opt);
        if (!(optCopy.state & State_Sunken))
            optCopy.state |= State_Raised;
        drawLightBevel(p, &optCopy, nullptr, false);
        break;
    }

    case PE_PanelMenu:
        p->fillRect(opt->rect, opt->palette.window().color());
        break;

    case PE_FrameDockWidget:
    case PE_PanelMenuBar: {
        p->fillRect(opt->rect, opt->palette.button().color());
        p->save();
        p->setPen(cdata->shades[3]);
        p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        p->restore();
        break;
    }

    case PE_PanelStatusBar: {
        p->save();
        p->setPen(cdata->shades[3]);
        p->drawLine(r.left(), r.top(), r.right(), r.top());
        p->setPen(cdata->shades[0]);
        p->drawLine(r.left(), r.top() + 1, r.right(), r.top() + 1);
        p->restore();
        break;
    }

    case PE_IndicatorProgressChunk:
        drawGradientBox(p, r, opt->palette, 0.92, 1.66);
        break;

    case PE_IndicatorArrowUp:
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowRight:
    case PE_IndicatorArrowLeft: {
        const QRect ar = r.adjusted(1, 1, -1, -1);
        if (opt->state & State_Enabled) {
            drawArrow(p, pe, ar,
                      (opt->state & State_Selected) ? opt->palette.highlightedText().color()
                                                    : opt->palette.buttonText().color());
        } else {
            drawArrow(p, pe, ar.translated(1, 1), Qt::white);
            drawArrow(p, pe, ar, cdata->shades[7]);
        }
        break;
    }

    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        QStyleOption arrowOpt(*opt);
        arrowOpt.rect = opt->rect.adjusted(1, 1, -1, -1);
        drawPrimitive(pe == PE_IndicatorSpinUp ? PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                      &arrowOpt, p, widget);
        break;
    }

    default:
        QCommonStyle::drawPrimitive(pe, opt, p, widget);
        break;
    }
}

// ---------------------------------------------------------------------------
// drawControl
// ---------------------------------------------------------------------------

void BluecurveStyle::drawControl(ControlElement element, const QStyleOption *opt,
                                 QPainter *p, const QWidget *widget) const
{
    const ColorData *cdata = colorData(opt->palette);
    const QRect r = opt->rect;

    switch (element) {
    case CE_PushButton:
        // The face, default-button indicator and label are all drawn by
        // QCommonStyle::drawControl(CE_PushButton) (which paints
        // CE_PushButtonBevel through PE_PanelButtonCommand /
        // PE_FrameDefaultButton and then CE_PushButtonLabel).
        QCommonStyle::drawControl(element, opt, p, widget);
        break;

    case CE_PushButtonLabel: {
        const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(opt);
        if (!button)
            break;
        QRect textRect = button->rect;
        int tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
        if (!proxy()->styleHint(SH_UnderlineShortcut, button, widget))
            tf |= Qt::TextHideMnemonic;

        if (button->features & QStyleOptionButton::HasMenu) {
            const int indicatorSize = pixelMetric(PM_MenuButtonIndicator, button, widget);
            textRect = button->direction == Qt::LeftToRight
                           ? textRect.adjusted(0, 0, -indicatorSize, 0)
                           : textRect.adjusted(indicatorSize, 0, 0, 0);
        }

        if (!button->icon.isNull()) {
            const QIcon::Mode mode = QIcon::Normal;
            const QIcon::State state = (button->state & State_On) ? QIcon::On : QIcon::Off;
            QPixmap pixmap = button->icon.pixmap(button->iconSize, mode, state);
            const int pw = pixmap.width() / pixmap.devicePixelRatio();
            const int ph = pixmap.height() / pixmap.devicePixelRatio();
            const int spacing = 4;
            int labelWidth = pw;
            if (!button->text.isEmpty()) {
                const int textWidth = button->fontMetrics.boundingRect(textRect, tf, button->text).width();
                labelWidth += textWidth + spacing;
            }
            QRect iconRect(textRect.x() + (textRect.width() - labelWidth) / 2,
                           textRect.y() + (textRect.height() - ph) / 2, pw, ph);
            iconRect = visualRect(button->direction, textRect, iconRect);
            if (button->direction == Qt::RightToLeft)
                textRect.setRight(iconRect.left() - spacing / 2);
            else
                textRect.setLeft(iconRect.left() + iconRect.width() + spacing / 2);
            p->drawPixmap(iconRect, pixmap);
        } else {
            tf |= Qt::AlignHCenter;
        }

        drawItemText(p, textRect, tf, button->palette, button->state & State_Enabled,
                     button->text, QPalette::ButtonText);
        break;
    }

    case CE_ToolButtonLabel: {
        const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        if (!toolbutton)
            break;
        QRect rect = toolbutton->rect;
        int shiftX = 0, shiftY = 0;
        if (toolbutton->state & (State_Sunken | State_On)) {
            shiftX = proxy()->pixelMetric(PM_ButtonShiftHorizontal, toolbutton, widget);
            shiftY = proxy()->pixelMetric(PM_ButtonShiftVertical, toolbutton, widget);
        }
        const bool hasArrow = toolbutton->features & QStyleOptionToolButton::Arrow;

        if (((!hasArrow && toolbutton->icon.isNull()) && !toolbutton->text.isEmpty())
            || toolbutton->toolButtonStyle == Qt::ToolButtonTextOnly) {
            // 纯文字。
            int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
                alignment |= Qt::TextHideMnemonic;
            rect.translate(shiftX, shiftY);
            p->setFont(toolbutton->font);
            drawItemText(p, rect, alignment, toolbutton->palette,
                         opt->state & State_Enabled, toolbutton->text, QPalette::ButtonText);
            break;
        }

        // 图元：图标 pixmap 或箭头。布局结构与 QCommonStyle 一致：图元占
        // pmSize+4 的区域，文字排在其旁/下；IconOnly 时只画图元。
        QPixmap pm;
        QSize pmSize = toolbutton->iconSize;
        PrimitiveElement arrowPE = PE_IndicatorArrowRight;
        if (!hasArrow) {
            const QIcon::State state = (toolbutton->state & State_On) ? QIcon::On : QIcon::Off;
            const QIcon::Mode mode = (toolbutton->state & State_Enabled) ? QIcon::Normal : QIcon::Disabled;
            pm = toolbutton->icon.pixmap(toolbutton->rect.size().boundedTo(toolbutton->iconSize),
                                         mode, state);
            pmSize = pm.size() / pm.devicePixelRatio();
        } else {
            switch (toolbutton->arrowType) {
            case Qt::LeftArrow: arrowPE = PE_IndicatorArrowLeft; break;
            case Qt::UpArrow: arrowPE = PE_IndicatorArrowUp; break;
            case Qt::DownArrow: arrowPE = PE_IndicatorArrowDown; break;
            default: break;
            }
            pmSize = QSize(9, 8);
        }

        if (toolbutton->toolButtonStyle != Qt::ToolButtonIconOnly) {
            p->setFont(toolbutton->font);
            QRect pr = rect, tr = rect;
            int alignment = Qt::TextShowMnemonic;
            if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
                alignment |= Qt::TextHideMnemonic;

            if (toolbutton->toolButtonStyle == Qt::ToolButtonTextUnderIcon) {
                pr.setHeight(pmSize.height() + 4);
                tr.adjust(0, pr.height() - 1, 0, -1);
                pr.translate(shiftX, shiftY);
                if (!hasArrow)
                    drawItemPixmap(p, pr, Qt::AlignCenter, pm);
                else {
                    QStyleOption arrowOpt(*toolbutton);
                    arrowOpt.rect = pr;
                    drawPrimitive(arrowPE, &arrowOpt, p, widget);
                }
                alignment |= Qt::AlignCenter;
            } else {
                pr.setWidth(pmSize.width() + 4);
                tr.adjust(pr.width(), 0, 0, 0);
                pr.translate(shiftX, shiftY);
                if (!hasArrow)
                    drawItemPixmap(p, visualRect(opt->direction, rect, pr), Qt::AlignCenter, pm);
                else {
                    QStyleOption arrowOpt(*toolbutton);
                    arrowOpt.rect = visualRect(opt->direction, rect, pr);
                    drawPrimitive(arrowPE, &arrowOpt, p, widget);
                }
                alignment |= Qt::AlignLeft | Qt::AlignVCenter;
            }
            tr.translate(shiftX, shiftY);
            drawItemText(p, visualRect(opt->direction, rect, tr), alignment, toolbutton->palette,
                         toolbutton->state & State_Enabled, toolbutton->text, QPalette::ButtonText);
        } else {
            rect.translate(shiftX, shiftY);
            if (hasArrow) {
                QStyleOption arrowOpt(*toolbutton);
                arrowOpt.rect = rect;
                drawPrimitive(arrowPE, &arrowOpt, p, widget);
            } else {
                drawItemPixmap(p, rect, Qt::AlignCenter, pm);
            }
        }
        break;
    }

    case CE_TabBarTabShape: {
        const QStyleOptionTab *tabOpt = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tabOpt)
            break;

        const bool below = tabOpt->shape == QTabBar::RoundedSouth || tabOpt->shape == QTabBar::TriangularSouth;
        QRect tr = r;
        QRect fr = r;

        if (below) {
            tr.adjust(0, 1, 0, 0);
            fr.adjust(2, 2, -2, -2);
        } else {
            tr.adjust(0, 0, 0, -1);
            fr.adjust(1, 2, -2, -2);
        }

        if (!(opt->state & State_Selected)) {
            if (below) {
                tr.adjust(0, 0, 0, -1);
                fr.adjust(0, 0, 0, -1);
            } else {
                tr.adjust(0, 1, 0, 0);
                fr.adjust(0, 1, 0, 0);
            }
            p->save();
            p->setPen(cdata->shades[6]);
            p->drawRect(tr.adjusted(0, 0, -1, -1));
            p->setPen(opt->palette.light().color());
            if (!below)
                p->drawLine(tr.left() + 1, tr.top() + 1, tr.right() - 1, tr.top() + 1);
            p->setPen(cdata->shades[2]);
            p->drawLine(tr.right() - 1, tr.top() + 1, tr.right() - 1, tr.bottom() - 1);
            p->fillRect(fr, cdata->shades[2]);
            p->restore();
        } else {
            fr.adjust(1, 0, 0, below ? 0 : 2);
            p->save();
            p->setPen(cdata->shades[6]);
            if (below) {
                p->drawLine(tr.left(), tr.bottom() - 1, tr.left(), tr.top() - 1);
                p->drawLine(tr.left(), tr.bottom(), tr.right(), tr.bottom());
                p->drawLine(tr.right(), tr.top(), tr.right(), tr.bottom() - 1);
            } else {
                p->drawLine(tr.left(), tr.bottom() + 1, tr.left(), tr.top() + 1);
                p->drawLine(tr.left(), tr.top(), tr.right(), tr.top());
                p->drawLine(tr.right(), tr.top() + 1, tr.right(), tr.bottom());
            }
            p->setPen(opt->palette.light().color());
            if (below)
                p->drawLine(tr.left() + 1, tr.bottom() - 1, tr.left() + 1, tr.top() - 2);
            else
                p->drawLine(tr.left() + 1, tr.bottom() + 2, tr.left() + 1, tr.top() + 2);
            p->fillRect(fr, opt->palette.window().color());
            p->restore();
        }
        break;
    }

    case CE_TabBarTabLabel: {
        const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
        if (!tab)
            break;
        QRect textRect = tab->rect;
        const bool verticalTabs = tab->shape == QTabBar::RoundedEast || tab->shape == QTabBar::RoundedWest
                                  || tab->shape == QTabBar::TriangularEast || tab->shape == QTabBar::TriangularWest;
        int alignment = Qt::AlignCenter | Qt::TextShowMnemonic;
        if (!proxy()->styleHint(SH_UnderlineShortcut, opt, widget))
            alignment |= Qt::TextHideMnemonic;

        if (verticalTabs) {
            p->save();
            int newX, newY, newRot;
            if (tab->shape == QTabBar::RoundedEast || tab->shape == QTabBar::TriangularEast) {
                newX = textRect.width() + textRect.x();
                newY = textRect.y();
                newRot = 90;
            } else {
                newX = textRect.x();
                newY = textRect.y() + textRect.height();
                newRot = -90;
            }
            QTransform m = QTransform::fromTranslate(newX, newY);
            m.rotate(newRot);
            p->setTransform(m, true);
            // 旋转后的坐标系以 tab 左上角为原点、宽高已交换，与 QCommonStyle::tabLayout
            // 的约定一致。若沿用带 y 偏移的 tab->rect 画文字，文字会被平移到 tab 之外，
            // 超出 tab bar 的绘制区域而被裁剪（垂直 tab 文字显示异常）。
            // 注意不能用 subElementRect(SE_TabBarTabText)：它返回的矩形已为图标预留
            // 空间，下面的 icon 分支还会再 setLeft 一次，叠加就是双重偏移。
            textRect = QRect(QPoint(0, 0), QSize(textRect.height(), textRect.width()));
        }

        if (!tab->icon.isNull()) {
            const QSize iconSize = tab->iconSize.isValid() ? tab->iconSize : QSize(16, 16);
            QRect iconRect(0, 0, iconSize.width(), iconSize.height());
            iconRect.moveTopLeft(QPoint(textRect.x() + 2,
                                        textRect.center().y() - iconSize.height() / 2));
            const QIcon::State state = (tab->state & State_Selected) ? QIcon::On : QIcon::Off;
            const QPixmap tabIcon = tab->icon.pixmap(iconSize, QIcon::Normal, state);
            p->drawPixmap(iconRect, tabIcon);
            textRect.setLeft(textRect.left() + iconSize.width() + 4);
        }
        proxy()->drawItemText(p, textRect, alignment, tab->palette, tab->state & State_Enabled,
                              tab->text, widget ? widget->foregroundRole() : QPalette::WindowText);
        if (verticalTabs)
            p->restore();

        if (tab->state & State_HasFocus) {
            const int OFFSET = 1 + pixelMetric(PM_DefaultFrameWidth, opt, widget);
            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*tab);
            fropt.rect.setRect(tab->rect.left() + 1 + OFFSET, tab->rect.y() + OFFSET,
                               tab->rect.width() - 2 * OFFSET, tab->rect.height() - 2 * OFFSET);
            drawPrimitive(PE_FrameFocusRect, &fropt, p, widget);
        }
        break;
    }

    case CE_MenuItem: {
        const QStyleOptionMenuItem *miOpt = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (!miOpt)
            break;
        const int tab = menuItemTabWidth(miOpt);
        int maxpmw = miOpt->maxIconWidth;
        const bool checked = miOpt->checkType != QStyleOptionMenuItem::NotCheckable && miOpt->checked;

        if (miOpt->menuItemType == QStyleOptionMenuItem::Separator) {
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Button));
            p->save();
            p->setPen(cdata->shades[2]);
            p->drawLine(r.left() + 6, r.top() + 4, r.right() - 6, r.top() + 4);
            p->setPen(opt->palette.light().color());
            p->drawLine(r.left() + 6, r.top() + 5, r.right() - 6, r.top() + 5);
            p->restore();
            break;
        }

        if ((opt->state & State_Selected) && (opt->state & State_Enabled)) {
            drawGradientBox(p, opt->rect, opt->palette, 0.9, 1.2);
        } else {
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Button));
        }

        maxpmw = qMax(maxpmw, 22);
        QRect cr(r.left(), r.top(), maxpmw, r.height());
        QRect sr(r.right() - 12, r.top(), 12, r.height());
        QRect tr(r.right() - tab - 4, r.top(), tab, r.height());
        QRect ir(cr.right() + 4, r.top(), tr.left() - cr.right() - 8, r.height());

        const bool reverse = QGuiApplication::isRightToLeft();
        if (reverse) {
            cr = visualRect(opt->direction, opt->rect, cr);
            sr = visualRect(opt->direction, opt->rect, sr);
            tr = visualRect(opt->direction, opt->rect, tr);
            ir = visualRect(opt->direction, opt->rect, ir);
        }

        if (!miOpt->icon.isNull()) {
            if (checked) {
                QStyleOption buttonOpt;
                buttonOpt.rect = cr;
                buttonOpt.state = opt->state | QStyle::State_On;
                buttonOpt.state &= ~QStyle::State_Sunken;
                if ((opt->state & State_Selected) && (opt->state & State_Enabled))
                    buttonOpt.state |= QStyle::State_MouseOver;
                buttonOpt.palette = opt->palette;
                drawPrimitive(PE_PanelButtonCommand, &buttonOpt, p, widget);
            }
            const QPixmap pixmap = miOpt->icon.pixmap(pixelMetric(PM_SmallIconSize, opt, widget),
                                                      QIcon::Normal, checked ? QIcon::On : QIcon::Off);
            QRect pmr(QPoint(0, 0), pixmap.size() / pixmap.devicePixelRatio());
            pmr.moveCenter(cr.center());
            p->drawPixmap(pmr.topLeft(), pixmap);
        } else if (checked) {
            QStyleOption checkOpt;
            checkOpt.state = (opt->state & (State_Enabled | State_Selected)) | State_On;
            checkOpt.rect = cr;
            checkOpt.palette = opt->palette;
            drawPrimitive(PE_IndicatorMenuCheckMark, &checkOpt, p, widget);
        }

        QColor textcolor;
        QColor embosscolor;
        if (opt->state & State_Selected) {
            if (!(opt->state & State_Enabled)) {
                textcolor = opt->palette.text().color();
                embosscolor = opt->palette.light().color();
            } else {
                textcolor = opt->palette.highlightedText().color();
                embosscolor = opt->palette.midlight().color().lighter();
            }
        } else if (!(opt->state & State_Enabled)) {
            textcolor = opt->palette.text().color();
            embosscolor = opt->palette.light().color();
        } else {
            textcolor = embosscolor = opt->palette.buttonText().color();
        }

        const QString text = miOpt->text;
        if (!text.isNull()) {
            const int t = text.indexOf(QLatin1Char('\t'));
            if (t >= 0) {
                int alignFlag = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
                alignFlag |= reverse ? Qt::AlignLeft : Qt::AlignRight;
                if (!(opt->state & State_Enabled)) {
                    p->setPen(embosscolor);
                    p->drawText(tr.translated(1, 1), alignFlag, text.mid(t + 1));
                    tr.translate(-1, -1);
                    p->setPen(textcolor);
                }
                p->drawText(tr, alignFlag, text.mid(t + 1));
            }
            int alignFlag = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
            alignFlag |= reverse ? Qt::AlignRight : Qt::AlignLeft;
            if (!(opt->state & State_Enabled)) {
                p->setPen(embosscolor);
                p->drawText(ir.translated(1, 1), alignFlag, text.left(t));
                ir.translate(-1, -1);
                p->setPen(textcolor);
            }
            p->drawText(ir, alignFlag, text.left(t));
        }

        if (miOpt->menuItemType == QStyleOptionMenuItem::SubMenu) {
            QStyleOption arrowOpt;
            arrowOpt.state = opt->state;
            arrowOpt.rect = QRect(0, 0, 8, 9);
            arrowOpt.rect.moveCenter(sr.center());
            arrowOpt.palette = opt->palette;
            drawPrimitive(reverse ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight, &arrowOpt, p, widget);
        }
        break;
    }

    case CE_MenuBarEmptyArea:
        p->fillRect(r, opt->palette.brush(QPalette::Button));
        break;

    case CE_MenuBarItem: {
        if ((opt->state & State_Enabled) && (opt->state & State_Sunken))
            drawGradientBox(p, opt->rect, opt->palette, 0.9, 1.2);
        else
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Button));

        const QStyleOptionMenuItem *miOpt = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        if (!miOpt)
            break;
        drawItemText(p, opt->rect,
                     Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine,
                     opt->palette, opt->state & State_Enabled, miOpt->text,
                     (opt->state & State_Sunken) ? QPalette::HighlightedText : QPalette::ButtonText);
        break;
    }

    case CE_ProgressBarGroove: {
        p->save();
        p->setBrush(cdata->shades[3]);
        p->setPen(cdata->shades[5]);
        p->drawRect(r.adjusted(0, 0, -1, -1));
        p->restore();
        break;
    }

    case CE_ProgressBarContents: {
        const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
        if (!pb)
            break;
        const bool vertical = !(pb->state & QStyle::State_Horizontal);
        const QRect pr = progressChunkRect(pb, opt->rect);
        if (pr.width() <= 0 || pr.height() <= 0)
            break;

        drawGradientBox(p, pr, opt->palette, 0.92, 1.66);

        if (pb->textVisible && !pb->text.isEmpty()) {
            // 块内文字：白色，clip 到填充块，避免盖到块外。SE_ProgressBarGroove
            // 已覆盖为整条，opt->rect 即整条，块内白字与 CE_ProgressBarLabel 的
            // 块外黑字按同一基准居中，clip 无缝衔接。
            const QRect textRect = opt->rect;
            p->save();
            p->setClipRect(pr);
            if (vertical) {
                // 旋转坐标系到条中心，文字矩形以原点为中心、宽高交换，
                // 旋转后正好纵向排布在条内。
                p->translate(textRect.center());
                // QProgressBar::TopToBottom（默认）为顺时针 90°；屏幕坐标
                // y 向下，顺时针在屏幕上表现为文字"7" 朝下，所以取反。
                p->rotate(pb->bottomToTop ? 90 : -90);
                drawItemText(p, QRect(-textRect.height() / 2, -textRect.width() / 2,
                                      textRect.height(), textRect.width()),
                             Qt::AlignCenter | Qt::TextSingleLine, pb->palette,
                             pb->state & State_Enabled, pb->text, QPalette::HighlightedText);
            } else {
                drawItemText(p, textRect, Qt::AlignCenter | Qt::TextSingleLine, pb->palette,
                             pb->state & State_Enabled, pb->text, QPalette::HighlightedText);
            }
            p->restore();
        }
        break;
    }

    case CE_ProgressBarLabel: {
        const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
        if (!pb || !pb->textVisible || pb->text.isEmpty())
            break;
        const bool vertical = !(pb->state & QStyle::State_Horizontal);
        // SE_ProgressBarLabel 已覆盖为整条，块外文字与 CE_ProgressBarContents
        // 的块/白字同基准（opt->rect），clip 排除区与实际块一致，不会在文字
        // 跨块边界时切出缺口。无 widget（item delegate）时 opt->rect 同样成立。
        const QRect fullRect = opt->rect;
        const QRect pr = progressChunkRect(pb, fullRect);
        p->save();
        // 块外文字：黑色，clip 排除填充块（白色文字已在 contents 里画过）。
        if (pr.width() > 0 && pr.height() > 0)
            p->setClipRegion(QRegion(fullRect).subtracted(QRegion(pr)));
        if (vertical) {
            p->translate(fullRect.center());
            p->rotate(pb->bottomToTop ? 90 : -90);
            drawItemText(p, QRect(-fullRect.height() / 2, -fullRect.width() / 2,
                                  fullRect.height(), fullRect.width()),
                         Qt::AlignCenter | Qt::TextSingleLine, pb->palette,
                         pb->state & State_Enabled, pb->text, QPalette::Text);
        } else {
            drawItemText(p, fullRect, Qt::AlignCenter | Qt::TextSingleLine, pb->palette,
                         pb->state & State_Enabled, pb->text, QPalette::Text);
        }
        p->restore();
        break;
    }

    case CE_CheckBox:
    case CE_RadioButton: {
        if (opt->state & State_MouseOver) {
            QRegion region(opt->rect);
            region -= subElementRect(element == CE_CheckBox ? SE_CheckBoxIndicator : SE_RadioButtonIndicator,
                                     opt, widget);
            p->save();
            p->setClipRegion(region);
            p->fillRect(opt->rect, opt->palette.brush(QPalette::Midlight));
            p->restore();
        }
        QCommonStyle::drawControl(element, opt, p, widget);
        break;
    }

    case CE_CheckBoxLabel:
    case CE_RadioButtonLabel: {
        const QStyleOptionButton *checkboxOpt = qstyleoption_cast<const QStyleOptionButton *>(opt);
        const int alignment = QGuiApplication::isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft;
        drawItemText(p, opt->rect, alignment | Qt::AlignVCenter | Qt::TextShowMnemonic,
                     opt->palette, opt->state & State_Enabled, checkboxOpt ? checkboxOpt->text : QString());
        break;
    }

    case CE_HeaderSection: {
        QStyle::State state = opt->state;
        if (!(state & (QStyle::State_Sunken | QStyle::State_Raised)))
            state |= QStyle::State_Raised;

        const QBrush *fill;
        if (state & State_On)
            fill = (state & State_Sunken) ? &opt->palette.brush(QPalette::Mid)
                                          : &opt->palette.brush(QPalette::Midlight);
        else
            fill = &opt->palette.brush(QPalette::Button);

        QStyleOption optCopy(*opt);
        optCopy.state = state;
        drawLightBevel(p, &optCopy, fill, false);
        break;
    }

    case CE_Splitter: {
        if (opt->state & State_MouseOver)
            p->fillRect(opt->rect, opt->palette.midlight());
        p->save();
        if (opt->state & State_Horizontal) {
            const int yMid = r.center().y() + 2;
            for (int i = 0; i < 21; i += 5) {
                p->setPen(cdata->shades[5]);
                p->drawLine(r.left() + 1, yMid - 10 + i, r.right() - 1, yMid - 10 + i - 3);
                p->setPen(Qt::white);
                p->drawLine(r.left() + 1, yMid - 10 + i + 1, r.right() - 1, yMid - 10 + i - 2);
            }
        } else {
            const int xMid = r.center().x() + 2;
            for (int i = 0; i < 21; i += 5) {
                p->setPen(cdata->shades[5]);
                p->drawLine(xMid - 10 + i + 3, r.top() + 1, xMid - 10 + i, r.bottom() - 1);
                p->setPen(Qt::white);
                p->drawLine(xMid - 10 + i + 4, r.top() + 1, xMid - 10 + i + 1, r.bottom() - 1);
            }
        }
        p->restore();
        break;
    }

    case CE_ScrollBarAddLine:
    case CE_ScrollBarSubLine: {
        QStyleOption optCopy(*opt);
        optCopy.state = opt->state | ((opt->state & State_Enabled) ? State_Raised : QStyle::State());
        const QBrush &fill = opt->palette.brush((opt->state & State_MouseOver) ? QPalette::Midlight
                                                                               : QPalette::Button);
        drawLightBevel(p, &optCopy, &fill, true);

        PrimitiveElement pe = PE_IndicatorArrowDown;
        if (element == CE_ScrollBarAddLine && (opt->state & State_Horizontal))
            pe = PE_IndicatorArrowRight;
        else if (element == CE_ScrollBarSubLine && (opt->state & State_Horizontal))
            pe = PE_IndicatorArrowLeft;
        else if (element == CE_ScrollBarSubLine)
            pe = PE_IndicatorArrowUp;

        QStyleOption arrowOpt(*opt);
        arrowOpt.state = opt->state & ~State_MouseOver;
        arrowOpt.rect = opt->rect.adjusted(3, 3, -3, -3);
        drawPrimitive(pe, &arrowOpt, p, widget);
        break;
    }

    case CE_ScrollBarSubPage:
    case CE_ScrollBarAddPage: {
        p->fillRect(opt->rect, cdata->shades[3]);
        p->save();
        p->setPen(cdata->shades[5]);
        if (opt->state & State_Horizontal) {
            p->drawLine(r.left(), r.top(), r.right(), r.top());
            p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        } else {
            p->drawLine(r.left(), r.top(), r.left(), r.bottom());
            p->drawLine(r.right(), r.top(), r.right(), r.bottom());
        }
        p->restore();
        break;
    }

    case CE_ScrollBarSlider: {
        QStyleOption optCopy(*opt);
        optCopy.state = (opt->state & ~State_Sunken) | ((opt->state & State_Enabled) ? State_Raised : QStyle::State());
        const QBrush &fill = opt->palette.brush((opt->state & State_MouseOver) ? QPalette::Midlight
                                                                               : QPalette::Button);
        drawLightBevel(p, &optCopy, &fill, true);

        if (opt->state & State_Horizontal && opt->rect.width() < 31)
            break;
        if (!(opt->state & State_Horizontal) && opt->rect.height() < 31)
            break;
        drawSliderGrip(p, cdata, r, opt->state & State_Horizontal);
        break;
    }

    case CE_SizeGrip: {
        const QStyleOptionSizeGrip *grip = qstyleoption_cast<const QStyleOptionSizeGrip *>(opt);
        if (!grip)
            break;
        const int step = 4;
        p->save();
        switch (grip->corner) {
        case Qt::TopLeftCorner: {
            int xi = r.right() - 2;
            int yi = r.bottom() - 2;
            while (xi > r.left() + 3) {
                p->setPen(cdata->shades[5]);
                p->drawLine(xi, r.top(), r.left(), yi);
                p->setPen(Qt::white);
                p->drawLine(xi - 1, r.top(), r.left(), yi - 1);
                xi -= step;
                yi -= step;
            }
            break;
        }
        case Qt::BottomRightCorner: {
            int xi = r.left();
            int yi = r.top();
            while (xi < r.right() - 3) {
                p->setPen(Qt::white);
                p->drawLine(xi, r.bottom(), r.right(), yi);
                p->setPen(cdata->shades[5]);
                p->drawLine(xi + 1, r.bottom(), r.right(), yi + 1);
                xi += step;
                yi += step;
            }
            break;
        }
        default:
            break;
        }
        p->restore();
        break;
    }

    case CE_ToolBar: {
        p->fillRect(opt->rect, opt->palette.button());
        p->save();
        p->setPen(cdata->shades[0]);
        p->drawLine(r.left(), r.top(), r.right(), r.top());
        p->setPen(cdata->shades[3]);
        p->drawLine(r.left(), r.bottom(), r.right(), r.bottom());
        p->restore();
        break;
    }

    default:
        QCommonStyle::drawControl(element, opt, p, widget);
        break;
    }
}

// ---------------------------------------------------------------------------
// drawComplexControl
// ---------------------------------------------------------------------------

void BluecurveStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *opt,
                                        QPainter *p, const QWidget *widget) const
{
    const ColorData *cdata = colorData(opt->palette);

    switch (control) {
    case CC_ComboBox: {
        const QStyleOptionComboBox *comboboxOpt = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
        const QRect frame = subControlRect(CC_ComboBox, opt, SC_ComboBoxFrame, widget);
        const QRect arrow = subControlRect(CC_ComboBox, opt, SC_ComboBoxArrow, widget);
        const QRect field = subControlRect(CC_ComboBox, opt, SC_ComboBoxEditField, widget);

        if ((opt->subControls & SC_ComboBoxFrame) && frame.isValid()) {
            QStyleOption frameOpt;
            frameOpt.state = opt->state | State_Raised;
            frameOpt.rect = frame;
            frameOpt.palette = opt->palette;
            const QBrush &fill = (opt->state & State_MouseOver) ? opt->palette.brush(QPalette::Midlight)
                                                                : opt->palette.brush(QPalette::Button);
            drawLightBevel(p, &frameOpt, &fill, true);
        }

        if ((opt->subControls & SC_ComboBoxArrow) && arrow.isValid()) {
            QStyleOption arrowOpt;
            // QComboBox sets State_Selected on focused non-editable combos;
            // the arrow still sits on the button face, so keep it buttonText
            // colored instead of the highlighted-text color.
            arrowOpt.state = opt->state & ~(State_MouseOver | State_Selected);
            arrowOpt.rect = QRect(0, 0, 9, 8);
            arrowOpt.rect.moveCenter(arrow.center());
            arrowOpt.palette = opt->palette;
            drawPrimitive(PE_IndicatorArrowDown, &arrowOpt, p, widget);

            p->save();
            p->setPen(cdata->shades[3]);
            const int midY = (arrow.top() + arrow.bottom()) / 2;
            p->drawLine(arrow.center().x() - 2, midY + 5, arrow.center().x() + 2, midY + 5);
            p->drawLine(arrow.center().x() - 2, midY + 6, arrow.center().x() + 2, midY + 6);
            p->restore();
        }

        if ((opt->subControls & SC_ComboBoxEditField) && field.isValid()) {
            p->save();
            p->setPen(cdata->shades[4]);
            QRect fieldRect = field;
            if (comboboxOpt && comboboxOpt->editable) {
                fieldRect.adjust(-1, -1, 1, 1);
                p->drawLine(fieldRect.right(), fieldRect.top() - 1,
                            fieldRect.right(), fieldRect.bottom() + 1);
                p->setPen(opt->palette.light().color());
                p->drawLine(fieldRect.right() + 1, fieldRect.top(),
                            fieldRect.right() + 1, fieldRect.bottom());
            } else {
                p->drawLine(fieldRect.right() + 1, fieldRect.top() - 2,
                            fieldRect.right() + 1, fieldRect.bottom() + 2);
                p->setPen(opt->palette.light().color());
                p->drawLine(fieldRect.right() + 2, fieldRect.top() - 1,
                            fieldRect.right() + 2, fieldRect.bottom() + 1);
            }
            p->restore();

            if (opt->state & State_HasFocus && !(comboboxOpt && comboboxOpt->editable)) {
                QStyleOption focusOpt(*opt);
                focusOpt.rect = field.adjusted(0, 0, -2, 0);
                drawPrimitive(PE_FrameFocusRect, &focusOpt, p, widget);
            }
        }
        break;
    }

    case CC_SpinBox: {
        const QStyleOptionSpinBox *spinboxOpt = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
        const QRect frame = subControlRect(CC_SpinBox, opt, SC_SpinBoxFrame, widget);
        const QRect up = subControlRect(CC_SpinBox, opt, SC_SpinBoxUp, widget);
        const QRect down = subControlRect(CC_SpinBox, opt, SC_SpinBoxDown, widget);

        if ((opt->subControls & SC_SpinBoxFrame) && frame.isValid()) {
            QStyleOption textOpt(*opt);
            textOpt.state = opt->state | State_Sunken;
            textOpt.rect = frame;
            drawTextRect(p, &textOpt, &opt->palette.brush(QPalette::Base));
        }

        p->save();
        p->setPen(cdata->shades[5]);
        p->drawLine(up.topLeft(), down.bottomLeft());
        p->drawLine(up.left(), up.bottom() + 1, up.right(), up.bottom() + 1);
        p->restore();

        if ((opt->subControls & SC_SpinBoxUp) && up.isValid()) {
            const bool sunken = (opt->activeSubControls == SC_SpinBoxUp) && (opt->state & State_Sunken);
            QRect ur = up.adjusted(1, 0, 0, 0);
            p->fillRect(ur, opt->palette.brush(QPalette::Button));
            p->save();
            p->setPen(sunken ? cdata->shades[2] : opt->palette.light().color());
            p->drawLine(ur.left(), ur.top(), ur.right(), ur.top());
            p->drawLine(ur.left(), ur.top(), ur.left(), ur.bottom());
            p->setPen(sunken ? opt->palette.light().color() : cdata->shades[2]);
            p->drawLine(ur.right(), ur.top() + 1, ur.right(), ur.bottom());
            p->drawLine(ur.left() + 1, ur.bottom(), ur.right(), ur.bottom());
            p->restore();

            QStyleOption arrowOpt;
            arrowOpt.state = opt->state;
            arrowOpt.rect = ur.adjusted(0, 1, 0, -1);
            arrowOpt.palette = opt->palette;
            drawPrimitive(spinboxOpt && spinboxOpt->buttonSymbols == QAbstractSpinBox::PlusMinus
                              ? PE_IndicatorSpinPlus
                              : PE_IndicatorSpinUp,
                          &arrowOpt, p, widget);
        }

        if ((opt->subControls & SC_SpinBoxDown) && down.isValid()) {
            const bool sunken = (opt->activeSubControls == SC_SpinBoxDown) && (opt->state & State_Sunken);
            QRect dr = down.adjusted(1, 0, 0, 0);
            p->fillRect(dr, opt->palette.brush(QPalette::Button));
            p->save();
            p->setPen(sunken ? cdata->shades[2] : opt->palette.light().color());
            p->drawLine(dr.left(), dr.top(), dr.right(), dr.top());
            p->drawLine(dr.left(), dr.top(), dr.left(), dr.bottom());
            p->setPen(sunken ? opt->palette.light().color() : cdata->shades[2]);
            p->drawLine(dr.right(), dr.top() + 1, dr.right(), dr.bottom());
            p->drawLine(dr.left() + 1, dr.bottom(), dr.right(), dr.bottom());
            p->restore();

            QStyleOption arrowOpt;
            arrowOpt.state = opt->state;
            arrowOpt.rect = dr.adjusted(0, 1, 0, -1);
            arrowOpt.palette = opt->palette;
            drawPrimitive(spinboxOpt && spinboxOpt->buttonSymbols == QAbstractSpinBox::PlusMinus
                              ? PE_IndicatorSpinMinus
                              : PE_IndicatorSpinDown,
                          &arrowOpt, p, widget);
        }
        break;
    }

    case CC_Slider: {
        const QStyleOptionSlider *sliderOpt = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        QRect groove = subControlRect(CC_Slider, opt, SC_SliderGroove, widget);
        QRect handle = subControlRect(CC_Slider, opt, SC_SliderHandle, widget);

        if ((opt->subControls & SC_SliderGroove) && groove.isValid()) {
            if (opt->state & State_HasFocus) {
                QStyleOption focusOpt;
                focusOpt.state = QStyle::State();
                focusOpt.rect = groove;
                focusOpt.palette = opt->palette;
                drawPrimitive(PE_FrameFocusRect, &focusOpt, p, widget);
            }

            if (sliderOpt && sliderOpt->orientation == Qt::Horizontal) {
                const int dh = (groove.height() - 5) / 2;
                groove.adjust(0, dh, 0, -dh);
            } else {
                const int dw = (groove.width() - 5) / 2;
                groove.adjust(dw, 0, -dw, 0);
            }

            p->save();
            p->setPen(cdata->shades[5]);
            p->setBrush(opt->palette.mid().color());
            p->drawRect(groove.adjusted(0, 0, -1, -1));
            p->setPen(cdata->shades[4]);
            p->drawLine(groove.left() + 1, groove.top() + 1, groove.left() + 1, groove.bottom() - 1);
            p->drawLine(groove.left() + 1, groove.top() + 1, groove.right() - 1, groove.top() + 1);
            p->restore();
        }

        if ((opt->subControls & SC_SliderHandle) && handle.isValid()) {
            p->save();
            p->setPen(cdata->shades[6]);
            p->drawRect(handle.adjusted(0, 0, -1, -1));
            p->setPen(Qt::white);
            p->drawLine(handle.left() + 2, handle.top() + 1, handle.right() - 2, handle.top() + 1);
            p->drawLine(handle.left() + 1, handle.top() + 2, handle.left() + 1, handle.bottom() - 2);
            p->fillRect(handle.adjusted(2, 2, -2, -2),
                        opt->palette.brush((opt->state & State_MouseOver) ? QPalette::Midlight
                                                                          : QPalette::Button));
            p->restore();

            if (sliderOpt && sliderOpt->orientation == Qt::Horizontal) {
                const int x = handle.x() + handle.width() / 2 - 5;
                const int y = handle.y() + (handle.height() - 7) / 2;
                p->save();
                p->setPen(cdata->shades[5]);
                p->drawLine(x, y + 4, x + 3, y + 1);
                p->drawLine(x + 2, y + 6, x + 8, y);
                p->drawLine(x + 7, y + 5, x + 10, y + 2);
                p->setPen(Qt::white);
                p->drawLine(x + 1, y + 4, x + 3, y + 2);
                p->drawLine(x + 3, y + 6, x + 8, y + 1);
                p->drawLine(x + 8, y + 5, x + 10, y + 3);
                p->restore();
            } else {
                const int x = handle.x() + (handle.width() - 7) / 2;
                const int y = handle.y() + handle.height() / 2 - 5;
                p->save();
                p->setPen(cdata->shades[5]);
                p->drawLine(x + 4, y, x + 1, y + 3);
                p->drawLine(x + 6, y + 2, x, y + 8);
                p->drawLine(x + 5, y + 7, x + 2, y + 10);
                p->setPen(Qt::white);
                p->drawLine(x + 4, y + 1, x + 2, y + 3);
                p->drawLine(x + 6, y + 3, x + 1, y + 8);
                p->drawLine(x + 5, y + 8, x + 3, y + 10);
                p->restore();
            }
        }

        if (opt->subControls & SC_SliderTickmarks) {
            QStyleOptionComplex optCopy(*opt);
            optCopy.subControls = SC_SliderTickmarks;
            QCommonStyle::drawComplexControl(control, &optCopy, p, widget);
        }
        break;
    }

    case CC_ToolButton: {
        const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(opt);
        if (!toolbutton)
            break;

        const QRect button = subControlRect(control, toolbutton, SC_ToolButton, widget);
        const QRect menuarea = subControlRect(control, toolbutton, SC_ToolButtonMenu, widget);

        State bflags = toolbutton->state & ~State_Sunken;
        if (bflags & State_AutoRaise) {
            if (!(bflags & State_MouseOver) || !(bflags & State_Enabled))
                bflags &= ~State_Raised;
        }
        State mflags = bflags;
        if (toolbutton->state & State_Sunken) {
            if (toolbutton->activeSubControls & SC_ToolButton)
                bflags |= State_Sunken;
            mflags |= State_Sunken;
        }

        QStyleOption tool = *toolbutton;
        if (toolbutton->subControls & SC_ToolButton) {
            if (bflags & (State_Sunken | State_On | State_Raised)) {
                if (toolbutton->subControls & SC_ToolButtonMenu) {
                    QBrush fill;
                    const bool sunken = bflags & (QStyle::State_On | QStyle::State_Sunken);
                    QRect br = button;

                    if (bflags & QStyle::State_Sunken)
                        fill = opt->palette.brush(QPalette::Mid);
                    else if (bflags & QStyle::State_MouseOver)
                        fill = opt->palette.brush(QPalette::Midlight);
                    else
                        fill = (bflags & QStyle::State_On) ? opt->palette.brush(QPalette::Mid)
                                                           : opt->palette.brush(QPalette::Button);

                    p->save();
                    p->setPen(cdata->shades[6]);
                    p->drawRect(button.adjusted(0, 0, -1, -1));
                    if (bflags & (QStyle::State_On | QStyle::State_Sunken | QStyle::State_Raised)) {
                        p->setPen(sunken ? Qt::white : cdata->shades[2]);
                        p->drawLine(button.left() + 1, button.bottom() - 1, button.right() - 1, button.bottom() - 1);
                        p->setPen(sunken ? cdata->shades[2] : Qt::white);
                        p->drawLine(button.left() + 1, button.top() + 1, button.right() - 1, button.top() + 1);
                        p->drawLine(button.left() + 1, button.top() + 1, button.left() + 1, button.bottom() - 2);
                        br.adjust(2, 2, -1, -2);
                    } else {
                        br.adjust(1, 1, 0, -1);
                    }
                    p->restore();
                    p->fillRect(br, fill);
                } else {
                    tool.rect = button;
                    tool.state = bflags;
                    drawPrimitive(PE_PanelButtonTool, &tool, p, widget);
                }
            }
        }

        if (toolbutton->state & State_HasFocus) {
            QStyleOptionFocusRect fr;
            fr.QStyleOption::operator=(*toolbutton);
            fr.rect.adjust(3, 3, -3, -3);
            if (toolbutton->features & QStyleOptionToolButton::MenuButtonPopup)
                fr.rect.adjust(0, 0, -pixelMetric(QStyle::PM_MenuButtonIndicator, toolbutton, widget), 0);
            drawPrimitive(PE_FrameFocusRect, &fr, p, widget);
        }

        QStyleOptionToolButton label = *toolbutton;
        label.state = bflags;
        const int fw = proxy()->pixelMetric(PM_DefaultFrameWidth, opt, widget);
        label.rect = button.adjusted(fw, fw, -fw, -fw);
        drawControl(CE_ToolButtonLabel, &label, p, widget);

        if (toolbutton->subControls & SC_ToolButtonMenu) {
            tool.rect = menuarea;
            tool.state = mflags;
            if (mflags & (State_Sunken | State_On | State_Raised))
                drawPrimitive(PE_IndicatorButtonDropDown, &tool, p, widget);
            drawPrimitive(PE_IndicatorArrowDown, &tool, p, widget);
        }
        break;
    }

    default:
        QCommonStyle::drawComplexControl(control, opt, p, widget);
        break;
    }
}

// ---------------------------------------------------------------------------
// 布局与尺寸
// ---------------------------------------------------------------------------

QRect BluecurveStyle::subElementRect(SubElement element, const QStyleOption *opt,
                                     const QWidget *widget) const
{
    QRect rect;
    switch (element) {
    case SE_PushButtonFocusRect: {
        const QStyleOptionButton *buttonOpt = qstyleoption_cast<const QStyleOptionButton *>(opt);
        int dbw1 = 0;
        if (buttonOpt && (buttonOpt->features & QStyleOptionButton::DefaultButton))
            dbw1 = pixelMetric(PM_ButtonDefaultIndicator, opt, widget);
        const int dbw2 = dbw1 * 2;
        rect.setRect(opt->rect.x() + 3 + dbw1, opt->rect.y() + 3 + dbw1,
                     opt->rect.width() - 6 - dbw2, opt->rect.height() - 6 - dbw2);
        break;
    }
    case SE_ProgressBarGroove:
    case SE_ProgressBarLabel:
        // 文字画在整个条上（块内白/块外黑），不沿用 QCommonStyle 给右侧
        // label 预留的收窄槽：收窄会让 CE_ProgressBarContents（按槽算块）
        // 与 CE_ProgressBarLabel（按整条算排除区）的块基准错开，进度处于
        // 中段时文字会被两边的 clip 一起切掉。统一按整条计算。
        rect = opt->rect;
        break;
    default:
        rect = QCommonStyle::subElementRect(element, opt, widget);
        break;
    }
    return rect;
}

QRect BluecurveStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *opt,
                                     SubControl sc, const QWidget *widget) const
{
    QRect ret;
    switch (control) {
    case CC_SpinBox: {
        QSize bs;
        bs.setHeight(opt->rect.height() / 2 - 2);
        if (bs.height() < 8)
            bs.setHeight(8);
        bs.setWidth(bs.height() * 8 / 6);

        const int y = 1;
        const int x = opt->rect.width() - 1 - bs.width();

        switch (sc) {
        case SC_SpinBoxUp:
            ret.setRect(x, y, bs.width(), bs.height() + 1);
            break;
        case SC_SpinBoxDown:
            ret.setRect(x, y + bs.height() + 2, bs.width(), bs.height() + 1);
            break;
        case SC_SpinBoxEditField:
            ret = opt->rect.adjusted(0, 0, -bs.width(), 0);
            break;
        case SC_SpinBoxFrame:
            ret = opt->rect;
            break;
        default:
            break;
        }
        break;
    }

    case CC_ComboBox: {
        ret = QCommonStyle::subControlRect(control, opt, sc, widget);
        switch (sc) {
        case SC_ComboBoxArrow:
            ret.setTop(ret.top() - 2);
            ret.setLeft(ret.left() - 1);
            break;
        case SC_ComboBoxEditField:
            ret.setRight(ret.right() - 2);
            break;
        default:
            break;
        }
        break;
    }

    default:
        ret = QCommonStyle::subControlRect(control, opt, sc, widget);
        break;
    }
    return ret;
}

int BluecurveStyle::pixelMetric(PixelMetric metric, const QStyleOption *opt,
                                const QWidget *widget) const
{
    int ret;
    switch (metric) {
    case PM_ButtonMargin:
        ret = 10;
        break;
    case PM_ButtonDefaultIndicator:
        ret = 0;
        break;
    case PM_MenuButtonIndicator:
        ret = widget ? qMax(12, (widget->height() - 4) / 3) : 12;
        break;
    case PM_ButtonShiftHorizontal:
    case PM_ButtonShiftVertical:
        ret = 0;
        break;
    case PM_DefaultFrameWidth:
        ret = (widget && widget->inherits("QStackedWidget")) ? 2 : 1;
        break;
    case PM_MaximumDragDistance:
        ret = -1;
        break;
    case PM_ScrollBarExtent:
        ret = 15;
        break;
    case PM_ScrollBarSliderMin:
        ret = 31;
        break;
    case PM_SliderControlThickness: {
        const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!sl) {
            ret = 16;
            break;
        }
        int space = (sl->orientation == Qt::Horizontal) ? sl->rect.height() : sl->rect.width();
        int ticks = sl->tickPosition;
        int n = 0;
        if (ticks & QSlider::TicksAbove)
            ++n;
        if (ticks & QSlider::TicksBelow)
            ++n;
        if (!n) {
            ret = space;
            break;
        }
        int thick = 6;
        space -= thick;
        if (space > 0)
            thick += (space * 2) / (n + 2);
        ret = thick;
        break;
    }
    case PM_SliderLength:
        ret = 31;
        if (widget && widget->inherits("QSlider")) {
            const QSlider *slider = static_cast<const QSlider *>(widget);
            if (slider->orientation() == Qt::Horizontal) {
                if (widget->width() < ret)
                    ret = widget->width();
            } else if (widget->height() < ret) {
                ret = widget->height();
            }
        }
        break;
    case PM_DockWidgetSeparatorExtent:
    case PM_SplitterWidth:
        ret = 6;
        break;
    case PM_DockWidgetHandleExtent:
        ret = 10;
        break;
    case PM_MenuBarPanelWidth:
        ret = 1;
        break;
    case PM_ToolBarItemSpacing:
        ret = 0;
        break;
    case PM_TabBarTabOverlap:
        ret = 1;
        break;
    case PM_TabBarTabHSpace:
        ret = 11;
        break;
    case PM_TabBarTabVSpace:
        ret = 13;
        break;
    case PM_TabBarBaseHeight:
        ret = 0;
        break;
    case PM_TabBarBaseOverlap:
        ret = 2;
        break;
    case PM_TabBarTabShiftVertical:
        ret = 0;
        break;
    case PM_ProgressBarChunkWidth:
        ret = 2;
        break;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        ret = 13;
        break;
    case PM_MenuPanelWidth:
        ret = 3;
        break;
    case PM_MenuVMargin:
        ret = 1;
        break;
    case PM_HeaderMarkSize:
        ret = 32;
        break;
    case PM_ButtonIconSize:
        ret = 20;
        break;
    case PM_SubMenuOverlap:
        ret = 2;
        break;
    default:
        ret = QCommonStyle::pixelMetric(metric, opt, widget);
        break;
    }
    return ret;
}

QSize BluecurveStyle::sizeFromContents(ContentsType contents, const QStyleOption *opt,
                                       const QSize &contentsSize, const QWidget *widget) const
{
    QSize ret = QCommonStyle::sizeFromContents(contents, opt, contentsSize, widget);

    switch (contents) {
    case CT_PushButton: {
        const QStyleOptionButton *buttonOpt = qstyleoption_cast<const QStyleOptionButton *>(opt);
        int w = ret.width(), h = ret.height();
        if (buttonOpt && buttonOpt->icon.isNull()) {
            if (w < 85)
                w = 85;
            if (h < 30)
                h = 30;
        }
        ret = QSize(w, h);
        break;
    }

    case CT_MenuItem:
    case CT_MenuBarItem: {
        const QStyleOptionMenuItem *miOpt = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
        int w = contentsSize.width(), h = contentsSize.height();

        if (miOpt && miOpt->menuItemType == QStyleOptionMenuItem::Separator) {
            w = 10;
            h = 12;
        } else {
            if (h < 16)
                h = 16;
            if (opt)
                h = qMax(h, opt->fontMetrics.height() + 10);
        }
        if (contents == CT_MenuItem)
            w += qMax(miOpt ? miOpt->maxIconWidth : 0, 16) + 16;
        else
            w += 16;
        if (miOpt && !miOpt->text.isNull() && miOpt->text.indexOf(QLatin1Char('\t')) >= 0)
            w += 8;
        ret = QSize(w, h);
        break;
    }

    case CT_ToolButton: {
        int w = ret.width(), h = ret.height();
        if (h < 32)
            h = 32;
        if (w < 32)
            w = 32;
        ret = QSize(w, h);
        break;
    }

    case CT_ComboBox: {
        if (ret.height() < 27)
            ret.setHeight(27);
        break;
    }

    case CT_SpinBox: {
        if (ret.height() < 25)
            ret.setHeight(25);
        break;
    }

    case CT_SizeGrip: {
        const int size = qMax(qMax(ret.width(), ret.height()), 18);
        ret = QSize(size, size);
        break;
    }

    case CT_Slider: {
        const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt);
        if (!slider)
            break;
        if (slider->orientation == Qt::Horizontal) {
            if (ret.height() < 17)
                ret.setHeight(17);
        } else if (ret.width() < 17) {
            ret.setWidth(17);
        }
        break;
    }

    default:
        break;
    }
    return ret;
}

int BluecurveStyle::styleHint(StyleHint hint, const QStyleOption *opt, const QWidget *widget,
                              QStyleHintReturn *returnData) const
{
    int ret;
    switch (hint) {
    case SH_EtchDisabledText:
    case SH_ScrollBar_MiddleClickAbsolutePosition:
    case SH_Slider_SnapToValue:
    case SH_PrintDialog_RightAlignButtons:
    case SH_FontDialog_SelectAssociatedText:
    case SH_Menu_SpaceActivatesItem:
    case SH_MenuBar_AltKeyNavigation:
    case SH_Menu_MouseTracking:
    case SH_MenuBar_MouseTracking:
    case SH_ComboBox_ListMouseTracking:
    case SH_UnderlineShortcut:
    case SH_ToolBar_Movable:
        ret = 1;
        break;
    case SH_MainWindow_SpaceBelowMenuBar:
    case SH_Menu_AllowActiveAndDisabled:
        ret = 0;
        break;
    default:
        ret = QCommonStyle::styleHint(hint, opt, widget, returnData);
        break;
    }
    return ret;
}
