/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>
#include <QtCore/qstring.h>
#include <QtGui/qfontmetrics.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpolygon.h>
#include <QtCore/qstringbuilder.h>
#include <QtGui/qaccessible.h>
#include <QtWidgets/qstyleoption.h>

#ifndef QSTYLEHELPER_P_H
#define QSTYLEHELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qhexstring_p.h"

QT_BEGIN_NAMESPACE

class QPainter;
class QPixmap;
class QStyleOptionSlider;
class QStyleOption;
class QWindow;

namespace QStyleHelper
{
    QString uniqueName(const QString &key, const QStyleOption *option, const QSize &size);
    QString uniqueName(const QString &key, const QStyleOption *option, const QSize &size, qreal dpr);
#ifndef QT_NO_DIAL
    qreal angle(const QPointF &p1, const QPointF &p2);
    QPolygonF calcLines(const QStyleOptionSlider *dial);
    int calcBigLineSize(int radius);
    void drawDial(const QStyleOptionSlider *dial, QPainter *painter);
#endif //QT_NO_DIAL
    void drawBorderPixmap(const QPixmap &pixmap, QPainter *painter, const QRect &rect,
                     int left = 0, int top = 0, int right = 0,
                     int bottom = 0);
#ifndef QT_NO_ACCESSIBILITY
    bool isInstanceOf(QObject *obj, QAccessible::Role role);
    bool hasAncestor(QObject *obj, QAccessible::Role role);
#endif
    QColor backgroundColor(const QPalette &pal, const QWidget* widget = 0);
    QWindow *styleObjectWindow(QObject *so);

    // HiDPI helpers.  Qt 6 keeps these in qstylehelper_p.h as exported
    // functions; we keep them inline here so every style plugin can use them
    // without linking qstylehelper.cpp for their sake alone.
    static inline qreal dpi(const QStyleOption *option)
    {
        if (option) {
            const qreal fontDpi = option->fontMetrics.fontDpi();
            // fontDpi is uninitialised on some platforms (e.g. the offscreen
            // platform plugin) unless a QFont has been resolved against a
            // screen; fall back to the historic 96 DPI baseline in that case.
            if (fontDpi > 0)
                return fontDpi;
        }
        return qreal(96);
    }
    static inline qreal dpiScaled(qreal value, qreal dpi)
    {
        return value * dpi / qreal(96);
    }
    static inline qreal dpiScaled(qreal value, const QStyleOption *option)
    {
        return dpiScaled(value, dpi(option));
    }
    static inline qreal getDpr(const QPainter *painter)
    {
        Q_ASSERT(painter && painter->device());
        return painter->device()->devicePixelRatio();
    }
}


QT_END_NAMESPACE

#endif // QSTYLEHELPER_P_H
